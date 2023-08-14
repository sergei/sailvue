#ifndef SAILVUE_RACEDATA_H
#define SAILVUE_RACEDATA_H

#include <string>
#include <list>
#include "Chapter.h"

class RaceData {
public:
    RaceData(uint64_t  startIdx, uint64_t  endIdx);
    [[nodiscard]] const std::string &getName() const { return name; }
    [[nodiscard]] uint64_t getStartIdx() const { return m_startIdx; }
    [[nodiscard]] uint64_t getEndIdx() const { return m_endIdx; }
    void removeChapters(int i, int n);
    [[nodiscard]] std::list<Chapter *> &getChapters() ;

    void insertChapter(Chapter *chapter);

    void SetName(std::string s);
    void setEndIdx(uint64_t mEndIdx) { m_endIdx = mEndIdx; }

private:
    uint64_t m_startIdx=0;
    uint64_t  m_endIdx=0;

private:
    std::string name="Untitled Race";
    std::list<Chapter *> chapters;
};


#endif //SAILVUE_RACEDATA_H
