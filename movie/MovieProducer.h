#ifndef SAILVUE_MOVIEPRODUCER_H
#define SAILVUE_MOVIEPRODUCER_H

#include "navcomputer/Polars.h"

#include <list>
#include <utility>

#include "navcomputer/IProgressListener.h"
#include "Worker.h"
#include "ffmpeg/FFMpeg.h"
#include "TargetsOverlayMaker.h"
#include "OverlayMaker.h"

class EncodingProgressListener : public FfmpegProgressListener{
public:
    EncodingProgressListener(std::string mPrefix, const uint64_t mTotalDurationMs,
                             IProgressListener &rProgressListener)
    :m_prefix(std::move(mPrefix)), m_totalDurationMs (mTotalDurationMs), m_rProgressListener(rProgressListener)
    {
        m_prevPercent = -1;
    }
    [[nodiscard]] bool isStopRequested() const { return m_stopRequested; };

private:
    const std::string m_prefix;
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
                  std::vector<InstrumentInput> &instrDataVector, std::map<uint64_t, Performance> &performanceVector,
                  std::list<RaceData *> &raceList, IProgressListener &rProgressListener);

    void produce();

private:
    std::string produceChapter(OverlayMaker &overlayMaker, Chapter &chapter, int chapterNum, int totalChapters);
    void findGoProClipFragments(std::list<ClipFragment> &clipFragments, uint64_t startUtcMs, uint64_t stopUtcMs);
    void makeRaceVideo(const std::filesystem::path &raceFolder, std::list<std::string> &chaptersList);
    void makeChapterDescription(std::ofstream &df, const Chapter *chapter, uint64_t sec) ;
    bool isClipCacheValid(const std::filesystem::path &summaryFile, Chapter &chapter);

private:
    bool m_stopRequested = false;
    uint64_t m_totalRaceDuration = 0;

    const std::string &m_moviePath;
    IProgressListener& m_rProgressListener;
    std::list<GoProClipInfo> &m_rGoProClipInfoList;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::map<uint64_t, Performance> &m_rPerformanceVector;
    std::list<RaceData *> &m_RaceDataList;

    Polars m_polars;

    static void makeSummaryFile(const std::filesystem::path &summaryFile, const Chapter &chapter) ;
};


#endif //SAILVUE_MOVIEPRODUCER_H
