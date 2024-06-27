#ifndef SAILVUE_MARKERREADER_H
#define SAILVUE_MARKERREADER_H

#include <filesystem>
#include <list>
#include "adobe_premiere/ClipMarker.h"
#include "navcomputer/Chapter.h"
#include "cameras/CameraBase.h"


class MarkerReader {
public:
    void setTimeAdjustmentMs(int64_t timeAdjustmentMs){m_timeAdjustmentMs = timeAdjustmentMs;};
    void read(const std::filesystem::path &markersDir, const std::list<CameraClipInfo *> &cameraClips);
    void makeChapters(std::list<Chapter *> &chapters, std::vector<InstrumentInput> &instrDataVector);
    const std::list<ClipMarker> &getMarkersList() const { return m_markersList; }

    void makeMarkers(const std::list<Chapter *>& chapters, std::vector<InstrumentInput> &instrDataVector,
                     std::list<CameraClipInfo *> &clips, const std::filesystem::path &markerFile) const;

private:
    std::list<ClipMarker> m_markersList;

private:
    int64_t m_timeAdjustmentMs = 0;
    static std::string makeCsvEntry(const Chapter *pChapter, uint64_t inUtcMs, uint64_t outUtcMs, std::list<CameraClipInfo *> &clips);
};


#endif //SAILVUE_MARKERREADER_H
