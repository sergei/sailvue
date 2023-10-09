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

MovieProducer::MovieProducer(const std::string &path, const std::string &polarPath, std::list<GoProClipInfo> &clipsList,
                             std::vector<InstrumentInput> &instrDataVector,
                             std::vector<Performance> &performanceVector,
                             std::list<RaceData *> &raceList,
                             IProgressListener &rProgressListener)
:m_moviePath(path)
,m_rGoProClipInfoList(clipsList)
,m_rInstrDataVector(instrDataVector)
,m_rPerformanceVector(performanceVector)
,m_RaceDataList(raceList)
,m_rProgressListener(rProgressListener)
{
    m_polars.loadPolar(polarPath);
}

void MovieProducer::produce() {
    int raceCount = 0;
    auto ignoreCache = false;

    Caffeine caffeine;  // Prevent Mac from going to sleep while this variable is in scope

    m_stopRequested = false;
    for(RaceData *race: m_RaceDataList){
        std::cout << "Producing race " << race->getName() << std::endl;
        std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / ("race" +  std::to_string(raceCount));
        std::filesystem::create_directories(raceFolder);

        // Create description.txt file in that folder
        std::filesystem::path descFileName = raceFolder / "description.txt";
        std::ofstream df (descFileName, std::ios::out);

        // Augment charter list with performance chapters
        std::list<Chapter *> chapterList = race->getChapters();


        int movieWidth = m_rGoProClipInfoList.front().getWidth();

        int target_ovl_width = movieWidth;
        int target_ovl_height = 128;

        int startIdx = (int)chapterList.front()->getStartIdx();
        int endIdx = (int)chapterList.back()->getEndIdx();
        TargetsOverlayMaker targetsOverlayMaker(m_polars, m_rInstrDataVector,
                                                target_ovl_width, target_ovl_height,
                                                startIdx, endIdx, ignoreCache);

        int chapterCount = 0;
        m_totalRaceDuration = 0;
        std::list<std::string> chapterClips;
        for( Chapter *chapter: chapterList){
            // Add entry to the description file
            auto sec = m_totalRaceDuration / 1000;
            auto min = sec / 60;
            sec = sec % 60;
            std::ostringstream oss;
            df <<  std::setw(2) << std::setfill('0') << min << ":" << std::setw(2) << std::setfill('0') << sec;
            df << " " << chapter->getName();
            df << std::endl;

            std::filesystem::path chapterFolder = raceFolder / ("chapter" +  std::to_string(chapterCount));
            std::filesystem::create_directories(chapterFolder);
            std::string chapterClipName = produceChapter(targetsOverlayMaker, *chapter, chapterFolder,
                                                         movieWidth, target_ovl_height, ignoreCache);
            chapterClips.push_back(chapterClipName);
            chapterCount ++;
            if ( m_stopRequested ){
                return;
            }
        }
        if ( m_stopRequested ){
            return;
        }
        makeRaceVideo(raceFolder, chapterClips);
        raceCount ++;
        df.close();
    }
}

std::string MovieProducer::produceChapter(TargetsOverlayMaker &targetsOverlayMaker, Chapter &chapter,
                                          std::filesystem::path &folder, int movieWidth, int target_ovl_height,  bool ignoreCache) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

    std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

    std::filesystem::path outMoviePath = folder / "chapter.mp4";

    auto duration = float(stopUtcMs - startUtcMs) / 1000;
    float presentationDuration;
    bool changeDuration = false;
    if( chapter.getChapterType() == ChapterTypes::ChapterType::SPEED_PERFORMANCE) {
        presentationDuration = 60;
        changeDuration = true;
    }else{
        presentationDuration = duration;
    }

    m_totalRaceDuration += presentationDuration * 1000;

    // Check if the output file already exists
    if ( std::filesystem::is_regular_file(outMoviePath) && !ignoreCache){
        std::cout << "Chapter " << chapter.getName() << " already exists" << std::endl;
        return outMoviePath.native();
    }

    std::list<ClipFragment> goProclipFragments;

    // Create the input list
    findGoProClipFragments(goProclipFragments, startUtcMs, stopUtcMs);

    // Create chapter overlay clip

    int instr_ovl_width = movieWidth;
    int polar_ovl_width = 400;
    int height = goProclipFragments.front().height;
    int instrOvlHeight = 128;
    int timerHeight = 128;

    int perf_ovl_width = 200;
    int perf_ovl_height = 200;
    int perfPadX = 20;
    int perfPadY = 20;

    std::filesystem::path instrOverlayPath = folder / "instr_overlay";
    InstrOverlayMaker instrOverlayMaker(instrOverlayPath, instr_ovl_width, instrOvlHeight, ignoreCache);

    std::filesystem::path polarOverlayPath = folder / "polar_overlay";
    PolarOverlayMaker polarOverlayMaker(m_polars, m_rInstrDataVector, polarOverlayPath, polar_ovl_width,
                                        (int)chapter.getStartIdx(), (int)chapter.getEndIdx(), ignoreCache);


    std::filesystem::path startTimerOverlayPath = folder / "timer_overlay";
    StartTimerOverlayMaker startTimerOverlayMaker(startTimerOverlayPath, timerHeight, ignoreCache);


    std::filesystem::path targetsOverlayPath = folder / "targets_overlay";
    targetsOverlayMaker.addChapter(targetsOverlayPath, (int)chapter.getStartIdx(), (int)chapter.getEndIdx());

    std::filesystem::path performanceOverlayPath = folder / "perf_overlay";
    PerformanceOverlayMaker performanceOverlayMaker(m_rPerformanceVector, performanceOverlayPath,
                                                    perf_ovl_width, perf_ovl_height, ignoreCache);


    int timerX = goProclipFragments.front().width - startTimerOverlayMaker.getWidth();

    int count = 0;
    int prevProgress = -1;
    int totalCount = int(chapter.getEndIdx() - chapter.getStartIdx());

    for(auto instrData = m_rInstrDataVector.begin() + (long)chapter.getStartIdx();
        instrData != m_rInstrDataVector.begin() + (long)chapter.getEndIdx(); instrData++){
        char acFileName[80];
        sprintf(acFileName, INSTR_OVL_FILE_PAT, count);
        std::string instrOvlFileName = acFileName;
        instrOverlayMaker.addEpoch(instrOvlFileName, *instrData);

        sprintf(acFileName, POLAR_OVL_FILE_PAT, count);
        std::string polarOvlFileName = acFileName;
        polarOverlayMaker.addEpoch(polarOvlFileName, (int)chapter.getStartIdx() + count);

        sprintf(acFileName, TARGET_OVL_FILE_PAT, count);
        std::string targetOvlFileName = acFileName;
        targetsOverlayMaker.addEpoch(targetOvlFileName, (int)chapter.getStartIdx() + count);

        if ( chapter.getChapterType() == ChapterTypes::ChapterType::START ){
            sprintf(acFileName, TIMER_OVL_FILE_PAT, count);
            std::string timerOvlFileName = acFileName;
            uint64_t gunUtcTimeMs = m_rInstrDataVector[chapter.getGunIdx()].utc.getUnixTimeMs();
            startTimerOverlayMaker.addEpoch(timerOvlFileName, gunUtcTimeMs,  *instrData);
        }

        sprintf(acFileName, PERF_OVL_FILE_PAT, count);
        std::string perfOvlFileName = acFileName;
        performanceOverlayMaker.addEpoch(perfOvlFileName, (int)chapter.getStartIdx() + count);

        int progress = count * 100 / totalCount;
        if ( progress != prevProgress){
            m_rProgressListener.progress(chapter.getName() + " Overlay", progress);
            prevProgress = progress;
        }
        count ++;
    }


    // determine overlays framerate
    float overlaysFps = float(count) / presentationDuration;

    FFMpeg ffmpeg;
    float durationScale = presentationDuration / duration ;
    ffmpeg.setBackgroundClip(&goProclipFragments, changeDuration, durationScale);

    // Add instruments overlay
    ffmpeg.addOverlayPngSequence(0, height - instrOvlHeight, overlaysFps, instrOverlayPath.native(), INSTR_OVL_FILE_PAT);
    // Add targets overlay
    ffmpeg.addOverlayPngSequence(0, height - instrOvlHeight - target_ovl_height, overlaysFps, targetsOverlayPath.native(), TARGET_OVL_FILE_PAT);

    if ( chapter.getChapterType() == ChapterTypes::ChapterType::START){ // Add start timer overlay
        ffmpeg.addOverlayPngSequence(timerX, 0, overlaysFps, startTimerOverlayPath.native(), TIMER_OVL_FILE_PAT);
    }

    // Add performance overlay
    ffmpeg.addOverlayPngSequence(perfPadX, height - instrOvlHeight - target_ovl_height - perf_ovl_height - perfPadY, overlaysFps, performanceOverlayPath.native(), PERF_OVL_FILE_PAT);

    // Add polar overlay
    ffmpeg.addOverlayPngSequence(0, 0, overlaysFps, polarOverlayPath.native(), POLAR_OVL_FILE_PAT);

    uint64_t  clipDurationMs = presentationDuration * 1000;
    EncodingProgressListener progressListener("Chapter " + chapter.getName(), clipDurationMs, m_rProgressListener);
    ffmpeg.makeClip(outMoviePath.native(), progressListener);
    m_stopRequested = progressListener.isStopRequested();

    return outMoviePath.native();
}

void MovieProducer::findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs, uint64_t stopUtcMs) {
    int64_t inTime = -1;
    int64_t outTime = -1;
    // Determine list of GOPRO clips and their in and out points for given clip
    for(auto firstClip= m_rGoProClipInfoList.begin(); firstClip != m_rGoProClipInfoList.end(); firstClip++){
//        std::cout << "First clip " << firstClip->getFileName() << " " << firstClip->getClipStartUtcMs() << ":" << firstClip->getClipEndUtcMs() << std::endl;
        if(firstClip->getClipStartUtcMs() <= startUtcMs && startUtcMs <= firstClip->getClipEndUtcMs() ) { // Chapter starts in this clip
            inTime = int64_t(startUtcMs - firstClip->getClipStartUtcMs());

            // Now find the clip containing the stop time
            for(auto clip = firstClip; clip != m_rGoProClipInfoList.end(); clip++){
//                std::cout << "clip " << clip->getFileName() << " " << clip->getClipStartUtcMs() << ":" << clip->getClipEndUtcMs() << std::endl;
                if (stopUtcMs <= clip->getClipEndUtcMs()){ // Last clip
                    // Check for the corner case when the stop_utc falls inbetween start_utc of the previous clip
                    // and start_utc of this one
                    if (stopUtcMs <= clip->getClipStartUtcMs()){
                        break;
                    }
                    outTime = int64_t(stopUtcMs - clip->getClipStartUtcMs());
                    clipFragments.emplace_back(inTime, outTime, clip->getFileName(), clip->getWidth(), clip->getHeight());
                    break;
                }else{ // The time interval spans to subsequent clips
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
    EncodingProgressListener progressListener("Race " , m_totalRaceDuration, m_rProgressListener);
    FFMpeg::joinChapters(chaptersList, outMoviePath.native(), progressListener);
}



bool EncodingProgressListener::ffmpegProgress(uint64_t msEncoded) {
    int progress = int(msEncoded * 100 / m_totalDurationMs);
    if ( m_prevPercent != progress){

        uint64_t secEncoded = msEncoded / 1000;
        uint64_t seconds = secEncoded / 60;
        uint64_t minutes = secEncoded % 60;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << seconds << ":" << std::setw(2) << std::setfill('0') << minutes;

        m_rProgressListener.progress( m_prefix + " " + oss.str() + " encoded", progress);
        m_stopRequested = m_rProgressListener.stopRequested();
        return m_stopRequested;
    }else{
        return false;
    }
}

