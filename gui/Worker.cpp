#include "movie/MovieProducer.h" // WTF Should be the first file otherwise the Eigen compilation fails

#include <QCoreApplication>
#include <QUrl>
#include <qsettings.h>
#include "Worker.h"
#include "n2k/YdvrReader.h"
#include "gopro/GoPro.h"
#include "Settings.h"
#include "navcomputer/TimeDeltaComputer.h"
#include "Project.h"
#include "navcomputer/Calibration.h"
#include "n2k/ExpeditionReader.h"
#include "adobe_premiere/MarkerReader.h"

void Worker::readData(const QString &goproDir, const QString &insta360Dir, const QString &adobeMarkersDir, const QString &logsType, const QString &nmeaDir, const QString &polarFile, bool bIgnoreCache){
    std::cout << "goproDir " + goproDir.toStdString() << std::endl;
    std::cout << "logsType " + nmeaDir.toStdString() << std::endl;
    std::cout << "nmeaDir " + nmeaDir.toStdString() << std::endl;
    std::cout << "polarFile " + polarFile.toStdString() << std::endl;

    std::string stYdvrDir = QUrl(nmeaDir).toLocalFile().toStdString();
    std::string stGoProDir = QUrl(goproDir).toLocalFile().toStdString();

    const std::string &stAdobeMarkersDir = QUrl(adobeMarkersDir).toLocalFile().toStdString();
    const std::string &stInsta360Dir = QUrl(insta360Dir).toLocalFile().toStdString();

    std::string stCacheDir = "/tmp/sailvue";

    bool bSummaryOnly = false;
    bool bMappingOnly = false;

    QSettings settings;
    QString pgnSrcCsvPath = settings.value(SETTINGS_KEY_PGN_CSV, "").toString();
    std::string stPgnSrcCsv;

    double twaOffset = Project::twaOffset();
    Calibration calibration(twaOffset);

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

    InstrDataReader * reader = nullptr;
    bool useExpedition = logsType == "EXPEDITION";
    if ( useExpedition ){
        ExpeditionReader *expReader = new ExpeditionReader(stYdvrDir, stCacheDir,   *this);
        reader = expReader;
    }else{
        YdvrReader *ydvrReader = new YdvrReader(stYdvrDir, stCacheDir, stPgnSrcCsv, bSummaryOnly, bMappingOnly,  *this);
        reader = ydvrReader;
    }

    int pointsCount = 0;
    int clipCount = 0;
    if ( ! stGoProDir.empty() ){
        GoPro goPro(stGoProDir, stCacheDir, *reader, *this);

        // Create path containing points from all gopro clips
        m_rGoProClipInfoList.clear();
        m_rInstrDataVector.clear();
        for (auto& clip : goPro.getGoProClipList()) {
            m_rGoProClipInfoList.push_back(clip);
            clipCount++;
            for( auto &ii : *clip.getInstrData()) {
                calibration.calibrate(ii);
                m_rInstrDataVector.push_back(ii);
                pointsCount++;
            }
        }
    }else{
        std::list<InstrumentInput> listInputs;
        reader->read(0, 0xFFFFFFFFFFFFFFFFLL, listInputs);
        m_rInstrDataVector.clear();
        for( auto &ii : listInputs){
            calibration.calibrate(ii);
            m_rInstrDataVector.push_back(ii);
            pointsCount++;
        }

        if( !stAdobeMarkersDir.empty() &&  !stInsta360Dir.empty() ) {
            QDateTime raceTime = QDateTime::fromMSecsSinceEpoch(qint64(m_rInstrDataVector[0].utc.getUnixTimeMs()));
            std::string raceName = "Adobe Race " + raceTime.toString("yyyy-MM-dd hh:mm").toStdString();



            MarkerReader markerReader;
            markerReader.setTimeAdjustmentMs(5000);
//            markerReader.read(stAdobeMarkersDir, stInsta360Dir);

            std::list<Chapter *> chapters;
//            markerReader.makeChapters(m_rInstrDataVector, chapters);

            auto *race = new RaceData(0, m_rInstrDataVector.size() - 1);
            race->SetName(raceName);
            for( auto chapter : chapters){
                race->insertChapter(chapter);
            }
            m_RaceDataList.push_back(race);
        }

    }


    // Make performance vector the same size as the instr data vector
    m_rPerformanceMap.clear();

    delete reader;

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

    MovieProducer movieProducer(moviePath, polarPath, m_rGoProClipInfoList, m_rInstrDataVector, m_rPerformanceMap,
                                m_RaceDataList, *this);

    movieProducer.produce();

    emit produceFinished("Production complete");
}


void Worker::computeStats(const QString &polarUrl){

    int totalLen = 0;
    for ( auto *r: m_RaceDataList) {
        for (auto chapter: r->getChapters()) {
            totalLen += int(chapter->getEndIdx() - chapter->getStartIdx());
        }
    }
    int prevPerc = 0;
    int count = 0;

    Polars polars;
    polars.loadPolar(QUrl(polarUrl).toLocalFile().toStdString());

    m_rPerformanceMap.clear();

    int raceIdx = 0;
    TimeDeltaComputer timeDeltaComputer(polars, m_rInstrDataVector);

    for ( RaceData *race: m_RaceDataList) {

        // Go through the list of chapters and determine if they are fetches or not
        for(auto it = race->getChapters().begin(); it != race->getChapters().end(); it++) {
            Chapter *chapter = *it;

            auto nextIt = it;
            auto prevIt = it;
            nextIt++;
            bool isFetch = false;
            // Entire chapter is a fetch if it goes directly from mark to mark or start to mark
            if (it != race->getChapters().begin() && nextIt != race->getChapters().end()) {
                prevIt--;
                isFetch = ((*prevIt)->getChapterType() == ChapterTypes::START ||
                           (*prevIt)->getChapterType() == ChapterTypes::MARK_ROUNDING)
                          && (*nextIt)->getChapterType() == ChapterTypes::MARK_ROUNDING;
            }
            chapter->setFetch(isFetch);
        }

        timeDeltaComputer.startRace();
        int chapterIdx = 0;
        for(auto it = race->getChapters().begin(); it != race->getChapters().end(); it++, chapterIdx++) {
            Chapter *chapter = *it;

            auto nextIt = it;
            auto prevIt = it;
            nextIt++;

            timeDeltaComputer.startLeg();
            for( uint64_t idx= chapter->getStartIdx(); idx < chapter->getEndIdx(); idx++){
                int perc = count * 100 / int(totalLen);
                if( perc != prevPerc){
                    progress("Computing stats", perc);
                    if ( stopRequested() ){
                        break;
                    }
                    prevPerc = perc;
                }
                count ++;

                // In case of mark rounding we decide separately if we are fetching or not before and after the mark
                // depending on the previous and next chapters are fetches or not
                bool isFetch = chapter->isFetch();
                if ( chapter->getChapterType() == ChapterTypes::MARK_ROUNDING ) {
                    if (idx < chapter->getGunIdx()) {
                        if ( it!=race->getChapters().begin() ) {
                            isFetch = (*prevIt)->isFetch();
                        }
                    }else{
                        if( nextIt != race->getChapters().end() ){
                            isFetch = (*nextIt)->isFetch();
                        }
                    }
                }

                auto utcMs = m_rInstrDataVector[idx].utc.getUnixTimeMs();
                m_rPerformanceMap[utcMs].raceIdx = raceIdx;
                m_rPerformanceMap[utcMs].legIdx = chapterIdx;
                bool beforeStart = chapter->getChapterType() == ChapterTypes::ChapterType::START && idx < chapter->getGunIdx();
                if ( beforeStart ){
                    m_rPerformanceMap[utcMs].isValid = false;
                    m_rPerformanceMap[utcMs].legDistLostToTargetMeters = 0;
                    m_rPerformanceMap[utcMs].legTimeLostToTargetSec = 0;
                }else{
                    timeDeltaComputer.updatePerformance(idx, m_rPerformanceMap[utcMs], isFetch);
                }
            }
        }
        raceIdx ++;
    }

}

void Worker::exportGpx(const QString &path) {
    std::string gpxName = QUrl(path).toLocalFile().toStdString();
    QFile gpxFile(QUrl(path).toLocalFile());
    if (!gpxFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        std::cerr << "Failed to open  " << gpxName << std::endl;
        return;
    }
    int totalLen = 0;
    for ( auto *r: m_RaceDataList) {
        totalLen += int(r->getEndIdx() - r->getStartIdx());
    }
    int prevPerc = 0;
    int count = 0;

    std::cout << "exporting GPX to  " << gpxName << std::endl;

    progress("Exporting GPX", 0);
    emit produceStarted();
    QXmlStreamWriter s(&gpxFile);
    s.setAutoFormatting(true);
    s.writeStartDocument();

    s.writeStartElement("gpx");
    s.writeAttribute("xmlns", "http://www.topografix.com/GPX/1/1");
    for ( auto *race: m_RaceDataList) {
        s.writeStartElement("trk");
        s.writeTextElement("name", race->getName());
        s.writeStartElement("trkseg");

        for(auto idx = race->getStartIdx(); idx < race->getEndIdx(); idx ++){
            int perc = count * 100 / int(totalLen);
            if( perc != prevPerc){
                progress("Exporting GPX", perc);
                if ( stopRequested() ){
                    break;
                }
                prevPerc = perc;
            }
            count ++;

            auto  ii = m_rInstrDataVector[idx];
            s.writeStartElement("trkpt");
            char acBuff[80];
            sprintf(acBuff,"%.5f", ii.loc.getLat());
            s.writeAttribute("lat", acBuff);
            sprintf(acBuff,"%.5f", ii.loc.getLon());
            s.writeAttribute("lon", acBuff);

            QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(ii.utc.getUnixTimeMs())).toUTC();
            QString txt = time.toString("yyyy-MM-ddThh:mm:ssZ");
            s.writeTextElement("time", txt);

            s.writeEndElement();  // </trkpt>
        }

        s.writeEndElement(); // </trkseg>
        s.writeEndElement(); // </trk>
    }

    s.writeEndElement();

    s.writeEndDocument();
    gpxFile.close();

    progress("Exporting GPX", 100);
    std::cout << "Export complete" << std::endl;
    emit produceFinished("GPX export complete");
}

void Worker::exportStats(const QString &polarUrl, const QString &path) {

    std::cout << "Computing stats" << std::endl;
    emit produceStarted();
    progress("Computing stats", 0);

    computeStats(polarUrl);

    progress("Exporting CSV", 0);

    // Store stats as a CSV file
    std::string csvName = QUrl(path).toLocalFile().toStdString();
    std::cout << "exporting stats to  " << csvName << std::endl;
    std::ofstream ofs(csvName);

    ofs << m_rInstrDataVector[0].toCsv(true);
    ofs << m_rPerformanceMap[0].toCsv(true, m_rInstrDataVector[0].utc);
    ofs << std::endl;

    auto totalLen = m_rInstrDataVector.size();
    int prevPerc = 0;
    int count = 0;
    for(auto & ii : m_rInstrDataVector){
        int perc = count * 100 / int(totalLen);
        if( perc != prevPerc){
            progress("Exporting CSV", perc);
            if ( stopRequested() ){
                break;
            }
            prevPerc = perc;
        }
        ofs << ii.toCsv(false);
        auto utcMs = ii.utc.getUnixTimeMs();
        ofs << m_rPerformanceMap[utcMs].toCsv(false, ii.utc);
        ofs << std::endl;
        count ++;
    }

    ofs.close();
    emit produceFinished("CSV export complete");

    progress("Exporting CSV", 100);
    std::cout << "Export complete" << std::endl;
}
