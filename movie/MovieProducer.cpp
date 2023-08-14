#include "MovieProducer.h"
#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "InstrOverlayMaker.h"

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
            m_rProgressListener.progress(chapter->getName(), 50);
            produceChapter(*chapter, chapterFolder);
            chapterCount ++;
        }
        raceCount ++;
    }
}

void MovieProducer::produceChapter(Chapter &chapter, std::filesystem::path folder) {
    uint64_t startUtcMs = m_rInstrDataVector[chapter.getStartIdx()].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[chapter.getEndIdx()].utc.getUnixTimeMs();

    std::cout << "Producing chapter " << chapter.getName() << " " << startUtcMs << ":" << stopUtcMs << std::endl;

    std::list<ClipFragment> goProclipFragments;

    // Create the input list
    findGoProClipFragments(goProclipFragments, startUtcMs, stopUtcMs);

    // Create chapter overlay clip
    std::filesystem::path instrOverlayPath = folder / "instr_overlay";

    int width = goProclipFragments.front().width;
    InstrOverlayMaker instrOverlayMaker(instrOverlayPath, width, 128, true);
    int count = 0;
    for(auto instrData = m_rInstrDataVector.begin() + chapter.getStartIdx();
        instrData != m_rInstrDataVector.begin() + chapter.getEndIdx(); instrData++){
        std::stringstream ss;
        ss << "INSTR_OVL" << std::setw(5) << std::setfill('0') << count << ".png";
        instrOverlayMaker.addEpoch(ss.str(), *instrData);
    }

    // make ffmpeg arguments
    std::filesystem::path outMoviePath = folder / "chapter.mp4";

    std::string ffmpegArgs = "~/bin/ffmpeg ";
    for(const auto& src: goProclipFragments){
        if ( src.in != -1)
            ffmpegArgs += " -ss " +  std::to_string(src.in) + "ms ";

        if ( src.out != -1)
            ffmpegArgs += " -to " +  std::to_string(src.out) + "ms ";

        ffmpegArgs += " -i \"" + src.fileName + "\"";
    }
    ffmpegArgs += " \"" + outMoviePath.native() + "\"";
    std::cout <<  "[" << ffmpegArgs << "]" << std::endl;
}

void MovieProducer::findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs, uint64_t stopUtcMs) {
    int64_t inTime = -1;
    int64_t outTime = -1;
    // Determine list of GOPRO clips and their in and out points for given clip
    for(auto firstClip= m_rGoProClipInfoList.begin(); firstClip != m_rGoProClipInfoList.end(); firstClip++){
//        std::cout << "First clip " << firstClip->getFileName() << " " << firstClip->getClipStartUtcMs() << ":" << firstClip->getClipEndUtcMs() << std::endl;
        if(firstClip->getClipStartUtcMs() <= startUtcMs && startUtcMs <= firstClip->getClipEndUtcMs() ) { // Chapter starts in this clip
            inTime = startUtcMs - firstClip->getClipStartUtcMs();

            // Now find the clip containing the stop time
            for(auto clip = firstClip; clip != m_rGoProClipInfoList.end(); clip++){
//                std::cout << "clip " << clip->getFileName() << " " << clip->getClipStartUtcMs() << ":" << clip->getClipEndUtcMs() << std::endl;
                if (stopUtcMs <= clip->getClipEndUtcMs()){ // Last clip
                    // Check for the corner case when the stop_utc falls inbetween start_utc of the previous clip
                    // and start_utc of this one
                    if (stopUtcMs <= clip->getClipStartUtcMs()){
                        break;
                    }
                    outTime = stopUtcMs - clip->getClipStartUtcMs();
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

