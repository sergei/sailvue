#include "movie/MovieProducer.h" // WTF Should be the first file otherwise the Eigen compilation fails

#include <QCoreApplication>
#include <QUrl>
#include <qsettings.h>
#include "Worker.h"
#include "n2k/YdvrReader.h"
#include "gopro/GoPro.h"
#include "PgnSrcTreeModel.h"

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

    MovieProducer movieProducer(moviePath, polarPath, m_rGoProClipInfoList, m_rInstrDataVector, m_RaceDataList, *this);
    movieProducer.produce();

    emit produceFinished();
}
