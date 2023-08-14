#ifndef SAILVUE_WORKER_H
#define SAILVUE_WORKER_H


#include <QObject>
#include <iostream>
#include <QGeoPath>
#include "navcomputer/IProgressListener.h"
#include "gopro/GoPro.h"
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/RaceData.h"

class Worker : public QObject, IProgressListener  {
    Q_OBJECT

public:
    explicit Worker(std::list<GoProClipInfo> &rGoProClipInfoList,
                    std::vector<InstrumentInput> &rInstrDataVector,
                    std::list<RaceData *> &raceDataList)
                    : m_rGoProClipInfoList(rGoProClipInfoList),
                    m_rInstrDataVector(rInstrDataVector),
                    m_RaceDataList(raceDataList)
                    {}
    void progress(const std::string& state, int progress) override;
    bool stopRequested() override;

public slots:
    void readData(const QString &goproDir, const QString &nmeaDir, const QString &polarFile, bool bIgnoreCache);
    void stopWork() ;
    void produce(const QString &path);
signals:
    void ProgressStatus(const QString &state, int progress);
    void pathAvailable();

    // Production related signals
    void produceStarted();
    void produceFinished();

private:
    bool b_keepRunning = true;
    std::list<GoProClipInfo> &m_rGoProClipInfoList;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::list<RaceData *> &m_RaceDataList;
};


#endif //SAILVUE_WORKER_H
