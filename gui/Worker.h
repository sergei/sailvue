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
#include "cameras/CameraBase.h"

class Worker : public QObject, IProgressListener  {
    Q_OBJECT

public:
    explicit Worker(std::list<GoProClipInfo> &rGoProClipInfoList,
                    std::list<CameraClipInfo *> &rCameraClipsList,
                    std::vector<InstrumentInput> &rInstrDataVector,
                    std::map<uint64_t, Performance> &rPerformanceMap,
                    std::list<RaceData *> &raceDataList)
                    : m_rGoProClipInfoList(rGoProClipInfoList),
                      m_rCameraClipsList(rCameraClipsList),
                      m_rInstrDataVector(rInstrDataVector),
                      m_rPerformanceMap(rPerformanceMap),
                      m_RaceDataList(raceDataList)
                    {}
    void progress(const std::string& state, int progress) override;
    bool stopRequested() override;

public slots:
    void readData(const QString &goproDir, const QString &insta360Dir, const QString &adobeMarkersDir, const QString &logsType, const QString &nmeaDir, const QString &polarFile, bool bIgnoreCache);
    void stopWork() ;
    void produce(const QString &moviePathUrl, const QString &polarUrl);
    void exportStats(const QString &polarUrl, const QString &path);
    void exportGpx(const QString &path);
signals:
    void ProgressStatus(const QString &state, int progress);
    void pathAvailable();

    // Production related signals
    void produceStarted();
    void markersImported();
    void produceFinished(const QString &message);

private:
    bool b_keepRunning = true;
    std::list<GoProClipInfo> &m_rGoProClipInfoList;
    std::list<CameraClipInfo *> &m_rCameraClipsList;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::map<uint64_t, Performance> &m_rPerformanceMap;
    std::list<RaceData *> &m_RaceDataList;
    CameraBase *m_pCamera = nullptr;

    void computeStats(const QString &polarUrl);
};


#endif //SAILVUE_WORKER_H
