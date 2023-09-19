#include "n2k/YdvrReader.h"
#include "PgnSrcTreeModel.h"

PgnTreeItem *PgnTreeItem::child(int row) const {
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int PgnTreeItem::row() const
{
    if (m_parentItem)
        return int(m_parentItem->m_childItems.indexOf(const_cast<PgnTreeItem*>(this)));
    return 0;
}

QVariant PgnTreeItem::data(int column) const {
    if (column < 0 || column >= 1)
        return {};  // Have only one column

    if ( isPgn  ){
        std::string src = "None";
        const PgnTreeItem *selectedSource = child(m_sourceIdx);
        if ( selectedSource != nullptr){
            src = selectedSource->desc;
        }
        std::string txt = desc + " (PGN:" + std::to_string(pgnNum) + ") - " + src;
        return QString::fromStdString(txt );
    }else {
        return QString::fromStdString(desc);
    }
}

PgnSrcTreeModel::PgnSrcTreeModel(QObject *parent)
:QAbstractItemModel(parent) {
    rootItem = new PgnTreeItem("root", nullptr);

    auto *worker = new PgnListWorker(m_mapPgnDevices, m_mapPgnDescription);
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    connect(this, &PgnSrcTreeModel::readData, worker, &PgnListWorker::readData);
    connect(this, &PgnSrcTreeModel::stop, worker, &PgnListWorker::stopWork);
    connect(worker, &PgnListWorker::ProgressStatus, this, &PgnSrcTreeModel::handleProgress);
    connect(worker, &PgnListWorker::DataAvailable, this, &PgnSrcTreeModel::handleDataAvailable);

    workerThread.start();


    QSettings settings;
    QString pgnSrcCsvPath = settings.value(SETTINGS_KEY_PGN_CSV, "").toString();
    if ( pgnSrcCsvPath != "" ){
        loadData(pgnSrcCsvPath);
    }
}

PgnSrcTreeModel::~PgnSrcTreeModel(){
    workerThread.quit();
    workerThread.wait();

    delete rootItem;
}

QModelIndex PgnSrcTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return {};

    PgnTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PgnTreeItem*>(parent.internalPointer());

    PgnTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return {};
}
QModelIndex PgnSrcTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto *childItem = static_cast<PgnTreeItem*>(index.internalPointer());
    if ( childItem == nullptr )
        return {};

    PgnTreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}
int PgnSrcTreeModel::rowCount(const QModelIndex &parent) const
{
    PgnTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PgnTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}
int PgnSrcTreeModel::columnCount(const QModelIndex &parent) const
{
    int i;
    if (parent.isValid()){
        i = static_cast<PgnTreeItem*>(parent.internalPointer())->columnCount();
    }else{
        i = rootItem->columnCount();
    }

    return i;
}
QVariant PgnSrcTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role != Qt::DisplayRole)
        return {};

    auto *item = static_cast<PgnTreeItem*>(index.internalPointer());

    return item->data(index.column());
}
Qt::ItemFlags PgnSrcTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}
QVariant PgnSrcTreeModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return {};
}

void PgnSrcTreeModel::handleDataAvailable() {
    for( auto & pgnDevLst : m_mapPgnDevices ) {
        auto pgn = pgnDevLst.first;
        m_mapPgnDeviceIdx[pgn] = 0;  // Set to the first device in the list
    }
    emit dataReadComplete();
}

void PgnSrcTreeModel::createModel() {
    emit layoutAboutToBeChanged();

    delete rootItem;
    rootItem = new PgnTreeItem("root", nullptr);

    for( auto & pgnDevLst : m_mapPgnDevices ) {
        auto pgn = pgnDevLst.first;
        auto pgnDesc = m_mapPgnDescription[pgn];

        auto pgnItem = new PgnTreeItem(int(pgn), pgnDesc, rootItem);

        int idx = 0;
        for (auto& devName : pgnDevLst.second){
            auto srcItem = new PgnTreeItem(devName, pgnItem);
            pgnItem->appendChild(srcItem);
            if ( idx == m_mapPgnDeviceIdx[pgn] ){
                pgnItem->m_sourceIdx = idx;
            }
            idx ++;
        }
        rootItem->appendChild(pgnItem);
    }

    emit layoutChanged();
}

void PgnSrcTreeModel::currentChanged(QModelIndex current, QModelIndex previous) {
    if (!current.isValid())
        return ;


    auto item = static_cast<PgnTreeItem *>(current.internalPointer());

    if (!item->isPgn){
        emit layoutAboutToBeChanged();
        PgnTreeItem * pgnItem = item->parentItem();
        pgnItem->m_sourceIdx = item->row();
        m_mapPgnDeviceIdx[pgnItem->pgnNum] = pgnItem->m_sourceIdx;
        emit layoutChanged();
        m_isDirty = true;
        emit isDirtyChanged();
    }

}

void PgnSrcTreeModel::loadData(const QString &pgnSrcCsvPath) {
    std::cout << "loadData " << pgnSrcCsvPath.toStdString() << std::endl;
    std::string stCsvFile = QUrl(pgnSrcCsvPath).toLocalFile().toStdString();
    std::ifstream pgnsSrcsFileStream (stCsvFile, std::ios::in);

    m_mapPgnDeviceIdx.clear();
    m_mapPgnDevices.clear();
    m_mapPgnDescription.clear();

    std::string line;
    while (std::getline(pgnsSrcsFileStream, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        if ( tokens.size() > 3 ){
            uint32_t pgn = std::stoul(tokens[0]);
            std::string desc = tokens[1];
            std::vector<std::string> srcs;
            uint32_t defaultSrcIdx = std::stoul(tokens[2]);
            m_mapPgnDeviceIdx[pgn] = defaultSrcIdx;
            for (int i = 3; i < tokens.size(); i++){
                srcs.push_back(tokens[i]);
            }
            m_mapPgnDevices[pgn] = srcs;
            m_mapPgnDescription[pgn] = desc;
        }
    }
    pgnsSrcsFileStream.close();
    setPgnSrcCsvPath(pgnSrcCsvPath);
    createModel();
}

void PgnListWorker::readData(const QString &nmeaDir, const QString &pgnSrcCsvPath){
    std::cout << "nmeaDir " + nmeaDir.toStdString() << std::endl;
    b_keepRunning = true;

    std::string stYdvrDir = QUrl(nmeaDir).toLocalFile().toStdString();

    std::string stCacheDir = "/tmp/sailvue";
    bool bSummaryOnly = true;
    bool bMappingOnly = true;

    YdvrReader ydvrReader(stYdvrDir, stCacheDir, "", bSummaryOnly, bMappingOnly, *this);

    // Get data
    ydvrReader.getPgnData(m_mapPgnDevices, m_mapPgnDescription);

    // Store data to CSV file
    std::string stCsvFile = QUrl(pgnSrcCsvPath).toLocalFile().toStdString();
    std::ofstream pgnsSrcsFileStream (stCsvFile, std::ios::out);

    for( auto & pgnDevLst : m_mapPgnDevices ){
        auto pgn = pgnDevLst.first;
        auto pgnDesc = m_mapPgnDescription[pgn];
        pgnsSrcsFileStream << pgn << "," << pgnDesc  << ",0";  // Set the first source as default
        for (auto& devName : pgnDevLst.second){
            pgnsSrcsFileStream << "," << devName;
        }
        pgnsSrcsFileStream << std::endl;
    }

    pgnsSrcsFileStream.close();
    std::cout << "Created new PGN sources file " << stCsvFile << std::endl;

    emit DataAvailable();
}


void PgnSrcTreeModel::saveData() {
    // Store data to CSV file
    std::string stCsvFile = QUrl(m_pgnSrcCsvPath).toLocalFile().toStdString();
    std::ofstream pgnsSrcsFileStream (stCsvFile, std::ios::out);

    for( auto & pgnDevLst : m_mapPgnDevices ){
        auto pgn = pgnDevLst.first;
        auto pgnDesc = m_mapPgnDescription[pgn];
        pgnsSrcsFileStream << pgn << "," << pgnDesc  << "," << m_mapPgnDeviceIdx[pgn];
        for (auto& devName : pgnDevLst.second){
            pgnsSrcsFileStream << "," << devName;
        }
        pgnsSrcsFileStream << std::endl;
    }

    pgnsSrcsFileStream.close();
    std::cout << "Saved PGN sources file " << stCsvFile << std::endl;

    QSettings settings;
    settings.setValue(SETTINGS_KEY_PGN_CSV, m_pgnSrcCsvPath);

    m_isDirty = false;
    emit isDirtyChanged();
}


void PgnListWorker::stopWork()
{
    std::cout << "stopWork " << std::endl;
    b_keepRunning = false;
}

