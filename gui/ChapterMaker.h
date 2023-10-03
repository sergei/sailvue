//
// Created by Sergei on 10/2/23.
//

#ifndef SAILVUE_CHAPTERMAKER_H
#define SAILVUE_CHAPTERMAKER_H

#include "RaceTreeModel.h"
#include "ChapterTypes.h"

class ChapterMaker : public NavStatsEventsListener {
public:
    explicit ChapterMaker(TreeItem *raceTreeItem, std::vector<InstrumentInput> &instrDataVector);
    void onTack(uint32_t fromIdx, uint32_t toIdx, bool isTack, double distLossMeters) override;
    void onMarkRounding(uint32_t eventIdx, uint32_t fromIdx, uint32_t toIdx, bool isWindward) override;
    Chapter *makePerformanceChapter(Chapter *pPrevChapter, Chapter *pNextChapter);
private:
    TreeItem *m_pRaceTreeItem;
    std::vector<InstrumentInput> &m_rInstrDataVector;

};


#endif //SAILVUE_CHAPTERMAKER_H
