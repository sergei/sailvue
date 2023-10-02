
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

class TreeItem
{
public:
    explicit TreeItem(RaceData *m_pRaceData= nullptr, Chapter *m_pChapter = nullptr, TreeItem *parentItem = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);
    void insertChapterChild(TreeItem *child);
    void removeChild(TreeItem *child);
    void removeChildren(int i, int n);
    TreeItem *child(int row);
    [[nodiscard]] int childCount() const;
    static int columnCount() ;
    [[nodiscard]] QVariant data(int column) const;
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
        emit isDirtyChanged();
    }

    [[nodiscard]] QString projectName() const { return m_project.projectName(); }
    [[nodiscard]] bool isDirty() const { return m_project.isDirty(); }
    [[nodiscard]] QString goproPath() const{ return m_project.goproPath(); }
    [[nodiscard]] QString nmeaPath() const{ return m_project.nmeaPath(); }
    [[nodiscard]] QString polarPath() const{ return m_project.polarPath(); }
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
    Q_INVOKABLE void makeEvents();
    Q_INVOKABLE void updateChapter(const QString &uuid, const QString &chapterName, ChapterTypes::ChapterType chapterType,
                                   uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    Q_INVOKABLE void updateRace(const QString &raceName);
    Q_INVOKABLE void deleteSelected();

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

    void readData(const QString &goproDir, const QString &nmeaDir, const QString &polarFile, bool ignoreCache);
    void stop();
    void progressStatus(const QString &state, int progress);
    void fullPathReady(const QGeoPath fullPath);
    void loadFinished();
    void racePathIdxChanged(uint64_t racePathIdx);

    void chapterSelected(QString uuid, QString chapterName, ChapterTypes::ChapterType chapterType, uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    void chapterUnSelected(QString uuid);
    void chapterAdded(QString uuid, QString chapterName, ChapterTypes::ChapterType chapterType, uint64_t startIdx, uint64_t endIdx, uint64_t gunIdx);
    void chapterDeleted(QString uuid);

    void raceSelected(QString raceName, uint64_t startIdx, uint64_t endIdx);
    void raceUnSelected();

    // Production related signals
    void produce(const QString &produceFolder, const QString &polarFile);
    void produceStarted();
    void produceFinished();

#pragma clang diagnostic pop

private:
    void makeRaceList();

private:
    Project m_project;
    TreeItem *rootItem;
    QThread workerThread;
    QGeoPath m_geoPath;

    std::list<GoProClipInfo> m_GoProClipInfoList;
    std::vector<InstrumentInput> m_InstrDataVector;
    std::list<RaceData *> m_RaceDataList;

    QModelIndex m_selectedTreeIdx = QModelIndex();

    RaceData *m_pCurrentRace = nullptr;
    uint64_t m_ulCurrentInstrDataIdx = 0;

    void deleteAllRaces();
};

class ChapterMaker : public NavStatsEventsListener {
public:
    explicit ChapterMaker(TreeItem *raceTreeItem);
    void onTack(uint32_t fromIdx, uint32_t toIdx, bool isTack, double distLossMeters) override;
    void onMarkRounding(uint32_t eventIdx, uint32_t fromIdx, uint32_t toIdx, bool isWindward) override;
private:
    TreeItem *m_pRaceTreeItem;
};


#endif //SAILVUE_RACETREEMODEL_H
