#include <fstream>
#include "MovieProducer.h"
#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "InstrOverlayMaker.h"
#include "PolarOverlayMaker.h"
#include "StartTimerOverlayMaker.h"
#include "TargetsOverlayMaker.h"
#include "utils/Caffeine.h"
#include "PerformanceOverlayMaker.h"
#include "OverlayMaker.h"
#include "RudderOverlayMaker.h"
#include "PilotClipOverlayMaker.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MovieProducer::MovieProducer(const std::string &path, const std::string &polarPath, std::list<GoProClipInfo> &clipsList,
                             std::vector<InstrumentInput> &instrDataVector,
                             std::map<uint64_t, Performance> &performanceVector,
                             std::list<RaceData *> &raceList,
                             IProgressListener &rProgressListener)
  : m_moviePath(path)
    , m_rGoProClipInfoList(clipsList)
    , m_rInstrDataVector(instrDataVector)
    , m_rPerformanceVector(performanceVector)
    , m_RaceDataList(raceList)
    , m_rProgressListener(rProgressListener) {
  m_polars.loadPolar(polarPath);
}

void MovieProducer::makePilotClips(std::list<CameraClipInfo *> &rCameraClipsList) {
  Caffeine caffeine; // Prevent Mac from going to sleep while this variable is in scope
  std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / "40-PILOTS";
  std::cout << "Creating race folder " << raceFolder << std::endl;
  std::filesystem::create_directories(raceFolder);

  for (const auto &clip: rCameraClipsList) {
    int width = 1920;
    int height = 1080;

    PilotClipOverlayMaker pilotClipOverlayMaker(width, height, 0, 0);
    OverlayMaker overlayMaker(raceFolder, width, height);
    overlayMaker.addOverlayElement(pilotClipOverlayMaker);

    std::cout << "Making pilot clip for " << clip->getFileName() << std::endl;
    std::filesystem::path clipPath = std::filesystem::path(m_moviePath) / clip->getFileName();
    if (!std::filesystem::exists(clipPath)) {
      std::cerr << "Clip file " << clipPath << " does not exist, skipping" << std::endl;
      continue;
    }

    auto clipBaseName = std::filesystem::path(clip->getFileName()).filename();
    auto pilotClipName = std::filesystem::path(clip->getFileName() + ".pilot.mov").filename();

    uint64_t startTimeMs = clip->getInstrData()->front().utc.getUnixTimeMs();
    uint64_t endTimeMs = clip->getInstrData()->back().utc.getUnixTimeMs();
    int totalCount = 0;

    // Start timing image creation
    auto startImageGeneration = std::chrono::high_resolution_clock::now();

    EncodingProgressListener epochsProgressListener("Epochs for " +
                                                    std::filesystem::path(clip->getFileName()).filename().string(),
                                                    endTimeMs - startTimeMs,
                                                    m_rProgressListener);

    uint64_t nextEpochMs = startTimeMs;
    for (const auto &epoch: *clip->getInstrData()) {
      if (epoch.utc.getUnixTimeMs() >= nextEpochMs) {
        epochsProgressListener.ffmpegProgress(nextEpochMs - startTimeMs);
        overlayMaker.addEpoch(epoch, true);
        nextEpochMs = epoch.utc.getUnixTimeMs() + 1000;
        totalCount++;
      }
      m_stopRequested = epochsProgressListener.isStopRequested();
      if (m_stopRequested) {
        return;
      }
    }

    if (totalCount < 2) {
      std::cerr << "Clip file " << clipPath << " has too few epochs, skipping" << std::endl;
      continue;
    }

    // End timing image creation
    auto endImageGeneration = std::chrono::high_resolution_clock::now();
    auto imageGenerationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endImageGeneration - startImageGeneration).count();
    std::cout << "Generated " << overlayMaker.getImageQueue().size() << " frames in "
        << imageGenerationTime << "ms ("
        << (!overlayMaker.getImageQueue().empty() ? imageGenerationTime / overlayMaker.getImageQueue().size() : 0)
        << "ms per frame)" << std::endl;

    auto presentationDuration = float(endTimeMs - startTimeMs) / 1000;
    float overlaysFps = float(totalCount) / presentationDuration;

    std::filesystem::path clipFulPathName = raceFolder / pilotClipName;

    // Convert startTimeMs from UTC to local so it matches time shown in the instrument cell
    QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(startTimeMs));
    QTimeZone tz = time.timeZone();
    int offsetSec = tz.offsetFromUtc(time);


    FFMpeg ffmpeg;
    std::cout << "Initializing encoder..." << std::endl;
    bool initSuccess = ffmpeg.initializeEncoder(
      clipFulPathName.string(),
      overlayMaker.getWidth(),
      overlayMaker.getHeight(),
      overlaysFps,
      startTimeMs + offsetSec * 1000
    );

    // Start timing encoding process
    auto startEncoding = std::chrono::high_resolution_clock::now();
    if (!initSuccess) {
      std::cerr << "Failed to initialize encoder for " << clipFulPathName.string() << std::endl;
      return;
    }

    uint64_t clipDurationMs = presentationDuration * 1000;
    EncodingProgressListener
        progressListener("Creating " + pilotClipName.string(), clipDurationMs, m_rProgressListener);

    // Encode all images from the queue
    std::cout << "Starting encoding of " << overlayMaker.getImageQueue().size() << " frames..." << std::endl;
    bool encodingSuccess = ffmpeg.encodeQImageSequence(overlayMaker.getImageQueue(), overlaysFps, progressListener);

    // End timing encoding process
    auto endEncoding = std::chrono::high_resolution_clock::now();
    auto encodingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endEncoding - startEncoding).count();
    std::cout << "Encoded " << overlayMaker.getImageQueue().size() << " frames in "
        << encodingTime << "ms ("
        << (overlayMaker.getImageQueue().size() > 0 ? encodingTime / overlayMaker.getImageQueue().size() : 0)
        << "ms per frame)" << std::endl;

    if (!encodingSuccess) {
      std::cerr << "Failed to encode image sequence" << std::endl;
      m_stopRequested = true;
      return;
    }

    m_stopRequested = progressListener.isStopRequested();

    if (m_stopRequested) {
      return;
    }
  }
}

void MovieProducer::makeOverlaysForAllClips(std::list<CameraClipInfo *> &rCameraClipsList) {
  Caffeine caffeine; // Prevent Mac from going to sleep while this variable is in scope
  std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / "50-INSTR-OVERLAYS";
  std::cout << "Creating race folder " << raceFolder << std::endl;
  std::filesystem::create_directories(raceFolder);

  uint64_t gunIdx = UINT64_MAX;

  // Determine the start time by iteartin over all chapters in all races and finding the one that has type == RaceStart
  for (auto &race : m_RaceDataList) {
    for (auto &chapter : race->getChapters()) {
      if (chapter->getChapterType() == ChapterTypes::START) {
        gunIdx = chapter->getGunIdx();
        break;
      }
    }
    if (gunIdx != UINT64_MAX) {
      break;
    }
  }

  for (const auto &clip: rCameraClipsList) {
    int movieWidth = 1920;
    int movieHeight = 1080;

    int instr_ovl_width = movieWidth;
    int instrOvlHeight = 128;

    int polar_ovl_width = 400;
    int polar_ovl_height = polar_ovl_width;

    int rudder_ovl_width = 400;
    int rudder_ovl_height = 200;

    int timerHeight = 256;
    int timerWidth = 360;
    int timerX = -1; // Right aligned

    InstrOverlayMaker instrOverlayMaker(m_rInstrDataVector, instr_ovl_width, instrOvlHeight, 0,
                                        movieHeight - instrOvlHeight);
    instrOverlayMaker.initHistory(m_rInstrDataVector);

    PolarOverlayMaker polarOverlayMaker(m_polars, m_rInstrDataVector, polar_ovl_width, polar_ovl_height, 0, 0);
    polarOverlayMaker.initHistory(m_rInstrDataVector);

    RudderOverlayMaker rudderOverlayMaker(rudder_ovl_width, rudder_ovl_height, 0, polar_ovl_height);
    rudderOverlayMaker.initHistory(m_rInstrDataVector);

    StartTimerOverlayMaker startTimerOverlayMaker(m_polars, m_rInstrDataVector, timerWidth, timerHeight, timerX, 0);
    startTimerOverlayMaker.initStartEpochs(gunIdx);

    OverlayMaker overlayMaker(raceFolder, movieWidth, movieHeight);

    overlayMaker.addOverlayElement(instrOverlayMaker);
    overlayMaker.addOverlayElement(polarOverlayMaker);
    overlayMaker.addOverlayElement(rudderOverlayMaker);
    overlayMaker.addOverlayElement(startTimerOverlayMaker);


    std::cout << "Making overlay for " << clip->getFileName() << std::endl;
    std::filesystem::path clipPath = std::filesystem::path(m_moviePath) / clip->getFileName();
    if (!std::filesystem::exists(clipPath)) {
      std::cerr << "Clip file " << clipPath << " does not exist, skipping" << std::endl;
      continue;
    }

    auto clipBaseName = std::filesystem::path(clip->getFileName()).filename();
    auto pilotClipName = std::filesystem::path(clip->getFileName() + ".overlay.mov").filename();
    std::filesystem::path clipFulPathName = raceFolder / pilotClipName;
    // Skip if file already exists
    if (std::filesystem::exists(clipFulPathName)) {
      std::cout << "Overlay clip " << clipFulPathName << " already exists, skipping" << std::endl;
      continue;
    }


    uint64_t startTimeMs = clip->getInstrData()->front().utc.getUnixTimeMs();
    uint64_t endTimeMs = clip->getInstrData()->back().utc.getUnixTimeMs();
    int totalCount = 0;

    // Start timing image creation
    auto startImageGeneration = std::chrono::high_resolution_clock::now();

    EncodingProgressListener epochsProgressListener("Epochs for " +
                                                    std::filesystem::path(clip->getFileName()).filename().string(),
                                                    endTimeMs - startTimeMs,
                                                    m_rProgressListener);

    uint64_t nextEpochMs = startTimeMs;
    for (const auto &epoch: *clip->getInstrData()) {
      if (epoch.utc.getUnixTimeMs() >= nextEpochMs) {
        epochsProgressListener.ffmpegProgress(nextEpochMs - startTimeMs);
        overlayMaker.addEpoch(epoch, true);
        nextEpochMs = epoch.utc.getUnixTimeMs() + 1000;
        totalCount++;
      }
      m_stopRequested = epochsProgressListener.isStopRequested();
      if (m_stopRequested) {
        return;
      }
    }

    if (totalCount < 2) {
      std::cerr << "Clip file " << clipPath << " has too few epochs, skipping" << std::endl;
      continue;
    }

    // End timing image creation
    auto endImageGeneration = std::chrono::high_resolution_clock::now();
    auto imageGenerationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endImageGeneration - startImageGeneration).count();
    std::cout << "Generated " << overlayMaker.getImageQueue().size() << " frames in "
        << imageGenerationTime << "ms ("
        << (!overlayMaker.getImageQueue().empty() ? imageGenerationTime / overlayMaker.getImageQueue().size() : 0)
        << "ms per frame)" << std::endl;

    auto presentationDuration = float(endTimeMs - startTimeMs) / 1000;
    float overlaysFps = float(totalCount) / presentationDuration;


    // Convert startTimeMs from UTC to local so it matches time shown in the instrument cell
    QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(startTimeMs));
    QTimeZone tz = time.timeZone();
    int offsetSec = tz.offsetFromUtc(time);


    FFMpeg ffmpeg;
    std::cout << "Initializing encoder..." << std::endl;
    bool initSuccess = ffmpeg.initializeEncoder(
      clipFulPathName.string(),
      overlayMaker.getWidth(),
      overlayMaker.getHeight(),
      overlaysFps,
      startTimeMs + offsetSec * 1000
    );

    // Start timing encoding process
    auto startEncoding = std::chrono::high_resolution_clock::now();
    if (!initSuccess) {
      std::cerr << "Failed to initialize encoder for " << clipFulPathName.string() << std::endl;
      return;
    }

    uint64_t clipDurationMs = presentationDuration * 1000;
    EncodingProgressListener
        progressListener("Creating " + pilotClipName.string(), clipDurationMs, m_rProgressListener);

    // Encode all images from the queue
    std::cout << "Starting encoding of " << overlayMaker.getImageQueue().size() << " frames..." << std::endl;
    bool encodingSuccess = ffmpeg.encodeQImageSequence(overlayMaker.getImageQueue(), overlaysFps, progressListener);

    // End timing encoding process
    auto endEncoding = std::chrono::high_resolution_clock::now();
    auto encodingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endEncoding - startEncoding).count();
    std::cout << "Encoded " << overlayMaker.getImageQueue().size() << " frames in "
        << encodingTime << "ms ("
        << (overlayMaker.getImageQueue().size() > 0 ? encodingTime / overlayMaker.getImageQueue().size() : 0)
        << "ms per frame)" << std::endl;

    if (!encodingSuccess) {
      std::cerr << "Failed to encode image sequence" << std::endl;
      m_stopRequested = true;
      return;
    }

    m_stopRequested = progressListener.isStopRequested();

    if (m_stopRequested) {
      return;
    }
  }
}

void MovieProducer::produce() {
  int raceCount = 0;

  Caffeine caffeine; // Prevent Mac from going to sleep while this variable is in scope

  m_stopRequested = false;
  for (RaceData *race: m_RaceDataList) {
    std::cout << "Producing race " << race->getName() << std::endl;
    std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / ("race" + std::to_string(raceCount));
    std::cout << "Creating race folder " << raceFolder << std::endl;
    std::filesystem::create_directories(raceFolder);

    // Create description.txt file in that folder
    std::filesystem::path descFileName = raceFolder / "description.txt";
    std::ofstream df(descFileName, std::ios::out);

    // Augment charter list with performance chapters
    std::list<Chapter *> chapterList = race->getChapters();


    int movieWidth = 1920;
    int movieHeight = 1080;

    if (!m_rGoProClipInfoList.empty()) {
      movieWidth = m_rGoProClipInfoList.front().getWidth();
      movieHeight = m_rGoProClipInfoList.front().getHeight();
    }

    int target_ovl_width = movieWidth;
    int target_ovl_height = 128;

    int instr_ovl_width = movieWidth;
    int instrOvlHeight = 128;

    int polar_ovl_width = 400;
    int polar_ovl_height = polar_ovl_width;

    int rudder_ovl_width = 400;
    int rudder_ovl_height = 200;

    int startIdx = (int) chapterList.front()->getStartIdx();
    int endIdx = (int) chapterList.back()->getEndIdx();

    int timerHeight = 256;
    int timerWidth = 360;
    int timerX = -1; // Right aligned

    int perf_ovl_width = 200;
    int perf_ovl_height = 200;
    int perfPadX = 20;
    int perfPadY = 20;
    int perfX = perfPadX;
    int perfY = movieHeight - instrOvlHeight - target_ovl_height - perf_ovl_height - perfPadY;

    TargetsOverlayMaker targetsOverlayMaker(m_polars, m_rInstrDataVector,
                                            target_ovl_width, target_ovl_height,
                                            0, movieHeight - instrOvlHeight - target_ovl_height,
                                            startIdx, endIdx);

    InstrOverlayMaker instrOverlayMaker(m_rInstrDataVector, instr_ovl_width, instrOvlHeight, 0,
                                        movieHeight - instrOvlHeight);

    PolarOverlayMaker polarOverlayMaker(m_polars, m_rInstrDataVector, polar_ovl_width, polar_ovl_height, 0, 0);

    RudderOverlayMaker rudderOverlayMaker(rudder_ovl_width, rudder_ovl_height, 0, polar_ovl_height);

    StartTimerOverlayMaker startTimerOverlayMaker(m_polars, m_rInstrDataVector, timerWidth, timerHeight, timerX, 0);

    PerformanceOverlayMaker performanceOverlayMaker(m_rPerformanceVector,
                                                    perf_ovl_width, perf_ovl_height, perfX, perfY);


    OverlayMaker overlayMaker(raceFolder, movieWidth, movieHeight);

    overlayMaker.addOverlayElement(instrOverlayMaker);
    //        overlayMaker.addOverlayElement(targetsOverlayMaker);
    overlayMaker.addOverlayElement(polarOverlayMaker);
    overlayMaker.addOverlayElement(rudderOverlayMaker);
    overlayMaker.addOverlayElement(startTimerOverlayMaker);
    //        overlayMaker.addOverlayElement(performanceOverlayMaker);

    int chapterCount = 0;
    int numChapters = chapterList.size();
    m_totalRaceDuration = 0;
    std::list<std::string> chapterClips;
    for (Chapter *chapter: chapterList) {
      // Add entry to the description file
      auto sec = m_totalRaceDuration / 1000;
      makeChapterDescription(df, chapter, sec);

      chapterCount++;
      std::string chapterClipName = produceChapter(overlayMaker, *chapter, chapterCount, numChapters);
      chapterClips.push_back(chapterClipName);
      if (m_stopRequested) {
        return;
      }
    }
    if (m_stopRequested) {
      return;
    }

    raceCount++;
    df.close();
  }
}

void MovieProducer::makeChapterDescription(std::ofstream &df, const Chapter *chapter, uint64_t sec) {
  // Time stamp
  auto min = sec / 60;
  sec = sec % 60;
  df << std::setw(2) << std::setfill('0') << min << ":" << std::setw(2) << std::setfill('0') << sec;

  // Name
  df << " " << chapter->getName();

  if (chapter->getChapterType() == ChapterTypes::MARK_ROUNDING) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter->getStartIdx()].utc.getUnixTimeMs();
    df << " (";
    // Median TWS
    InstrumentInput median = InstrumentInput::median(m_rInstrDataVector.begin() + long(chapter->getStartIdx()),
                                                     m_rInstrDataVector.begin() + long(chapter->getEndIdx()));

    if (median.tws.isValid(startUtcMs)) {
      df << "TWS " << std::fixed << std::setprecision(0) << median.tws.getKnots() << " kts, ";
    }

    // Distance sailed
    uint32_t ulDistSailedMeters = m_rInstrDataVector[chapter->getEndIdx()].log.getMeters()
                                  - m_rInstrDataVector[chapter->getStartIdx()].log.getMeters();
    df << ulDistSailedMeters << " meters, ";

    // Time sailed
    uint64_t ulTimeSailedSeconds = (m_rInstrDataVector[chapter->getEndIdx()].utc.getUnixTimeMs()
                                    - m_rInstrDataVector[chapter->getStartIdx()].utc.getUnixTimeMs()) / 1000;

    df << ulTimeSailedSeconds << " seconds";

    df << ")";
  } else if (chapter->getChapterType() == ChapterTypes::TACK_GYBE) {
    Performance perf = m_rPerformanceVector[m_rInstrDataVector[chapter->getEndIdx()].utc.getUnixTimeMs()];
    df << " (";
    auto legTimeLostToTargetSec = perf.legTimeLostToTargetSec;
    auto legDistLostToTargetMeters = perf.legDistLostToTargetMeters;
    if (perf.legTimeLostToTargetSec > 0) {
      df << "Lost ";
    } else {
      legTimeLostToTargetSec = -legTimeLostToTargetSec;
      legDistLostToTargetMeters = -legDistLostToTargetMeters;
      df << "Gained ";
    }
    df << "time: " << std::fixed << std::setprecision(0) << legTimeLostToTargetSec << " sec, ";
    df << "distance: " << std::fixed << std::setprecision(0) << legDistLostToTargetMeters << " meters";
    df << ")";
  }

  df << std::endl;
}

bool checkProResAvailable() {
  const AVCodec *codec = avcodec_find_encoder_by_name("prores_ks");
  if (!codec) {
    std::cerr << "ProRes codec not available. Please install or verify your FFmpeg build includes ProRes support." <<
        std::endl;
    return false;
  }
  return true;
}

std::string MovieProducer::produceChapter(OverlayMaker &overlayMaker, Chapter &chapter, int chapterNum,
                                          int totalChapters) {
  uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
  uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

  std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

  std::string chapterWithNum = "Chapter " + chapter.getName() + " (" + std::to_string(chapterNum) + "/" +
                               std::to_string(totalChapters) + ")";


  auto duration = float(stopUtcMs - startUtcMs) / 1000;
  float presentationDuration = duration;
  bool changeDuration = false;
  //    if( chapter.getChapterType() == ChapterTypes::ChapterType::SPEED_PERFORMANCE) {
  //        presentationDuration = 60;
  //        changeDuration = true;
  //    }

  m_totalRaceDuration += presentationDuration * 1000;

  std::list<ClipFragment> goProclipFragments;

  // Create the input list
  findGoProClipFragments(goProclipFragments, startUtcMs, stopUtcMs);

  int prevProgress = -1;
  int totalCount = int(chapter.getEndIdx() - chapter.getStartIdx());
  // determine overlays framerate
  float overlaysFps = float(totalCount) / presentationDuration;
  u_int64_t ulEpochStep = 1;
  if (overlaysFps > 10) {
    // We  don't want to have too many frames
    // let's skip some epochs
    int targetFps = 10;
    ulEpochStep = totalCount / u_int64_t(presentationDuration) / targetFps;
    // Recompute overlay FPS
    overlaysFps = float(totalCount / ulEpochStep) / presentationDuration;
  }

  if (chapter.getChapterType() == ChapterTypes::ChapterType::SPEED_PERFORMANCE && totalCount > 1200) {
    // We don't want to have too many frames
    // let's skip some epochs
    ulEpochStep = totalCount / 1000;
    // Recompute overlay FPS
    overlaysFps = float(totalCount / ulEpochStep) / presentationDuration;
  }

  std::list<InstrumentInput> chapterEpochs;
  for (auto epochIdx = chapter.getStartIdx(); epochIdx < chapter.getEndIdx(); epochIdx += ulEpochStep) {
    InstrumentInput epoch;
    if (ulEpochStep == 1) {
      epoch = m_rInstrDataVector[epochIdx];
    } else {
      epoch = InstrumentInput::median(m_rInstrDataVector.begin() + epochIdx,
                                      m_rInstrDataVector.begin() + epochIdx + ulEpochStep);
    }
    chapterEpochs.push_back(epoch);
  }

  std::filesystem::path chapterFolder = overlayMaker.setChapter(chapter, chapterEpochs);
  // This call creates new chapter name
  std::ostringstream oss;
  oss << "CHAPTER-OVERLAY-" << chapter.getUuid().toStdString() << ".mov";

  std::filesystem::path clipFulPathName = chapterFolder.parent_path() / oss.str();


  chapter.setChapterClipFileName(clipFulPathName.native());

  // Check if the chapter already exists
  // Don't return until the m_totalRaceDuration is updated
  std::filesystem::path summaryFile = chapterFolder / "summary.csv";
  if (isClipCacheValid(summaryFile, chapter)) {
    return clipFulPathName;
  }

  // Start timing image creation
  auto startImageGeneration = std::chrono::high_resolution_clock::now();

  int count = 0;
  for (auto &epoch: chapterEpochs) {
    if (m_stopRequested) {
      return "";
    }

    overlayMaker.addEpoch(epoch);

    int progress = count * 100 / totalCount;
    if (progress != prevProgress) {
      m_rProgressListener.progress(chapterWithNum + " Overlay", progress);
      prevProgress = progress;
    }
    count++;
  }

  // End timing image creation
  auto endImageGeneration = std::chrono::high_resolution_clock::now();
  auto imageGenerationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
    endImageGeneration - startImageGeneration).count();
  std::cout << "Generated " << overlayMaker.getImageQueue().size() << " frames in "
      << imageGenerationTime << "ms ("
      << (overlayMaker.getImageQueue().size() > 0 ? imageGenerationTime / overlayMaker.getImageQueue().size() : 0)
      << "ms per frame)" << std::endl;


  // Initialize FFmpeg encoder
  FFMpeg ffmpeg;
  float durationScale = presentationDuration / duration;
  ffmpeg.setBackgroundClip(&goProclipFragments, changeDuration, durationScale);

  // Check transparency support
  bool transparencyNeeded = true; // Assume we need transparency for overlays
  if (transparencyNeeded && !checkProResAvailable()) {
    std::cerr << "ProRes not available, falling back to H.264 without transparency" << std::endl;
    transparencyNeeded = false;
  }

  // Start timing encoding process
  auto startEncoding = std::chrono::high_resolution_clock::now();

  // Initialize the encoder and check for success
  std::cout << "Initializing encoder..." << std::endl;
  uint64_t startTimeMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();

  // Convert startTimeMs from UTC to local so it matches time shown in the instrument cell
  QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(startTimeMs));
  QTimeZone tz = time.timeZone();
  int offsetSec = tz.offsetFromUtc(time);

  bool initSuccess = ffmpeg.initializeEncoder(
    clipFulPathName.string(),
    overlayMaker.getWidth(),
    overlayMaker.getHeight(),
    overlaysFps,
    startTimeMs + offsetSec * 1000
  );

  if (!initSuccess) {
    std::cerr << "Failed to initialize encoder for " << clipFulPathName.string() << std::endl;
    return "";
  }

  uint64_t clipDurationMs = presentationDuration * 1000;
  EncodingProgressListener progressListener(chapterWithNum, clipDurationMs, m_rProgressListener);

  // Encode all images from the queue
  std::cout << "Starting encoding of " << overlayMaker.getImageQueue().size() << " frames..." << std::endl;
  bool encodingSuccess = ffmpeg.encodeQImageSequence(overlayMaker.getImageQueue(), overlaysFps, progressListener,
                                                     "Title");
  overlayMaker.clearImageQueue();

  // End timing encoding process
  auto endEncoding = std::chrono::high_resolution_clock::now();
  auto encodingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
    endEncoding - startEncoding).count();
  std::cout << "Encoded " << overlayMaker.getImageQueue().size() << " frames in "
      << encodingTime << "ms ("
      << (overlayMaker.getImageQueue().size() > 0 ? encodingTime / overlayMaker.getImageQueue().size() : 0)
      << "ms per frame)" << std::endl;

  if (!encodingSuccess) {
    std::cerr << "Failed to encode image sequence" << std::endl;
    m_stopRequested = true;
    return "";
  }

  m_stopRequested = progressListener.isStopRequested();

  if (m_stopRequested) {
    return "";
  }

  makeSummaryFile(summaryFile, chapter);

  return clipFulPathName;
}

void MovieProducer::findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs,
                                           uint64_t stopUtcMs) {
  int64_t inTime = -1;
  int64_t outTime = -1;
  // Determine list of GOPRO clips and their in and out points for given clip
  for (auto firstClip = m_rGoProClipInfoList.begin(); firstClip != m_rGoProClipInfoList.end(); firstClip++) {
    //        std::cout << "First clip " << firstClip->getFileName() << " " << firstClip->getClipStartUtcMs() << ":" << firstClip->getClipEndUtcMs() << std::endl;
    if (firstClip->getClipStartUtcMs() <= startUtcMs && startUtcMs <= firstClip->getClipEndUtcMs()) {
      // Chapter starts in this clip
      inTime = int64_t(startUtcMs - firstClip->getClipStartUtcMs());

      // Now find the clip containing the stop time
      for (auto clip = firstClip; clip != m_rGoProClipInfoList.end(); clip++) {
        //                std::cout << "clip " << clip->getFileName() << " " << clip->getClipStartUtcMs() << ":" << clip->getClipEndUtcMs() << std::endl;
        if (stopUtcMs <= clip->getClipEndUtcMs()) {
          // Last clip
          // Check for the corner case when the stop_utc falls inbetween start_utc of the previous clip
          // and start_utc of this one
          if (stopUtcMs <= clip->getClipStartUtcMs()) {
            break;
          }
          outTime = int64_t(stopUtcMs - clip->getClipStartUtcMs());
          clipFragments.emplace_back(inTime, outTime, clip->getFileName(), clip->getWidth(), clip->getHeight());
          break;
        } else {
          // The time interval spans to subsequent clips
          outTime = -1; // Till the end of clip
          clipFragments.emplace_back(inTime, outTime, clip->getFileName(), clip->getWidth(), clip->getHeight());
          inTime = -1; // Start from the beginning of the next clip
        }
      }
      break;
    }
  }
}

void MovieProducer::makeRaceVideo(const std::filesystem::path &raceFolder, std::list<std::string> &chaptersList) {
  std::filesystem::path outMoviePath = raceFolder / "race.mp4";
  EncodingProgressListener progressListener("Race ", m_totalRaceDuration, m_rProgressListener);
  FFMpeg::joinChapters(chaptersList, outMoviePath.native(), progressListener);
}

bool MovieProducer::isClipCacheValid(const std::filesystem::path &summaryFile, Chapter &chapter) {
  // Read the summary file
  if (!std::filesystem::is_regular_file(summaryFile)) {
    return false;
  }

  std::ifstream sf(summaryFile, std::ios::in);
  std::string line;
  std::getline(sf, line);
  std::istringstream ss(line);
  std::string item;

  std::getline(ss, item, ',');
  if (item != chapter.getUuid().toStdString()) {
    std::cout << "clip " << chapter.getName() << " uuid " << chapter.getUuid().toStdString() << " has changed " <<
        std::endl;
    return false;
  }

  std::getline(ss, item, ',');
  int startIdx = std::stoi(item);
  if (startIdx != chapter.getStartIdx()) {
    std::cout << "clip " << chapter.getName() << " startIdx " << chapter.getStartIdx() << " has changed " << std::endl;
    return false;
  }

  std::getline(ss, item, ',');
  int endIdx = std::stoi(item);
  if (endIdx != chapter.getEndIdx()) {
    std::cout << "clip " << chapter.getName() << " endIdx " << chapter.getEndIdx() << " has changed " << std::endl;
    return false;
  }

  std::getline(ss, item, ',');
  int chapterType = std::stoi(item);
  if (chapterType != chapter.getChapterType()) {
    std::cout << "clip " << chapter.getName() << " type " << chapter.getChapterType() << " has changed " << std::endl;
    return false;
  }

  std::cout << "clip " << chapter.getName() << chapter.getUuid().toStdString() << " is still the same " << std::endl;
  return true;
}

void MovieProducer::makeSummaryFile(const std::filesystem::path &summaryFile, const Chapter &chapter) {
  std::ostringstream oss;
  oss << chapter.getUuid().toStdString() << "," << chapter.getStartIdx() << "," << chapter.getEndIdx()
      << "," << chapter.getChapterType() << "," << chapter.getName() << std::endl;
  // Write the summary file
  std::ofstream sf(summaryFile, std::ios::out);
  sf << oss.str();
}


bool EncodingProgressListener::ffmpegProgress(uint64_t msEncoded) {
  int progress = int(msEncoded * 100 / m_totalDurationMs);
  if (m_prevPercent != progress) {
    uint64_t secEncoded = msEncoded / 1000;
    uint64_t seconds = secEncoded / 60;
    uint64_t minutes = secEncoded % 60;

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << seconds << ":" << std::setw(2) << std::setfill('0') << minutes;

    m_rProgressListener.progress(m_prefix + " " + oss.str() + " encoded", progress);
    m_stopRequested = m_rProgressListener.stopRequested();
    return m_stopRequested;
  } else {
    return false;
  }
}
