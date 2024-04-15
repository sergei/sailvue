#ifndef SAILVUE_WORKER_H
#define SAILVUE_WORKER_H

#include <map>
#include <QObject>
#include <iostream>
#include <QGeoPath>
#include "navcomputer/IProgressListener.h"
#include "gopro/GoPro.h"
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/RaceData.h"
#include "navcomputer/Performance.h"

class Worker : public QObject, IProgressListener  {
    Q_OBJECT

public:
    explicit Worker(std::list<GoProClipInfo> &rGoProClipInfoList,
                    std::vector<InstrumentInput> &rInstrDataVector,
                    std::map<uint64_t, Performance> &rPerformanceMap,
                    std::list<RaceData *> &raceDataList)
                    : m_rGoProClipInfoList(rGoProClipInfoList),
                      m_rInstrDataVector(rInstrDataVector),
                      m_rPerformanceMap(rPerformanceMap),
                      m_RaceDataList(raceDataList)
                    {}
    void progress(const std::string& state, int progress) override;
    bool stopRequested() override;

public slots:
    void readData(const QString &goproDir, const QString &logsType, const QString &nmeaDir, const QString &polarFile, bool bIgnoreCache);
    void stopWork() ;
    void produce(const QString &moviePathUrl, const QString &polarUrl);
    void exportStats(const QString &polarUrl, const QString &path);
    void exportGpx(const QString &path);
signals:
    void ProgressStatus(const QString &state, int progress);
    void pathAvailable();

    // Production related signals
    void produceStarted();
    void produceFinished(const QString &message);

private:
    bool b_keepRunning = true;
    std::list<GoProClipInfo> &m_rGoProClipInfoList;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::map<uint64_t, Performance> &m_rPerformanceMap;
    std::list<RaceData *> &m_RaceDataList;

    void computeStats(const QString &polarUrl);
};


#endif //SAILVUE_WORKER_H
