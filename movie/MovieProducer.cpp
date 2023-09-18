#include "MovieProducer.h"
#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "InstrOverlayMaker.h"
#include "PolarOverlayMaker.h"
#include "StartTimerOverlayMaker.h"

MovieProducer::MovieProducer(const std::string &path, std::list<GoProClipInfo> &clipsList,
                             std::vector<InstrumentInput> &instrDataVector, std::list<RaceData *> &raceList,
                             IProgressListener &rProgressListener)
:m_moviePath(path)
,m_rGoProClipInfoList(clipsList)
,m_rInstrDataVector(instrDataVector)
,m_RaceDataList(raceList)
,m_rProgressListener(rProgressListener)
{

}

void MovieProducer::produce() {
    int raceCount = 0;
    auto ignoreCache = false;

    m_stopRequested = false;
    for(RaceData *race: m_RaceDataList){
        std::cout << "Producing race " << race->getName() << std::endl;
        std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / ("race" +  std::to_string(raceCount));
        std::filesystem::create_directories(raceFolder);

        int chapterCount = 0;
        m_totalRaceDuration = 0;
        std::list<std::string> chapterClips;
        for( Chapter *chapter: race->getChapters()){
            std::filesystem::path chapterFolder = raceFolder / ("chapter" +  std::to_string(chapterCount));
            std::filesystem::create_directories(chapterFolder);
            std::string chapterClipName = produceChapter(*chapter, chapterFolder, ignoreCache);
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
    }
}

std::string MovieProducer::produceChapter(Chapter &chapter, std::filesystem::path &folder, bool ignoreCache) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

    std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

    std::filesystem::path outMoviePath = folder / "chapter.mp4";
    uint64_t  clipDurationMs = stopUtcMs - startUtcMs;
    m_totalRaceDuration += clipDurationMs;

    // Check if the output file already exists
    if ( std::filesystem::is_regular_file(outMoviePath) && !ignoreCache){
        std::cout << "Chapter " << chapter.getName() << " already exists" << std::endl;
        return outMoviePath.native();
    }

    std::list<ClipFragment> goProclipFragments;

    // Create the input list
    findGoProClipFragments(goProclipFragments, startUtcMs, stopUtcMs);

    // Create chapter overlay clip

    int instr_ovl_width = goProclipFragments.front().width;
    int polar_ovl_width = 400;
    int height = goProclipFragments.front().height;
    int instrOvlHeight = 128;
    int timerHeight = 128;

    std::filesystem::path instrOverlayPath = folder / "instr_overlay";
    InstrOverlayMaker instrOverlayMaker(instrOverlayPath, instr_ovl_width, instrOvlHeight, ignoreCache);

    std::filesystem::path polarOverlayPath = folder / "polar_overlay";
    PolarOverlayMaker polarOverlayMaker(m_rInstrDataVector, polarOverlayPath, polar_ovl_width,
                                        (int)chapter.getStartIdx(), (int)chapter.getEndIdx(), ignoreCache);


    std::filesystem::path startTimerOverlayPath = folder / "timer_overlay";
    StartTimerOverlayMaker startTimerOverlayMaker(startTimerOverlayPath, timerHeight, ignoreCache);

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

        if ( chapter.getChapterType() == ChapterType::START){
            sprintf(acFileName, TIMER_OVL_FILE_PAT, count);
            std::string timerOvlFileName = acFileName;
            uint64_t gunUtcTimeMs = m_rInstrDataVector[chapter.getGunIdx()].utc.getUnixTimeMs();
            startTimerOverlayMaker.addEpoch(timerOvlFileName, gunUtcTimeMs,  *instrData);
        }

        int progress = count * 100 / totalCount;
        if ( progress != prevProgress){
            m_rProgressListener.progress(chapter.getName() + "Overlay", progress);
            prevProgress = progress;
        }
        count ++;
    }

    // determine framerate
    int fps = (int)lround(float(count) / (float(stopUtcMs - startUtcMs) / 1000) );

    FFMpeg ffmpeg;
    ffmpeg.setBackgroundClip(&goProclipFragments);

    ffmpeg.addOverlayPngSequence(0, height - instrOvlHeight, fps, instrOverlayPath.native(), INSTR_OVL_FILE_PAT);

    if ( chapter.getChapterType() == ChapterType::START){
        ffmpeg.addOverlayPngSequence(timerX, 0, fps, startTimerOverlayPath.native(), TIMER_OVL_FILE_PAT);
    }
    ffmpeg.addOverlayPngSequence(0, 0, fps, polarOverlayPath.native(), POLAR_OVL_FILE_PAT);

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

