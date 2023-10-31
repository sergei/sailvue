#ifndef SAILVUE_OVERLAYMAKER_H
#define SAILVUE_OVERLAYMAKER_H


#include <string>
#include "OverlayElement.h"

class OverlayMaker {
public:
    OverlayMaker(const std::filesystem::path &folder, int width, int height);
    void addOverlayElement(OverlayElement &element){m_elements.push_back(&element);}
    void setChapter(Chapter &chapter);
    void addEpoch(int epochIdx);
    static std::string getFileNamePattern(Chapter &chapter);
    [[nodiscard]] const std::filesystem::path &getChapterFolder() const { return m_ChapterFolder; }
private:
    std::list<OverlayElement *> m_elements;
    const std::filesystem::path &m_workDir;
    const int m_width;
    const int m_height;
    std::filesystem::path m_ChapterFolder;
    int m_ChapterCount = 0;
    int m_OverlayCount = 0;
    std::map<std::string, std::string> m_chapterNamePatterns;
};


#endif //SAILVUE_OVERLAYMAKER_H
