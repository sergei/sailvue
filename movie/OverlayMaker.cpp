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
    oss <<  "chapter_" << std::setw(3) << std::setfill('0') << m_ChapterCount;
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

void OverlayMaker::addEpoch(const InstrumentInput &epoch) {
    std::ostringstream oss;
    oss <<  "overlay_" << std::setw(5) << std::setfill('0') << m_OverlayCount << ".png";
    m_OverlayCount ++;
    std::filesystem::path pngName =  m_ChapterFolder / oss.str();

    if ( std::filesystem::is_regular_file(pngName) ){
        return ;
    }
    QImage fullImage(m_width, m_height, QImage::Format_ARGB32);
    fullImage.fill(QColor(0, 0, 0, 0));
    QPainter fullPainter(&fullImage);


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
}

