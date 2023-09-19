#ifndef SAILVUE_PGNSRCTREEMODEL_H
#define SAILVUE_PGNSRCTREEMODEL_H

#include <QVariant>
#include <QAbstractItemModel>
#include <QtQmlIntegration>
#include <utility>
#include "navcomputer/IProgressListener.h"

static const char *const SETTINGS_KEY_PGN_CSV = "pgnSrcCsvPath";

class PgnTreeItem
{
public:
    PgnTreeItem(int pgn, std::string desc, PgnTreeItem *parent):isPgn(true),pgnNum(pgn), desc(std::move(desc)), m_parentItem(parent) {}
    PgnTreeItem(std::string desc, PgnTreeItem *parent):isPgn(false), pgnNum(-1), desc(std::move(desc)), m_parentItem(parent){}
    ~PgnTreeItem(){qDeleteAll(m_childItems);};

    void appendChild(PgnTreeItem *child){m_childItems.append(child); }
    PgnTreeItem *child(int row) const;
    [[nodiscard]] int childCount() const {return int(m_childItems.count());}
    static int columnCount() {return 1;};
    [[nodiscard]] QVariant data(int column) const;
    [[nodiscard]] int row() const;
    PgnTreeItem *parentItem(){return m_parentItem;};

    int m_sourceIdx = 0;
    bool isPgn;
    int pgnNum;
private:
    QList<PgnTreeItem *> m_childItems;
    PgnTreeItem *m_parentItem;
    std::string desc;
};


class PgnSrcTreeModel : public QAbstractItemModel {
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

    Q_PROPERTY(QString pgnSrcCsvPath READ pgnSrcCsvPath WRITE setPgnSrcCsvPath NOTIFY pgnSrcCsvPathChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)

public:

// Required QAbstractItemModel methods
    explicit PgnSrcTreeModel(QObject *parent = nullptr);
    ~PgnSrcTreeModel() override;

    Q_INVOKABLE [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    Q_INVOKABLE  [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int rowCount(const QModelIndex &) const override;
    [[nodiscard]] int columnCount(const QModelIndex &) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override
    {
        return { {Qt::DisplayRole, "display"} };
    }

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    [[nodiscard]] QString pgnSrcCsvPath() const{ return m_pgnSrcCsvPath; }
    void setPgnSrcCsvPath(const QString &path){
        m_pgnSrcCsvPath = path;
        emit pgnSrcCsvPathChanged();
    }

    [[nodiscard]] PgnTreeItem *getRootItem() const { return rootItem; }

    // Class extension methods
    Q_INVOKABLE void currentChanged(QModelIndex current, QModelIndex previous);
    [[nodiscard]] bool isDirty() const { return m_isDirty; }


    void handleProgress(const QString &state, int progress) {
        emit progressStatus(state, progress);
    }

public slots:
    void handleDataAvailable();
    void loadData(const QString &pgnSrcCsvPath);
    void saveData();

signals:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
    void isDirtyChanged();
    void pgnSrcCsvPathChanged();
    void readData(const QString &nmeaDir, const QString &pgnSrcCsvPath);
    void stop();
    void progressStatus(const QString &state, int progress);
    void dataReadComplete();
#pragma clang diagnostic pop

private:
    PgnTreeItem *rootItem;
    QString m_pgnSrcCsvPath;
    QThread workerThread;

    std::map<uint32_t, std::vector<std::string>> m_mapPgnDevices;
    std::map<uint32_t, std::string> m_mapPgnDescription;
    std::map<uint32_t, uint32_t > m_mapPgnDeviceIdx;
    bool m_isDirty = false;

    void createModel();
};

class PgnListWorker : public QObject, IProgressListener  {
Q_OBJECT

public:
    explicit PgnListWorker(std::map<uint32_t, std::vector<std::string>> &mapPgnDevices,
                           std::map<uint32_t, std::string> &mapPgnDescription)
            : m_mapPgnDevices(mapPgnDevices),
              m_mapPgnDescription(mapPgnDescription)
    {}
    void progress(const std::string& state, int progress) override {emit ProgressStatus(QString::fromStdString(state), progress);};
    bool stopRequested() override {QCoreApplication::processEvents(); return !b_keepRunning;}

public slots:
    void readData(const QString &nmeaDir, const QString &pgnSrcCsvPath);
    void stopWork();

signals:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
    void ProgressStatus(const QString &state, int progress);
    void DataAvailable();
#pragma clang diagnostic pop

private:
    bool b_keepRunning = true;
    std::map<uint32_t, std::vector<std::string>> &m_mapPgnDevices;
    std::map<uint32_t, std::string> &m_mapPgnDescription;
};


#endif //SAILVUE_PGNSRCTREEMODEL_H
