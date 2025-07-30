#ifndef SAILVUE_OVERLAYMAKER_H
#define SAILVUE_OVERLAYMAKER_H


#include <string>
#include <set>       // For std::set
#include "OverlayElement.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

class OverlayMaker {
public:
    OverlayMaker(const std::filesystem::path &folder, int width, int height);
    void addOverlayElement(OverlayElement &element){m_elements.push_back(&element);}
    std::filesystem::path & setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs);
    void addEpoch(const InstrumentInput &epoch, bool ignoreCache=false);
    static std::string getFileNamePattern(Chapter &chapter);
    std::vector<AVFrame*>& getFrameQueue() {
      return m_frameQueue;
    }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
private:
    AVFrame* convertQImageToAVFrame(const QImage& image);
private:
    std::list<OverlayElement *> m_elements;
    const std::filesystem::path &m_workDir;
    const int m_width;
    const int m_height;
    std::filesystem::path m_ChapterFolder;
    int m_ChapterCount = 0;
    int m_OverlayCount = 0;
    std::map<std::string, std::string> m_chapterNamePatterns;
    std::vector<AVFrame*> m_frameQueue;
    std::set<int> m_ffmpegFrameCache;
};


#endif //SAILVUE_OVERLAYMAKER_H
