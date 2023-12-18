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


        int movieWidth = m_rGoProClipInfoList.front().getWidth();
        int movieHeight = m_rGoProClipInfoList.front().getHeight();

        int target_ovl_width = movieWidth;
        int target_ovl_height = 128;

        int instr_ovl_width = movieWidth;
        int instrOvlHeight = 128;

        int polar_ovl_width = 400;

        int startIdx = (int)chapterList.front()->getStartIdx();
        int endIdx = (int)chapterList.back()->getEndIdx();

        int timerHeight = 128;
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

        PolarOverlayMaker polarOverlayMaker(m_polars, m_rInstrDataVector, polar_ovl_width, polar_ovl_width, 0, 0);

        StartTimerOverlayMaker startTimerOverlayMaker(m_rInstrDataVector, timerHeight, timerHeight, timerX, 0);

        PerformanceOverlayMaker performanceOverlayMaker(m_rPerformanceVector,
                                                        perf_ovl_width, perf_ovl_height, perfX, perfY);


        OverlayMaker overlayMaker(raceFolder, movieWidth, movieHeight);

        overlayMaker.addOverlayElement(instrOverlayMaker);
        overlayMaker.addOverlayElement(targetsOverlayMaker);
        overlayMaker.addOverlayElement(polarOverlayMaker);
        overlayMaker.addOverlayElement(startTimerOverlayMaker);
        overlayMaker.addOverlayElement(performanceOverlayMaker);

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

            std::string chapterClipName = produceChapter(overlayMaker, *chapter);
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

std::string MovieProducer::produceChapter(OverlayMaker &overlayMaker, Chapter &chapter) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

    std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

    overlayMaker.setChapter(chapter);
    std::filesystem::path clipFulPathName = overlayMaker.getChapterFolder() / "clip.mp4";

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

    // Check if the chapter already exists
    // Don't return until the m_totalRaceDuration is updated
    if ( std::filesystem::is_regular_file(clipFulPathName) ){
        return clipFulPathName;
    }


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
        // TODO do some filtering before decimation
        int targetFps = 10;
        ulEpochStep = totalCount / u_int64_t(presentationDuration) / targetFps;
        // Recompute overlay FPS
        overlaysFps = float(totalCount / ulEpochStep) / presentationDuration;
    }

    int count = 0;
    for(auto epochIdx = chapter.getStartIdx(); epochIdx < chapter.getEndIdx(); epochIdx += ulEpochStep){

        if ( m_stopRequested ){
            return "";
        }

        overlayMaker.addEpoch(int(epochIdx));

        int progress = count * 100 / totalCount;
        if ( progress != prevProgress){
            m_rProgressListener.progress(chapter.getName() + " Overlay", progress);
            prevProgress = progress;
        }
        count ++;
    }


    FFMpeg ffmpeg;
    float durationScale = presentationDuration / duration ;
    ffmpeg.setBackgroundClip(&goProclipFragments, changeDuration, durationScale);

    // Add  overlay
    ffmpeg.addOverlayPngSequence(0, 0, overlaysFps, overlayMaker.getChapterFolder(),
                                 OverlayMaker::getFileNamePattern(chapter));

    uint64_t  clipDurationMs = presentationDuration * 1000;
    EncodingProgressListener progressListener("Chapter " + chapter.getName(), clipDurationMs, m_rProgressListener);

    ffmpeg.makeClip(clipFulPathName, progressListener);
    m_stopRequested = progressListener.isStopRequested();

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

