#ifndef SAILVUE_MOVIEPRODUCER_H
#define SAILVUE_MOVIEPRODUCER_H

#include <list>

#include "navcomputer/IProgressListener.h"
#include "Worker.h"

struct ClipFragment {
    ClipFragment(int64_t in, int64_t out, const std::string &fileName, int w, int h):
        in(in), out(out),
        fileName(fileName), width(w), height(h) {}

    int64_t in; // Milliseconds
    int64_t out; // Milliseconds
    const int width;
    const int height;
    std::string fileName;
};

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

    void produceChapter(Chapter &chapter, std::filesystem::path folder);

    void findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs, uint64_t stopUtcMs);
};


#endif //SAILVUE_MOVIEPRODUCER_H
