#ifndef SAILVUE_CHAPTER_H
#define SAILVUE_CHAPTER_H

#include <string>
#include <list>
#include <QUuid>
#include "InstrumentInput.h"
#include "ChapterTypes.h"

static const int MIN_PERF_CHAPTER_DURATION = 1 * 60 * 1000; //

class Chapter {
public:
    Chapter(u_int64_t startIdx, u_int64_t endIdx);
    [[nodiscard]] const std::string &getName() const { return name; }
    [[nodiscard]] u_int64_t getStartIdx() const { return m_startIdx; }
    [[nodiscard]] u_int64_t getEndIdx() const { return m_endIdx; }
    [[nodiscard]] u_int64_t getGunIdx() const { return m_gunIdx; }
    [[nodiscard]] QString getUuid() const { return m_uuid.toString(QUuid::WithoutBraces); }
    [[nodiscard]] ChapterTypes::ChapterType getChapterType() const { return m_chapterType; }

    void SetName(std::string name);
    void setEndIdx(u_int64_t endIdx) { m_endIdx = endIdx; }
    void setStartIdx(u_int64_t startIdx) { m_startIdx = startIdx; }
    void SetGunIdx(u_int64_t gunIdx) { m_gunIdx = gunIdx; }
    void setChapterType(ChapterTypes::ChapterType chapterType) { m_chapterType = chapterType; }

private:
    QUuid m_uuid;
    std::string name="Untitled Chapter";
    u_int64_t m_startIdx=0;
    u_int64_t m_endIdx=0;
    u_int64_t m_gunIdx=0;
    ChapterTypes::ChapterType m_chapterType = ChapterTypes::TACK_GYBE;
};


#endif //SAILVUE_CHAPTER_H
