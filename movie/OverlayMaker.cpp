#include <filesystem>
#include <iostream>
#include "OverlayMaker.h"

OverlayMaker::OverlayMaker(const std::filesystem::path &folder, int width, int height)
: m_workDir(folder), m_width(width), m_height(height)
{
    std::cout << "Creating work folder " << m_workDir << std::endl;
    std::filesystem::create_directories(m_workDir);
}

std::filesystem::path & OverlayMaker::setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) {
    std::ostringstream oss;
    oss <<  "CLIP-PNGS-" << chapter.getUuid().toStdString();
    m_OverlayCount = 0;
    m_ChapterCount ++;
    m_ChapterFolder = m_workDir / std::filesystem::path(oss.str());
    std::filesystem::create_directories(m_ChapterFolder);

    for (auto &element : m_elements) {
        element->setChapter(chapter, chapterEpochs);
    }

    return m_ChapterFolder;
}

std::string OverlayMaker::getFileNamePattern(Chapter &chapter) {
    return {"overlay_%05d.png"};
}

AVFrame* OverlayMaker::convertQImageToAVFrame(const QImage& image) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return nullptr;
    }

    // Set frame properties
    frame->width = image.width();
    frame->height = image.height();
    frame->format = AV_PIX_FMT_RGBA;

    // Allocate frame buffer
    if (av_frame_get_buffer(frame, 0) < 0) {
        av_frame_free(&frame);
        return nullptr;
    }

    // Make frame writable
    if (av_frame_make_writable(frame) < 0) {
        av_frame_free(&frame);
        return nullptr;
    }

    // Copy data from QImage to AVFrame
    for (int y = 0; y < frame->height; y++) {
        memcpy(frame->data[0] + y * frame->linesize[0],
               image.scanLine(y),
               frame->width * 4);
    }

    return frame;
}

void OverlayMaker::addEpoch(const InstrumentInput &epoch, bool ignoreCache) {
    std::ostringstream oss;
    oss <<  "overlay_" << std::setw(5) << std::setfill('0') << m_OverlayCount << ".png";
    m_OverlayCount ++;
    std::filesystem::path pngName =  m_ChapterFolder / oss.str();

    if ( std::filesystem::is_regular_file(pngName) && !ignoreCache ){
        return ;
    }

    int frameIndex = m_OverlayCount++;

    // Skip processing if using cache
    if (!ignoreCache && m_ffmpegFrameCache.find(frameIndex) != m_ffmpegFrameCache.end()) {
      return;
    }

    // Create image for the frame
    QImage fullImage(m_width, m_height, QImage::Format_ARGB32);
    fullImage.fill(QColor(0, 0, 0, 0));
    QPainter fullPainter(&fullImage);

    // Draw all elements onto the image
    for(auto &element : m_elements){
        QImage elementImage(element->getWidth(), element->getHeight(), QImage::Format_ARGB32);
        elementImage.fill(QColor(0, 0, 0, 0));
        QPainter elementPainter(&elementImage);
        element->addEpoch(elementPainter, epoch);
        auto x = element->getX();
        if ( x < 0 ){
            x = m_width - (x+1) - element->getWidth();
        }
        fullPainter.drawImage(x, element->getY(), elementImage);
    }

    fullImage.save(QString::fromStdString(pngName.string()), "PNG");

    // Convert QImage to FFmpeg frame and add it to the frame queue
    AVFrame* frame = convertQImageToAVFrame(fullImage);
    if (frame) {
      m_frameQueue.push_back(frame);
      m_ffmpegFrameCache.insert(frameIndex);
    }
}

