#ifndef SAILVUE_MOVIEPRODUCER_H
#define SAILVUE_MOVIEPRODUCER_H

#include "navcomputer/Polars.h"

#include <list>

#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "ffmpeg/FFMpeg.h"

class EncodingProgressListener : public FfmpegProgressListener{
public:
    EncodingProgressListener(const std::string &mPrefix, const uint64_t mTotalDurationMs,
                             IProgressListener &rProgressListener)
    :m_prefix(mPrefix), m_totalDurationMs (mTotalDurationMs), m_rProgressListener(rProgressListener)
    {
        m_prevPercent = -1;
    }
    [[nodiscard]] bool isStopRequested() const { return m_stopRequested; };

private:
    const std::string &m_prefix;
    const uint64_t m_totalDurationMs;
    IProgressListener &m_rProgressListener;
    bool m_stopRequested=false;
private:
    int m_prevPercent = -1;
    bool ffmpegProgress(uint64_t msEncoded) override;
};

class MovieProducer  {
public:
    MovieProducer(const std::string &path, const std::string &polarPath, std::list<GoProClipInfo> &clipsList,
                  std::vector<InstrumentInput> &instrDataVector,
                  std::list<RaceData *> &raceList, IProgressListener &rProgressListener);

    void produce();
private:
    bool m_stopRequested = false;
    uint64_t m_totalRaceDuration = 0;

    const std::string &m_moviePath;
    IProgressListener& m_rProgressListener;
    std::list<GoProClipInfo> &m_rGoProClipInfoList;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::list<RaceData *> &m_RaceDataList;

    std::string produceChapter(Chapter &chapter, std::filesystem::path &folder, bool ignoreCache);
    void findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs, uint64_t stopUtcMs);
    void makeRaceVideo(const std::filesystem::path &raceFolder, std::list<std::string> &chaptersList);

    const char *INSTR_OVL_FILE_PAT  = "instr_%05d.png";
    const char *POLAR_OVL_FILE_PAT  = "polar_%05d.png";
    const char *TARGET_OVL_FILE_PAT = "target_%05d.png";
    const char *TIMER_OVL_FILE_PAT  = "timer_%05d.png";

    Polars m_polars;
};


#endif //SAILVUE_MOVIEPRODUCER_H
