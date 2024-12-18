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

MovieProducer::MovieProducer(const std::string &path, const std::string &polarPath, std::list<GoProClipInfo> &clipsList,
                             std::vector<InstrumentInput> &instrDataVector,
                             std::map<uint64_t, Performance> &performanceVector,
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

    Caffeine caffeine;  // Prevent Mac from going to sleep while this variable is in scope

    m_stopRequested = false;
    for(RaceData *race: m_RaceDataList){
        std::cout << "Producing race " << race->getName() << std::endl;
        std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / ("race" +  std::to_string(raceCount));
        std::cout << "Creating race folder " << raceFolder << std::endl;
        std::filesystem::create_directories(raceFolder);

        // Create description.txt file in that folder
        std::filesystem::path descFileName = raceFolder / "description.txt";
        std::ofstream df (descFileName, std::ios::out);

        // Augment charter list with performance chapters
        std::list<Chapter *> chapterList = race->getChapters();


        int movieWidth = 1920;
        int movieHeight = 1080;

        if ( !m_rGoProClipInfoList.empty() ){
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

        int startIdx = (int)chapterList.front()->getStartIdx();
        int endIdx = (int)chapterList.back()->getEndIdx();

        int timerHeight = 256;
        int timerWidth = 320;
        int timerX = -1;  // Right aligned

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

        InstrOverlayMaker instrOverlayMaker(m_rInstrDataVector, instr_ovl_width, instrOvlHeight, 0, movieHeight - instrOvlHeight);

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
        overlayMaker.addOverlayElement(performanceOverlayMaker);

        int chapterCount = 0;
        int numChapters = chapterList.size();
        m_totalRaceDuration = 0;
        std::list<std::string> chapterClips;
        for( Chapter *chapter: chapterList){

            // Add entry to the description file
            auto sec = m_totalRaceDuration / 1000;
            makeChapterDescription(df, chapter, sec);

            chapterCount ++;
            std::string chapterClipName = produceChapter(overlayMaker, *chapter, chapterCount, numChapters);
            chapterClips.push_back(chapterClipName);
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

void MovieProducer::makeChapterDescription(std::ofstream &df, const Chapter *chapter, uint64_t sec) {

    // Time stamp
    auto min = sec / 60;
    sec = sec % 60;
    df <<  std::setw(2) << std::setfill('0') << min << ":" << std::setw(2) << std::setfill('0') << sec;

    // Name
    df << " " << chapter->getName();

    if( chapter->getChapterType() == ChapterTypes::MARK_ROUNDING ) {
        uint64_t startUtcMs = m_rInstrDataVector[chapter->getStartIdx()].utc.getUnixTimeMs();
        df << " (";
        // Median TWS
        InstrumentInput median = InstrumentInput::median(m_rInstrDataVector.begin() + long(chapter->getStartIdx()),
                                        m_rInstrDataVector.begin() + long(chapter->getEndIdx()));

        if ( median.tws.isValid(startUtcMs) ){
            df << "TWS " << std::fixed << std::setprecision(0) << median.tws.getKnots() << " kts, ";
        }

        // Distance sailed
        uint32_t ulDistSailedMeters = m_rInstrDataVector[chapter->getEndIdx()].log.getMeters()
                                      - m_rInstrDataVector[chapter->getStartIdx()].log.getMeters();
        df <<  ulDistSailedMeters << " meters, ";

        // Time sailed
        uint64_t ulTimeSailedSeconds = (m_rInstrDataVector[chapter->getEndIdx()].utc.getUnixTimeMs()
                                        - m_rInstrDataVector[chapter->getStartIdx()].utc.getUnixTimeMs()) / 1000;

        df << ulTimeSailedSeconds << " seconds";

        df << ")";
    }else if ( chapter->getChapterType() == ChapterTypes::TACK_GYBE  ){
        Performance perf = m_rPerformanceVector[m_rInstrDataVector[chapter->getEndIdx()].utc.getUnixTimeMs()];
        df << " (";
            auto legTimeLostToTargetSec = perf.legTimeLostToTargetSec;
            auto legDistLostToTargetMeters = perf.legDistLostToTargetMeters;
            if ( perf.legTimeLostToTargetSec > 0 ){
                df << "Lost ";
            }else{
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

std::string MovieProducer::produceChapter(OverlayMaker &overlayMaker, Chapter &chapter, int chapterNum, int totalChapters) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

    std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

    std::string chapterWithNum = "Chapter " +  chapter.getName() + " (" + std::to_string(chapterNum) + "/"  + std::to_string(totalChapters)  + ")";


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
    if ( overlaysFps > 10 ){
        // We  don't want to have too many frames
        // let's skip some epochs
        int targetFps = 10;
        ulEpochStep = totalCount / u_int64_t(presentationDuration) / targetFps;
        // Recompute overlay FPS
        overlaysFps = float(totalCount / ulEpochStep) / presentationDuration;
    }

    if( chapter.getChapterType() == ChapterTypes::ChapterType::SPEED_PERFORMANCE &&  totalCount > 1200 ){
        // We don't want to have too many frames
        // let's skip some epochs
        ulEpochStep = totalCount / 1000;
        // Recompute overlay FPS
        overlaysFps = float(totalCount / ulEpochStep) / presentationDuration;
    }

    std::list<InstrumentInput> chapterEpochs;
    for(auto epochIdx = chapter.getStartIdx(); epochIdx < chapter.getEndIdx(); epochIdx += ulEpochStep) {

        InstrumentInput epoch;
        if (ulEpochStep == 1) {
            epoch = m_rInstrDataVector[epochIdx];
        } else {
            epoch = InstrumentInput::median(m_rInstrDataVector.begin() + epochIdx,
                                            m_rInstrDataVector.begin() + epochIdx + ulEpochStep);
        }
        chapterEpochs.push_back(epoch);
    }

    std::filesystem::path chapterFolder = overlayMaker.setChapter(chapter, chapterEpochs);  // This call creates new chapter name
    std::ostringstream oss;
    oss << "CHAPTER-OVERLAY-" << chapter.getUuid().toStdString() << ".MOV";

    std::filesystem::path clipFulPathName = chapterFolder.parent_path()  / oss.str() ;
    chapter.setChapterClipFileName(clipFulPathName.native());

    // Check if the chapter already exists
    // Don't return until the m_totalRaceDuration is updated
    std::filesystem::path summaryFile = chapterFolder / "summary.csv";
    if ( isClipCacheValid(summaryFile, chapter) ){
        return clipFulPathName;
    }

    int count = 0;
    for(auto &epoch: chapterEpochs){

        if ( m_stopRequested ){
            return "";
        }

        overlayMaker.addEpoch(epoch);

        int progress = count * 100 / totalCount;
        if ( progress != prevProgress){
            m_rProgressListener.progress(chapterWithNum + " Overlay", progress);
            prevProgress = progress;
        }
        count ++;
    }

    FFMpeg ffmpeg;
    float durationScale = presentationDuration / duration ;
    ffmpeg.setBackgroundClip(&goProclipFragments, changeDuration, durationScale);

    // Add  overlay
    ffmpeg.addOverlayPngSequence(0, 0, overlaysFps, chapterFolder,
                                 OverlayMaker::getFileNamePattern(chapter));

    uint64_t  clipDurationMs = presentationDuration * 1000;
    EncodingProgressListener progressListener(chapterWithNum, clipDurationMs, m_rProgressListener);

    ffmpeg.makeClip(clipFulPathName, progressListener);
    m_stopRequested = progressListener.isStopRequested();

    if ( m_stopRequested ){
        return "";
    }

    makeSummaryFile(summaryFile, chapter);

    return clipFulPathName;
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

bool MovieProducer::isClipCacheValid(const std::filesystem::path &summaryFile, Chapter &chapter) {

    // Read the summary file
    if ( !std::filesystem::is_regular_file(summaryFile) ){
        return false;
    }

    std::ifstream sf(summaryFile, std::ios::in);
    std::string line;
    std::getline(sf, line);
    std::istringstream ss(line);
    std::string item;

    std::getline(ss, item, ',');
    if ( item != chapter.getUuid().toStdString() ){
        std::cout << "clip " << chapter.getName() << " uuid "  << chapter.getUuid().toStdString() << " has changed " << std::endl;
        return false;
    }

    std::getline(ss, item, ',');
    int startIdx = std::stoi(item);
    if ( startIdx != chapter.getStartIdx() ){
        std::cout << "clip " << chapter.getName() << " startIdx " << chapter.getStartIdx() << " has changed " << std::endl;
        return false;
    }

    std::getline(ss, item, ',');
    int endIdx = std::stoi(item);
    if ( endIdx != chapter.getEndIdx() ){
        std::cout << "clip " << chapter.getName() << " endIdx "  << chapter.getEndIdx() << " has changed " << std::endl;
        return false;
    }

    std::getline(ss, item, ',');
    int chapterType = std::stoi(item);
    if ( chapterType != chapter.getChapterType() ){
        std::cout << "clip " << chapter.getName() << " type "  << chapter.getChapterType() << " has changed " << std::endl;
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

