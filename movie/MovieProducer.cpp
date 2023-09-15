#include "MovieProducer.h"
#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "InstrOverlayMaker.h"
#include "PolarOverlayMaker.h"

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
    for(RaceData *race: m_RaceDataList){
        std::cout << "Producing race " << race->getName() << std::endl;
        std::filesystem::path raceFolder = std::filesystem::path(m_moviePath) / ("race" +  std::to_string(raceCount));
        std::filesystem::create_directories(raceFolder);

        int chapterCount = 0;
        for( Chapter *chapter: race->getChapters()){
            std::filesystem::path chapterFolder = raceFolder / ("chapter" +  std::to_string(chapterCount));
            std::filesystem::create_directories(chapterFolder);
            produceChapter(*chapter, chapterFolder);
            chapterCount ++;
        }
        raceCount ++;
    }
}

void MovieProducer::produceChapter(Chapter &chapter, std::filesystem::path &folder) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

    std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

    std::list<ClipFragment> goProclipFragments;

    // Create the input list
    findGoProClipFragments(goProclipFragments, startUtcMs, stopUtcMs);

    // Create chapter overlay clip

    int instr_ovl_width = goProclipFragments.front().width;
    int polar_ovl_width = 400;
    int height = goProclipFragments.front().height;
    int instrOvlHeight = 128;

    auto ignoreCache = false;
    std::filesystem::path instrOverlayPath = folder / "instr_overlay";
    InstrOverlayMaker instrOverlayMaker(instrOverlayPath, instr_ovl_width, instrOvlHeight, ignoreCache);

    std::filesystem::path polarOverlayPath = folder / "polar_overlay";
    PolarOverlayMaker polarOverlayMaker(m_rInstrDataVector, polarOverlayPath, polar_ovl_width,
                                        (int)chapter.getStartIdx(), (int)chapter.getEndIdx(), ignoreCache);

    int count = 0;
    int prevProgress = -1;
    int totalCount = int(chapter.getEndIdx() - chapter.getStartIdx());

    for(auto instrData = m_rInstrDataVector.begin() + (long)chapter.getStartIdx();
        instrData != m_rInstrDataVector.begin() + (long)chapter.getEndIdx(); instrData++){
        char acFileName[80];
        sprintf(acFileName, INSTR_OVL_FILE_PAT, count);
        std::string instrOvlFileName = acFileName;
        instrOverlayMaker.addEpoch(instrOvlFileName, *instrData);
        if ( chapter.getChapterType() != ChapterType::START){
            sprintf(acFileName, POLAR_OVL_FILE_PAT, count);
            std::string polarOvlFileName = acFileName;
            polarOverlayMaker.addEpoch(polarOvlFileName, (int)chapter.getStartIdx() + count);
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
    ffmpeg.addOverlayPngSequence(0, 0, fps, polarOverlayPath.native(), POLAR_OVL_FILE_PAT);

    std::filesystem::path outMoviePath = folder / "chapter.mp4";
    ffmpeg.makeClip(outMoviePath.native());
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

