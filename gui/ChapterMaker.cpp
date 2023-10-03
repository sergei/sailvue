#include "ChapterMaker.h"

ChapterMaker::ChapterMaker(TreeItem *raceTreeItem, std::vector<InstrumentInput> &instrDataVector)
        :m_pRaceTreeItem(raceTreeItem), m_rInstrDataVector(instrDataVector)
{

}

void ChapterMaker::onTack(uint32_t startIdx, uint32_t endIdx, bool isTack, double distLossMeters) {
    RaceData *pRaceData = m_pRaceTreeItem->getRaceData();

    if (startIdx < pRaceData->getStartIdx())
        startIdx = pRaceData->getStartIdx();

    if (endIdx > pRaceData->getEndIdx())
        endIdx = pRaceData->getEndIdx();

    std::string chapterName;
    if ( isTack ){
        chapterName = "Tack";
    }else{
        chapterName = "Gybe";
    }

    std::cout << "insertChapter " << chapterName << " startIdx " << startIdx << " endIdx " << endIdx << std::endl;

    auto *chapter = new Chapter(startIdx, endIdx);
    chapter->SetName(chapterName);
    chapter->setChapterType(ChapterTypes::ChapterType::TACK_GYBE);
    pRaceData->insertChapter(chapter);

    // Add data to the view model
    auto *chapterTreeItem = new TreeItem(pRaceData, chapter, m_pRaceTreeItem);
    m_pRaceTreeItem->insertChapterChild(chapterTreeItem);
}

void ChapterMaker::onMarkRounding(uint32_t eventIdx, uint32_t startIdx, uint32_t endIdx, bool isWindward) {

    RaceData *pRaceData = m_pRaceTreeItem->getRaceData();

    if (startIdx < pRaceData->getStartIdx())
        startIdx = pRaceData->getStartIdx();

    if (endIdx > pRaceData->getEndIdx())
        endIdx = pRaceData->getEndIdx();

    std::string chapterName;
    if ( isWindward ){
        chapterName = "Windward mark";
    }else{
        chapterName = "Leeward mark";
    }

    std::cout << "insertChapter " << chapterName << " startIdx " << startIdx << " endIdx " << endIdx << std::endl;

    auto *chapter = new Chapter(startIdx, endIdx);
    chapter->SetName(chapterName);
    chapter->setChapterType(ChapterTypes::ChapterType::MARK_ROUNDING);
    chapter->SetGunIdx(eventIdx);
    pRaceData->insertChapter(chapter);

    // Add data to the view model
    auto *chapterTreeItem = new TreeItem(pRaceData, chapter, m_pRaceTreeItem);
    m_pRaceTreeItem->insertChapterChild(chapterTreeItem);
}

Chapter *ChapterMaker::makePerformanceChapter(Chapter *pPrevChapter, Chapter *pNextChapter) {
    auto startIdx = pPrevChapter->getEndIdx() + 1;
    auto endIdx = pNextChapter->getStartIdx() - 1;

    uint64_t startUtcMs = m_rInstrDataVector[startIdx].utc.getUnixTimeMs();
    uint64_t stopUtcMs = m_rInstrDataVector[endIdx].utc.getUnixTimeMs();

    uint64_t durationMs = stopUtcMs - startUtcMs;
    // Don't make too short chapters
    if (durationMs > MIN_PERF_CHAPTER_DURATION) {
        auto *chapter = new Chapter(startIdx, endIdx);
        chapter->setChapterType(ChapterTypes::SPEED_PERFORMANCE);

        // Now let's come up with the name
        double minTwa = 200;
        double maxTwa = -200;
        for(uint64_t i = startIdx; i < endIdx; i++){
            if( m_rInstrDataVector[i].twa.isValid(m_rInstrDataVector[i].utc.getUnixTimeMs())){
                double twa = m_rInstrDataVector[i].twa.getDegrees();
                if ( twa < minTwa)
                    minTwa = twa;
                if ( twa > maxTwa)
                    maxTwa = twa;
            }
        }

        if ( maxTwa < -190 ){
            std::cout << "No valid wind for performance chapter" << std::endl;
            return nullptr;
        }

        bool starBoard = minTwa >= 0 && maxTwa >= 0;
        bool downWind = abs(minTwa) > 100;

        std::string name;
        bool fetch = isFetch(pPrevChapter, pNextChapter);

        if ( fetch ){
            name = std::string(downWind ? "downwind" : "upwind") + " fetch";
        }else{
            name = std::string(starBoard ? "Starboard " : "Port ")
                   + std::string(downWind ? "downwind" : "upwind");
        }

        chapter->SetName(name);

        return chapter;
    }else{
        return nullptr;
    }
}

bool ChapterMaker::isFetch(const Chapter *pPrevChapter,
                           const Chapter *pNextChapter)  {// Determine if it's a fetch ( no tack or gybe before or after)
    bool isFetch = pPrevChapter->getChapterType() != ChapterTypes::TACK_GYBE &&
                   pNextChapter->getChapterType() != ChapterTypes::TACK_GYBE;
    return isFetch;
}
