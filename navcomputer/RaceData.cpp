#include "RaceData.h"

RaceData::RaceData(uint64_t  startIdx, uint64_t  endIdx)
    :m_startIdx(startIdx), m_endIdx(endIdx){

}

void RaceData::insertChapter(Chapter *chapter) {
    // Need to insert chapters in order of startIdx
    for(auto it = chapters.begin(); it != chapters.end(); it++){
        if( (*it)->getStartIdx() > chapter->getStartIdx() ){
            chapters.insert(it, chapter);
            return;
        }
    }
    // If we get here, then the chapter is the last one
    chapters.push_back(chapter);
}

void RaceData::SetName(std::string s) {
    name = s;
}

std::list<Chapter *> &RaceData::getChapters()  {
    return chapters;
}

void RaceData::removeChapters(int i, int n) {
    std::list<Chapter *>::iterator it1,it2;
    it1 = it2 = chapters.begin();
    advance(it1, i);
    advance(it2, i+n);
    chapters.erase(it1, it2);
}

