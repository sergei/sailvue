#include "movie/MovieProducer.h" // WTF Should be the first file otherwise the Eigen compilation fails

#include <QCoreApplication>
#include <QUrl>
#include <qsettings.h>
#include "Worker.h"
#include "n2k/YdvrReader.h"
#include "gopro/GoPro.h"
#include "PgnSrcTreeModel.h"
#include "Settings.h"
#include "navcomputer/TimeDeltaComputer.h"

void Worker::readData(const QString &goproDir, const QString &nmeaDir, const QString &polarFile, bool bIgnoreCache){
    std::cout << "goproDir " + goproDir.toStdString() << std::endl;
    std::cout << "nmeaDir " + nmeaDir.toStdString() << std::endl;
    std::cout << "polarFile " + polarFile.toStdString() << std::endl;

    /* ... here is the expensive or blocking operation ... */
    std::string stYdvrDir = QUrl(nmeaDir).toLocalFile().toStdString();
    std::string stGoProDir = QUrl(goproDir).toLocalFile().toStdString();
    std::string stCacheDir = "/tmp/sailvue";
    bool bSummaryOnly = false;
    bool bMappingOnly = false;

    QSettings settings;
    QString pgnSrcCsvPath = settings.value(SETTINGS_KEY_PGN_CSV, "").toString();
    std::string stPgnSrcCsv;
    if ( !pgnSrcCsvPath.isEmpty() ){
        stPgnSrcCsv = QUrl(pgnSrcCsvPath).toLocalFile().toStdString();
    } else {
        std::cerr << "No PGN sources file found" << std::endl;
        return;
    }

    if ( bIgnoreCache ){
        std::cout << "Deleting cached files" << std::endl;
        std::filesystem::remove_all(stCacheDir);
    }

    YdvrReader ydvrReader(stYdvrDir, stCacheDir, stPgnSrcCsv, bSummaryOnly, bMappingOnly,  *this);
    GoPro goPro(stGoProDir, stCacheDir, ydvrReader, *this);

    // Create path containing points from all gopro clips
    int clipCount = 0;
    int pointsCount = 0;
    m_rGoProClipInfoList.clear();
    m_rInstrDataVector.clear();
    for (const auto& clip : goPro.getGoProClipList()) {
        m_rGoProClipInfoList.push_back(clip);
        clipCount++;
        for( auto &ii : *clip.getInstrData()) {
            m_rInstrDataVector.push_back(ii);
            pointsCount++;
        }
    }

    // Make performance vector the same size as the instr data vector
    m_rPerformanceVector.clear();
    m_rPerformanceVector.resize(m_rInstrDataVector.size());

    std::cout << "clipCount " << clipCount << std::endl;
    std::cout << "pointsCount " << pointsCount << std::endl;

    emit pathAvailable();

}

void Worker::stopWork() {
    std::cout << "stopWork " << std::endl;
    b_keepRunning = false;
}

void Worker::progress(const std::string &state, int progress) {
    emit ProgressStatus(QString::fromStdString(state), progress);
}

bool Worker::stopRequested() {
    QCoreApplication::processEvents();
    return !b_keepRunning;
}

void Worker::produce(const QString &moviePathUrl, const QString &polarUrl) {
    std::cout << "produce " << moviePathUrl.toStdString() << std::endl;
    std::string moviePath = QUrl(moviePathUrl).toLocalFile().toStdString();
    std::string polarPath = QUrl(polarUrl).toLocalFile().toStdString();


    emit produceStarted();

    computeStats(polarUrl);

    MovieProducer movieProducer(moviePath, polarPath, m_rGoProClipInfoList, m_rInstrDataVector, m_rPerformanceVector,
                                m_RaceDataList, *this);

    movieProducer.produce();

    emit produceFinished();
}


void Worker::computeStats(const QString &polarUrl){
    Polars polars;
    polars.loadPolar(QUrl(polarUrl).toLocalFile().toStdString());

    m_rPerformanceVector.clear();
    m_rPerformanceVector.resize(m_rInstrDataVector.size());

    int raceIdx = 0;
    TimeDeltaComputer timeDeltaComputer(polars, m_rInstrDataVector);
    for ( RaceData *race: m_RaceDataList) {
        timeDeltaComputer.startRace();
        int chapterIdx = 0;
        for(auto it = race->getChapters().begin(); it != race->getChapters().end(); it++, chapterIdx++) {
            Chapter *chapter = *it;

            auto nextIt = it;
            auto prevIt = it;
            nextIt++;
            bool isFetch = false;
            if ( it!=race->getChapters().begin() &&  nextIt != race->getChapters().end()){
                prevIt--;
                isFetch = ((*prevIt)->getChapterType() == ChapterTypes::START || (*prevIt)->getChapterType() == ChapterTypes::MARK_ROUNDING)
                        && (*nextIt)->getChapterType() == ChapterTypes::MARK_ROUNDING;
            }
            timeDeltaComputer.startLeg();
            for( uint64_t idx= chapter->getStartIdx(); idx < chapter->getEndIdx(); idx++){
                m_rPerformanceVector[idx].raceIdx = raceIdx;
                m_rPerformanceVector[idx].legIdx = chapterIdx;
                bool beforeStart = chapter->getChapterType() == ChapterTypes::ChapterType::START && idx < chapter->getGunIdx();
                if ( beforeStart ){
                    m_rPerformanceVector[idx].isValid = false;
                    m_rPerformanceVector[idx].legDistLostToTargetMeters = 0;
                    m_rPerformanceVector[idx].legTimeLostToTargetSec = 0;
                }else{
                    timeDeltaComputer.updatePerformance(idx, m_rPerformanceVector[idx], isFetch);
                }
            }
        }
        raceIdx ++;
    }

}


void Worker::exportStats(const QString &polarUrl, const QString &path) {
    computeStats(polarUrl);

    // Store stats as a CSV file
    std::string csvName = QUrl(path).toLocalFile().toStdString();
    std::cout << "exporting stats to  " << csvName << std::endl;
    std::ofstream ofs(csvName);

    for(uint64_t idx = 0; idx < m_rPerformanceVector.size(); idx++){
        ofs <<  std::string(m_rInstrDataVector[idx]) << ",";
        ofs << m_rPerformanceVector[idx].toString(m_rInstrDataVector[idx].utc) << ",";
        ofs << std::endl;
    }

    ofs.close();
}
