#ifndef SAILVUE_MOVIEPRODUCER_H
#define SAILVUE_MOVIEPRODUCER_H

#include <list>

#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "ffmpeg/FFMpeg.h"

class MovieProducer {
public:
    MovieProducer(const std::string &path, std::list<GoProClipInfo> &clipsList,
                  std::vector<InstrumentInput> &instrDataVector,
                  std::list<RaceData *> &raceList, IProgressListener &rProgressListener);

    void produce();
private:
    const std::string &m_moviePath;
    IProgressListener& m_rProgressListener;
    std::list<GoProClipInfo> &m_rGoProClipInfoList;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::list<RaceData *> &m_RaceDataList;

    void produceChapter(Chapter &chapter, std::filesystem::path &folder);
    void findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs, uint64_t stopUtcMs);

    const char *INSTR_OVL_FILE_PAT = "instr_%05d.png";
    const char *POLAR_OVL_FILE_PAT = "polar_%05d.png";

};


#endif //SAILVUE_MOVIEPRODUCER_H
