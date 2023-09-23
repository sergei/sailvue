#include <iostream>
#include "RaceTreeModel.h"
#include "Worker.h"
#include "navcomputer/NavStats.h"

TreeItem::TreeItem(RaceData *pRaceData, Chapter *pChapter, TreeItem *parent)
        : m_pRaceData(pRaceData), m_pChapter(pChapter), m_parentItem(parent)
{

}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
    m_childItems.append(item);
}

void TreeItem::insertChapterChild(TreeItem *child) {
    // insert chapters in order of startIdx
    for(auto it = m_childItems.begin(); it != m_childItems.end(); it++){
        if( (*it)->getChapter()->getStartIdx() > child->getChapter()->getStartIdx() ){
            m_childItems.insert(it, child);
            return;
        }
    }
    // The chapter is the last one
    m_childItems.append(child);
}

void TreeItem::removeChild(TreeItem *item)
{
    m_childItems.removeOne(item);
}

void TreeItem::removeChildren(int i, int n)
{
    m_childItems.remove(i, n);
}

TreeItem *TreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int TreeItem::childCount() const
{
    return int(m_childItems.count());
}

int TreeItem::row() const
{
    if (m_parentItem)
        return int(m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this)));

    return 0;
}

int TreeItem::columnCount()
{
    // Always 1
    return 1;
}

QVariant TreeItem::data(int column) const
{
    if (column < 0 || column >= 1)
        return {};  // Have only one column

    if ( m_pChapter != nullptr )
        return QString::fromStdString(m_pChapter->getName());
    else if ( m_pRaceData != nullptr )
        return QString::fromStdString(m_pRaceData->getName());
    else
        return {}; // Should never happen

}

TreeItem *TreeItem::parentItem()
{
    return m_parentItem;
}

RaceData *TreeItem::getRaceData()  {
    return m_pRaceData;
}

Chapter *TreeItem::getChapter()  {
    return m_pChapter;
}


RaceTreeModel::RaceTreeModel(QObject *parent)
:QAbstractItemModel(parent),
 m_project(m_RaceDataList) {
    rootItem = new TreeItem();

    auto *worker = new Worker(m_GoProClipInfoList, m_InstrDataVector, m_RaceDataList);
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    connect(this, &RaceTreeModel::readData, worker, &Worker::readData);
    connect(this, &RaceTreeModel::stop, worker, &Worker::stopWork);
    connect(worker, &Worker::ProgressStatus, this, &RaceTreeModel::handleProgress);
    connect(worker, &Worker::pathAvailable, this, &RaceTreeModel::handleNewInstrDataVector);

    connect(this, &RaceTreeModel::produce, worker, &Worker::produce);
    connect(worker, &Worker::produceStarted, this, &RaceTreeModel::handleProduceStarted);
    connect(worker, &Worker::produceFinished, this, &RaceTreeModel::handleProduceFinished);


    workerThread.start();
}

RaceTreeModel::~RaceTreeModel()
{
    workerThread.quit();
    workerThread.wait();

    delete rootItem;
}

QModelIndex RaceTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return {};
}

QModelIndex RaceTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto *childItem = static_cast<TreeItem*>(index.internalPointer());
    if ( childItem == nullptr )
        return {};

    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}

int RaceTreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int RaceTreeModel::columnCount(const QModelIndex &parent) const
{
    int i;
    if (parent.isValid()){
        i = static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    }else{
        i = rootItem->columnCount();
    }

    return i;
}

QVariant RaceTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role != Qt::DisplayRole)
        return {};

    auto *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags RaceTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant RaceTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return {};
}

void RaceTreeModel::handleNewInstrDataVector() {

    // Create GeoPath to be shown on a map
    m_geoPath.clearPath();
    for( auto &ii : m_InstrDataVector) {
        m_geoPath.addCoordinate(QGeoCoordinate(ii.loc.getLat(), ii.loc.getLon()));
    }
    emit fullPathReady(m_geoPath);

    makeRaceList();

    showRaceData();
    m_ulCurrentInstrDataIdx = 0;

    emit loadFinished();
}

void RaceTreeModel::load(const QString &path) {
    if ( m_project.load(path) ){
        emit nmeaPathChanged();
        emit goProPathChanged();
        emit polarPathChanged();
        emit projectNameChanged();
        emit isDirtyChanged();
        emit readData(m_project.goproPath(), m_project.nmeaPath(), m_project.polarPath(), false);
    }
}

void RaceTreeModel::read(bool ignoreCache) {
    deleteAllRaces();
    emit readData(m_project.goproPath(), m_project.nmeaPath(), m_project.polarPath(), ignoreCache);
}

void RaceTreeModel::save() {
    if ( m_project.save() ) {
        emit isDirtyChanged();
    }
}

void RaceTreeModel::saveAs(const QString &path) {
    if ( m_project.saveAs(path) ) {
        emit projectNameChanged();
        emit isDirtyChanged();
    }
}

void RaceTreeModel::handleProgress(const QString &state, int progress) {
    emit progressStatus(state, progress);
}

void RaceTreeModel::stopDataProcessing() {
    emit stop();
}

void RaceTreeModel::seekToRacePathIdx(uint64_t racePathIdx) {
    m_ulCurrentInstrDataIdx = racePathIdx;
    emit racePathIdxChanged(racePathIdx);
}

qint64 RaceTreeModel::getIdxForUtc(uint64_t ulUtcMs) {
    // Now find the corresponding instr data idx
    for( qint64 i = 0; i < m_InstrDataVector.size(); i++){  // TODO don't alawys start from 0 but from the last idx
        if (m_InstrDataVector[i].utc.getUnixTimeMs() >= ulUtcMs ){
            m_ulCurrentInstrDataIdx = i;
            return m_ulCurrentInstrDataIdx;
        }
    }
    std::cerr << "clipPositionToRacePathIdx: No instr data found for " << ulUtcMs << std::endl;

    return -1;
}

void RaceTreeModel::showRaceData() {
    std::cout << "showRaceData" << std::endl;
    emit layoutAboutToBeChanged();

    delete rootItem;
    rootItem = new TreeItem();

    for ( RaceData *race : m_RaceDataList ){
        // Add race
        auto *raceTreeItem = new TreeItem(race, nullptr, rootItem);
        rootItem->appendChild(raceTreeItem);

        for( Chapter *chapter: race->getChapters() ) {
            // Add chapter
            auto *chapterTreeItem = new TreeItem(race, chapter, raceTreeItem);
            raceTreeItem->appendChild(chapterTreeItem);

            emit chapterAdded(chapter->getUuid(),QString::fromStdString(chapter->getName()),
                              chapter->getChapterType(),chapter->getStartIdx(),chapter->getEndIdx());
        }
    }

    emit layoutChanged();
}


void RaceTreeModel::deleteAllRaces() {
    emit layoutAboutToBeChanged();

    // Delete underlying data
    for (RaceData *race: m_RaceDataList){
        race->getChapters().clear();
    }
    m_RaceDataList.clear();

    // Delete view model
    rootItem->removeChildren(0, rootItem->childCount());

    emit layoutChanged();
}


void RaceTreeModel::deleteSelected() {
    if (!m_selectedTreeIdx.isValid())
        return ;

    auto *itemToDelete = static_cast<TreeItem*>(m_selectedTreeIdx.internalPointer());

    // Get ready for removal from the view model
    emit beginRemoveRows(m_selectedTreeIdx.parent(), m_selectedTreeIdx.row(), m_selectedTreeIdx.row());

    // Remove from underlying data
    auto *race = const_cast<RaceData*>(itemToDelete->getRaceData());
    std::cout << "deleteSelected Race: " << race->getName() << std::endl;

    if( itemToDelete->isRace() ){
        m_RaceDataList.remove(race);
        m_project.raceDataChanged();
        emit isDirtyChanged();
    }else{
        auto *chapter = const_cast<Chapter *>(itemToDelete->getChapter());
        race->getChapters().remove(chapter);
        std::cout << "deleteSelected Chapter: " << chapter->getName() << std::endl;
        m_project.raceDataChanged();
        emit isDirtyChanged();
        emit chapterDeleted(chapter->getUuid());
    }

    // Remove from the view model
    TreeItem *parent = itemToDelete->parentItem();
    parent->removeChild(itemToDelete);

    // Notify to redraw
    emit endRemoveRows();
}

void RaceTreeModel::addChapter() {

    // Find race to insert chapter into
    for( int i = 0; i < rootItem->childCount(); i++){
        TreeItem *raceTreeItem = rootItem->child(i);
        RaceData *race = raceTreeItem->getRaceData();
        if ( race->getStartIdx() <= m_ulCurrentInstrDataIdx && race->getEndIdx() >= m_ulCurrentInstrDataIdx){

            emit layoutAboutToBeChanged();
            // Found the race the UTC is pointing at
            // Add data to underlying storage
            
            // Make chapter one minute long starting at current UTC
            u_int64_t startIdx = m_ulCurrentInstrDataIdx;
            auto chapterStartMs = m_InstrDataVector[m_ulCurrentInstrDataIdx].utc.getUnixTimeMs() - 30000;
            for(;startIdx > 0; startIdx--){
                if (m_InstrDataVector[startIdx].utc.getUnixTimeMs() <= chapterStartMs)
                    break;
            }
            
            u_int64_t endIdx = m_ulCurrentInstrDataIdx;
            auto chapterEndMs = m_InstrDataVector[m_ulCurrentInstrDataIdx].utc.getUnixTimeMs() + 30000;
            for(;endIdx <= race->getEndIdx(); endIdx++){
                if (m_InstrDataVector[endIdx].utc.getUnixTimeMs() >= chapterEndMs)
                    break;
            }

            int chapterNum = raceTreeItem->childCount() + 1;
            std::string chapterName = "Chapter " + std::to_string(chapterNum);

            std::cout << "insertChapter " << chapterName << " startIdx " << startIdx << " endIdx " << endIdx << std::endl;

            auto *chapter = new Chapter(startIdx, endIdx);
            chapter->SetName(chapterName);
            race->insertChapter(chapter);

            // Add data to the view model
            auto *chapterTreeItem = new TreeItem(race, chapter, raceTreeItem);
            raceTreeItem->insertChapterChild(chapterTreeItem);

            m_project.raceDataChanged();
            emit isDirtyChanged();
            emit chapterAdded(chapter->getUuid(), QString::fromStdString(chapter->getName()), 0,
                                 chapter->getStartIdx(),  chapter->getEndIdx());
            emit layoutChanged();

            return;
        }
    }
    std::cerr << "No race found for the new chapter" << std::endl;
}

void RaceTreeModel::updateRace(const QString &raceName){
    std::cout << "updateRace " << raceName.toStdString() << std::endl;
    emit layoutAboutToBeChanged();
    RaceData *race = getSelectedRace();

    race->SetName(raceName.toStdString());
    m_project.raceDataChanged();

    emit isDirtyChanged();
    emit layoutChanged();
}

void RaceTreeModel::updateChapter(const QString &uuid, const QString &chapterName, uint64_t chapterType, uint64_t startIdx,
                                  uint64_t endIdx, uint64_t gunIdx) {
    std::cout << "updateChapter " << uuid.toStdString() << " " << chapterName.toStdString() << " " << chapterType << " " << startIdx << " " << endIdx << " " << gunIdx << std::endl;

    Chapter *chapter = getSelectedChapter();
    if ( chapter == nullptr ){
        std::cerr << "No chapter selected" << std::endl;
        return;
    }

    if ( chapter->getUuid() == uuid ) {
        emit layoutAboutToBeChanged();

        chapter->SetName(chapterName.toStdString());
        chapter->setChapterType(ChapterType(chapterType));
        chapter->setStartIdx(startIdx);
        chapter->setEndIdx(endIdx);
        chapter->SetGunIdx(gunIdx);
        m_project.raceDataChanged();
        emit isDirtyChanged();
        emit layoutChanged();

    }else{
        std::cerr << "Chapter uuid mismatch" << std::endl;
    }

}


void RaceTreeModel::splitRace() {
    if ( m_ulCurrentInstrDataIdx == 0){
        std::cerr << "Ignore split " << std::endl;
        return;
    }

    // Find the race to be split
    for( int i = 0; i < rootItem->childCount(); i++) {
        TreeItem *prevRaceTreeItem = rootItem->child(i);
        RaceData *prevRace = prevRaceTreeItem->getRaceData();
        if (prevRace->getStartIdx() <= m_ulCurrentInstrDataIdx && prevRace->getEndIdx() >= m_ulCurrentInstrDataIdx) {
            emit layoutAboutToBeChanged();

            // Determine the new end of old race
            uint64_t ulPrevRaceEndIdx = m_ulCurrentInstrDataIdx - 1;
            // Old race duration
            uint64_t ulPrevRaceStartMs = m_InstrDataVector[prevRace->getStartIdx()].utc.getUnixTimeMs();
            uint64_t ulPrevRaceEndMs = m_InstrDataVector[ulPrevRaceEndIdx].utc.getUnixTimeMs();

            if ((ulPrevRaceEndMs - ulPrevRaceStartMs) <= 2000 ){
                std::cerr << "Old race will be too short" << std::endl;
                return;
            }

            // Determine new race start and end idx
            uint64_t newRaceStartIdx = m_ulCurrentInstrDataIdx;
            uint64_t newRaceEndIdx = m_InstrDataVector.size() - 1;
            std::cout << "newRaceStartIdx " << newRaceStartIdx << " newRaceEndIdx " << newRaceEndIdx << std::endl;

            // Check if there is a race after that and adjust newRaceEndIdx if necessary

            int nextRaceIdx = i+1;
            if ( nextRaceIdx < rootItem->childCount() ){
                TreeItem *nextRaceTreeItem = rootItem->child(nextRaceIdx);
                RaceData *nextRace = nextRaceTreeItem->getRaceData();
                newRaceEndIdx = nextRace->getStartIdx() - 1;
                std::cout << "after adjustment newRaceStartIdx " << newRaceStartIdx << " newRaceEndIdx " << newRaceEndIdx << std::endl;
            }

            // Move chapters from old race to the new race if necessary
            QList<TreeItem *> replants;
            int prevChaptersNum = prevRaceTreeItem->childCount();
            int removeFrom = -1;
            int removeFromCount = 0;
            for( int j = 0; j < prevChaptersNum; j++){
                TreeItem *chapterItem = prevRaceTreeItem->child(j);
                Chapter *chapter = chapterItem->getChapter();
                if ( chapter->getStartIdx() >= newRaceStartIdx && chapter->getEndIdx() <= newRaceEndIdx){
                    if ( removeFrom == -1 )
                        removeFrom = j;
                    replants.append(chapterItem);
                    removeFromCount++;
                }
            }

            if ( removeFromCount != 0 ){
                // Remove from the old underlying data
                prevRace->removeChapters(removeFrom, removeFromCount);
                // Remove from the old race view model
                prevRaceTreeItem->removeChildren(removeFrom, removeFromCount);
            }

            // Adjust end of old race
            prevRace->setEndIdx(ulPrevRaceEndIdx);

            // Create new race
            auto *race = new RaceData(newRaceStartIdx, newRaceEndIdx);
            race->SetName("New race");

            // Add chapters to the new race
            for( TreeItem *chapterItem: replants){
                Chapter *chapter = chapterItem->getChapter();
                race->insertChapter(chapter);
            }

            m_RaceDataList.push_back(race);

            // Add data to the view model
            auto *raceTreeItem = new TreeItem(race, nullptr, rootItem);
            rootItem->appendChild(raceTreeItem);
            for ( Chapter *chapter: race->getChapters() ){
                auto *chapterTreeItem = new TreeItem(race, chapter, raceTreeItem);
                raceTreeItem->appendChild(chapterTreeItem);
            }

            m_project.raceDataChanged();
            emit isDirtyChanged();
            emit layoutChanged();
            return;
        }
    }

    std::cerr << "No race found to be split" << std::endl;
}


bool RaceTreeModel::isRaceSelected() const {
    if (!m_selectedTreeIdx.isValid())
        return false;

    auto *pSelectedItem = static_cast<TreeItem*>(m_selectedTreeIdx.internalPointer());

    return pSelectedItem->isRace();
}

bool RaceTreeModel::isChapterSelected() const {
    if (!m_selectedTreeIdx.isValid())
        return false;

    return !isRaceSelected();
}

QString RaceTreeModel::getSelectedName() const{
    if (isRaceSelected()){
        auto *pSelectedItem = static_cast<TreeItem*>(m_selectedTreeIdx.internalPointer());
        return QString::fromStdString(pSelectedItem->getRaceData()->getName());
    }else if ( isChapterSelected() ) {
        auto *pSelectedItem = static_cast<TreeItem *>(m_selectedTreeIdx.internalPointer());
        return QString::fromStdString(pSelectedItem->getChapter()->getName());
    }else{
        return "Nothing selected";
    }
}

Chapter *RaceTreeModel::getSelectedChapter() const {
    if ( isChapterSelected() ) {
        auto *pSelectedItem = static_cast<TreeItem *>(m_selectedTreeIdx.internalPointer());
        return pSelectedItem->getChapter();
    }else{
        return nullptr;
    }
}

RaceData *RaceTreeModel::getSelectedRace() const {
    auto *pSelectedItem = static_cast<TreeItem *>(m_selectedTreeIdx.internalPointer());
    return pSelectedItem->getRaceData();
}

void RaceTreeModel::currentChanged(QModelIndex current, QModelIndex previous) {
    m_selectedTreeIdx = current;

    if (!current.isValid())
        return ;

    if ( previous.isValid()){
        std::cout << "previous: " << std::endl;
        std::cout << "row: " << previous.row() << " column: " << previous.column() << std::endl;
        auto *treeItem = static_cast<TreeItem*>(previous.internalPointer());
        auto race = treeItem->getRaceData();
        if ( treeItem->isRace() ){
            std::cout << "Race " << race->getName() << std::endl;
        }else{
            auto chapter = treeItem->getChapter();
            std::cout  << "Race " << race->getName() << "Chapter " << chapter->getName() << std::endl;
            emit chapterUnSelected(chapter->getUuid());
        }
    }

    m_pCurrentRace = static_cast<TreeItem*>(current.internalPointer())->getRaceData();
    std::cout << "currentChanged " << getSelectedName().toStdString() << std::endl;

    if ( isChapterSelected() ){
        // If the race has changed we need to emit raceSelected signal
        emit raceSelected(QString::fromStdString(m_pCurrentRace->getName()),
                          m_pCurrentRace->getStartIdx(), m_pCurrentRace->getEndIdx());

        // All chapter map elements are removed from the map now, so we need to add them back
        for( Chapter *chapter: m_pCurrentRace->getChapters() ){
            emit chapterAdded(chapter->getUuid(),QString::fromStdString(chapter->getName()),
                              chapter->getChapterType(),chapter->getStartIdx(),chapter->getEndIdx());
        }

        Chapter *chapter = getSelectedChapter();
        emit chapterSelected(chapter->getUuid(), QString::fromStdString(chapter->getName()),
                             chapter->getChapterType(), chapter->getStartIdx(),  chapter->getEndIdx(),
                             chapter->getGunIdx());
    }else{
        emit raceSelected(QString::fromStdString(m_pCurrentRace->getName()),
                          m_pCurrentRace->getStartIdx(), m_pCurrentRace->getEndIdx());
        // All chapter map elements are removed from the map now, so we need to add them back
        for( Chapter *chapter: m_pCurrentRace->getChapters() ){
            emit chapterAdded(chapter->getUuid(),QString::fromStdString(chapter->getName()),
                              chapter->getChapterType(),chapter->getStartIdx(),chapter->getEndIdx());
        }
    }

}



Q_INVOKABLE void RaceTreeModel::selectionChanged(QItemSelection selected, QItemSelection deselected){
    std::cout << "deselected: " << std::endl;

    QModelIndexList indexes = deselected.indexes();
    for (const QModelIndex &index : indexes) {
        std::cout << "row: " << index.row() << " column: " << index.column() << std::endl;
        auto *treeItem = static_cast<TreeItem*>(index.internalPointer());
        auto race = treeItem->getRaceData();
        if ( treeItem->isRace() ){
            std::cout << "Race " << race->getName() << std::endl;
        }else{
            auto chapter = treeItem->getChapter();
            std::cout  << "Race " << race->getName() << "Chapter " << chapter->getName() << std::endl;
        }
    }

}

void RaceTreeModel::makeRaceList() {

    // If project we just finished to read contains no races, create one
    if( m_RaceDataList.empty() ){
        QDateTime raceTime = QDateTime::fromMSecsSinceEpoch(qint64(m_InstrDataVector[0].utc.getUnixTimeMs()));
        std::string raceName = "Race " + raceTime.toString("yyyy-MM-dd hh:mm").toStdString();

        auto *race = new RaceData(0, m_InstrDataVector.size() - 1);
        race->SetName(raceName);

        m_RaceDataList.push_back(race);
    }

    m_pCurrentRace = m_RaceDataList.front();
}

QString RaceTreeModel::getTimeString(uint64_t idx) const{
    if ( idx < m_InstrDataVector.size() ) {
        auto dt = QDateTime::fromMSecsSinceEpoch(qint64(m_InstrDataVector[idx].utc.getUnixTimeMs()));
        QString tz = dt.timeZoneAbbreviation();
        return dt.toString("hh:mm:ss") + " " + tz;
    }else{
        return "??:??:??";
    }

}

void RaceTreeModel::handleProduceStarted() {
    emit produceStarted();
}

void RaceTreeModel::handleProduceFinished() {
    emit produceFinished();
}

void RaceTreeModel::makeEvents() {

    if (!m_selectedTreeIdx.isValid())
        return ;

    // Find selected race
    auto *item = static_cast<TreeItem*>(m_selectedTreeIdx.internalPointer());
    if ( !item->isRace() ){
        item = item->parentItem();
    }

    ChapterMaker chapterMaker(item);
    NavStats navStats(m_InstrDataVector, chapterMaker);
    emit layoutAboutToBeChanged();

    for( uint64_t i = m_pCurrentRace->getStartIdx(); i < m_pCurrentRace->getEndIdx(); i++){
        navStats.update(i, m_InstrDataVector[i]);
    }

    m_project.raceDataChanged();
    emit isDirtyChanged();
    emit layoutChanged();

}

ChapterMaker::ChapterMaker(TreeItem *raceTreeItem)
:m_pRaceTreeItem(raceTreeItem)
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
        m_tackCount++;
        chapterName = "Tack " + std::to_string(m_tackCount);
    }else{
        chapterName = "Gybe " + std::to_string(m_tackCount);
        m_gybeCount++;
    }

    std::cout << "insertChapter " << chapterName << " startIdx " << startIdx << " endIdx " << endIdx << std::endl;

    auto *chapter = new Chapter(startIdx, endIdx);
    chapter->SetName(chapterName);
    pRaceData->insertChapter(chapter);

    // Add data to the view model
    auto *chapterTreeItem = new TreeItem(pRaceData, chapter, m_pRaceTreeItem);
    m_pRaceTreeItem->insertChapterChild(chapterTreeItem);
}

void ChapterMaker::onMarkRounding(uint32_t startIdx, uint32_t endIdx, bool isWindward) {

}
