
#ifndef SAILVUE_RACETREEMODEL_H
#define SAILVUE_RACETREEMODEL_H

#include <qqml.h>
#include <QAbstractItemModel>
#include <QThread>
#include <QGeoPath>
#include <QDateTime>
#include <QItemSelection>
#include "gopro/GoPro.h"
#include "navcomputer/RaceData.h"
#include "Project.h"
#include "navcomputer/NavStatsEventsListener.h"
#include "navcomputer/Performance.h"

class TreeItem
{
public:
    explicit TreeItem(RaceData *m_pRaceData= nullptr, Chapter *m_pChapter = nullptr, TreeItem *parentItem = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);
    int insertChapterChild(TreeItem *child);
    void removeChild(TreeItem *child);
    void removeChildren(int i, int n);
    TreeItem *child(int row);
    [[nodiscard]] int childCount() const;
    static int columnCount() ;
    [[nodiscard]] QVariant data(int column) const;
    bool setData(const QVariant &value, int column);
    [[nodiscard]] int row() const;
    TreeItem *parentItem();

    [[nodiscard]] bool isRace() const { return m_pChapter == nullptr; }
    [[nodiscard]] RaceData *getRaceData() ;
    [[nodiscard]] Chapter *getChapter() ;



private:
    QList<TreeItem *> m_childItems;
    TreeItem *m_parentItem;
    RaceData *m_pRaceData = nullptr;
    Chapter *m_pChapter = nullptr;
};

class RaceTreeModel  : public QAbstractItemModel {
    Q_OBJECT

    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

    Q_PROPERTY(QString projectName READ projectName NOTIFY projectNameChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)
    Q_PROPERTY(QString goproPath READ goproPath WRITE setGoProPath NOTIFY goProPathChanged)
    Q_PROPERTY(QString nmeaPath READ nmeaPath WRITE setNmeaPath NOTIFY nmeaPathChanged)
    Q_PROPERTY(QString polarPath READ polarPath WRITE setPolarPath NOTIFY polarPathChanged)
    Q_PROPERTY(float twaOffset READ twaOffset WRITE setTwaOffset NOTIFY twaOffsetChanged)


public:
// Required QAbstractItemModel methods
    explicit RaceTreeModel(QObject *parent = nullptr);
    ~RaceTreeModel() override;

    Q_INVOKABLE [[nodiscard]] QModelIndex index(int row, int column,
                                          const QModelIndex &parent) const override;

    Q_INVOKABLE  [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;


    [[nodiscard]] int rowCount(const QModelIndex &) const override;

    [[nodiscard]] int columnCount(const QModelIndex &) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]]  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override
    {
        return { {Qt::DisplayRole, "display"} };
    }

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

// QAbstractItemModel Class extension

    void setProjectName(const QString &name){
        m_project.setProjectName(name);
        emit projectNameChanged();
        emit isDirtyChanged();
    }

    void setGoProPath(const QString &path){
        m_project.setGoProPath(path);
        emit goProPathChanged();
        emit isDirtyChanged();
    }

    void setNmeaPath(const QString &path){
        m_project.setNmeaPath(path);
        emit nmeaPathChanged();
        emit isDirtyChanged();
    }

    void setPolarPath(const QString &path){
        m_project.setPolarPath(path);
        emit polarPathChanged();
    }

    void setTwaOffset(const double twaOffset){
        m_project.setTwaOffset(twaOffset);
        emit twaOffsetChanged();
    }

    [[nodiscard]] QString projectName() const { return m_project.projectName(); }
    [[nodiscard]] bool isDirty() const { return m_project.isDirty(); }
    [[nodiscard]] QString goproPath() const{ return m_project.goproPath(); }
    [[nodiscard]] QString nmeaPath() const{ return m_project.nmeaPath(); }
    [[nodiscard]] QString polarPath() const{ return m_project.polarPath(); }
    [[nodiscard]] double twaOffset() const{ return m_project.twaOffset(); }

    Q_INVOKABLE void setSelectionModel(QItemSelectionModel *selectionModel) { m_selectionModel = selectionModel;}
    Q_INVOKABLE void load(const QString &path);
    Q_INVOKABLE void read(bool ignoreCache);
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveAs(const QString &path);

    Q_INVOKABLE void showRaceData();
    Q_INVOKABLE void stopDataProcessing();
    Q_INVOKABLE void seekToRacePathIdx(uint64_t idx);
    Q_INVOKABLE void currentChanged(QModelIndex current, QModelIndex previous);
    Q_INVOKABLE void selectionChanged(QItemSelection selected, QItemSelection deselected);

    Q_INVOKABLE void splitRace();
    Q_INVOKABLE void addChapter();
    Q_INVOKABLE void detectManeuvers();
    Q_INVOKABLE void makeAnalytics();
    Q_INVOKABLE void updateChapter(const QString &uuid, const QString &chapterName, ChapterTypes::ChapterType chapterType,
                                   uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    Q_INVOKABLE void updateChapterStartIdx(uint64_t idx);
    Q_INVOKABLE void updateChapterEndIdx(uint64_t idx);
    Q_INVOKABLE void updateChapterGunIdx(uint64_t idx);
    Q_INVOKABLE void updateRace(const QString &raceName);

    Q_INVOKABLE [[nodiscard]] bool isRaceSelected() const ;
    Q_INVOKABLE [[nodiscard]] bool isChapterSelected() const;
    Q_INVOKABLE [[nodiscard]] QString getSelectedName() const;
    Q_INVOKABLE [[nodiscard]] QString getTimeString(uint64_t idx) const;

    std::list<GoProClipInfo> *getClipList()  {return &m_GoProClipInfoList;};

    [[nodiscard]] Chapter *getSelectedChapter() const;
    [[nodiscard]] RaceData *getSelectedRace() const;
    [[nodiscard]] uint64_t getUtcForIdx(qint64 idx) const {return m_InstrDataVector[idx].utc.getUnixTimeMs();};
    qint64 getIdxForUtc(uint64_t ulUtcMs);


public slots:
    void handleNewInstrDataVector();
    void handleProgress(const QString &state, int progress);

    void handleProduceStarted();
    void handleProduceFinished();

signals:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
    void goProPathChanged();
    void isDirtyChanged();
    void projectNameChanged();
    void nmeaPathChanged();
    void polarPathChanged();
    void twaOffsetChanged();

    void readData(const QString &goproDir, const QString &nmeaDir, const QString &polarFile, bool ignoreCache);
    void stop();
    void progressStatus(const QString &state, int progress);
    void fullPathReady(const QGeoPath fullPath);
    void loadStarted();
    void loadFinished();
    void racePathIdxChanged(uint64_t racePathIdx);

    void chapterSelected(QString uuid, QString chapterName, ChapterTypes::ChapterType chapterType, uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    void chapterUnSelected(QString uuid);
    void chapterAdded(QString uuid, QString chapterName, ChapterTypes::ChapterType chapterType, uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    void chapterUpdated(QString uuid, QString chapterName, ChapterTypes::ChapterType chapterType, uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    void chapterDeleted(QString uuid);

    void raceSelected(QString raceName, uint64_t startIdx, uint64_t endIdx);
    void raceUnSelected();

    // Production related signals
    void produce(const QString &produceFolder, const QString &polarFile);
    void produceStarted();
    void produceFinished();

    void exportStats(const QString &polarUrl, const QString &path);
    void beginSimulation(const QString &addr, uint16_t port);
    void endSimulation();

#pragma clang diagnostic pop

private:
    void makeRaceList();

private:
    Project m_project;
    TreeItem *rootItem;
    QThread workerThread;
    QThread m_netSimThread;
    QGeoPath m_geoPath;

    std::list<GoProClipInfo> m_GoProClipInfoList;
    std::vector<InstrumentInput> m_InstrDataVector;
    std::vector<Performance> m_PerformanceVector;

    std::list<RaceData *> m_RaceDataList;

    QModelIndex m_selectedTreeIdx = QModelIndex();

    RaceData *m_pCurrentRace = nullptr;
    uint64_t m_ulCurrentInstrDataIdx = 0;
    QItemSelectionModel *m_selectionModel = nullptr;

    void deleteItem(QModelIndex itemIndex);
    void deleteAllRaces();
    void computeStats();
    void startNetworkSimulator();
    void selectFirstChapter();
};


#endif //SAILVUE_RACETREEMODEL_H
