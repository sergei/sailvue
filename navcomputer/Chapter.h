#ifndef SAILVUE_CHAPTER_H
#define SAILVUE_CHAPTER_H

#include <string>
#include <list>
#include <QUuid>
#include "InstrumentInput.h"

enum ChapterType{
    BOAT_HANDLING,
    SPEED_PERFORMANCE,
    START,
};

class Chapter {
public:
    Chapter(u_int64_t startIdx, u_int64_t endIdx);
    [[nodiscard]] const std::string &getName() const { return name; }
    [[nodiscard]] u_int64_t getStartIdx() const { return m_startIdx; }
    [[nodiscard]] u_int64_t getEndIdx() const { return m_endIdx; }
    [[nodiscard]] u_int64_t getGunIdx() const { return m_gunIdx; }
    [[nodiscard]] QString getUuid() const { return m_uuid.toString(QUuid::WithoutBraces); }
    ChapterType getChapterType() { return m_chapterType; }

    void SetName(std::string name);
    void setEndIdx(u_int64_t endIdx) { m_endIdx = endIdx; }
    void setStartIdx(u_int64_t startIdx) { m_startIdx = startIdx; }
    void SetGunIdx(u_int64_t gunIdx) { m_gunIdx = gunIdx; }
    void setChapterType(ChapterType chapterType) { m_chapterType = chapterType; }

private:
    QUuid m_uuid;
    std::string name="Untitled Chapter";
    u_int64_t m_startIdx=0;
    u_int64_t m_endIdx=0;
    u_int64_t m_gunIdx=0;
    ChapterType m_chapterType = BOAT_HANDLING;
};


#endif //SAILVUE_CHAPTER_H
