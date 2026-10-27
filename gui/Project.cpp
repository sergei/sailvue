#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include "Project.h"

Project::Project(std::list<RaceData *> &mRaceDataList)
:m_RaceDataList(mRaceDataList) {

    QSettings settings;
    QString polarFile = settings.value(SETTINGS_KEY_POLAR_FILE, "").toString();
    if (polarFile != "" ){
        setPolarPath(polarFile);
    }

}

// Polar path is not part of the project anymore but a global setting
void Project::setPolarPath(const QString &path) {
    m_polarPath = path;
    QSettings settings;
    settings.setValue(SETTINGS_KEY_POLAR_FILE, path);
    std::cout << "set polar path to " << path.toStdString() << std::endl;
}

void Project::fromJson(const QJsonObject &json){

    if(  QJsonValue v = json["goproPath"]; v.isString()){
        setGoProPath(v.toString());
    }

    if(  QJsonValue v = json["insta360Path"]; v.isString()){
        setInsta360Path(v.toString());
    }

    if(  QJsonValue v = json["nmeaPath"]; v.isString()){
        setNmeaPath(v.toString());
    }

    if(  QJsonValue v = json["logsType"]; v.isString()){
        setLogsType(v.toString());
    }

    if(  QJsonValue v = json["races"]; v.isArray()){
        QJsonArray racesArray = v.toArray();
        for( QJsonValue raceJson: racesArray  ){
            QString raceName = raceJson["name"].toString();
            int startIdx = raceJson["startIdx"].toInt();
            int endIdx = raceJson["endIdx"].toInt();
            auto raceData = new RaceData(startIdx, endIdx);
            raceData->SetName(raceName.toStdString());

            if(  QJsonValue c = raceJson["chapters"]; c.isArray()){
                QJsonArray chaptersArray = c.toArray();
                for( QJsonValue chapterJson: chaptersArray  ){
                    QString chapterName = chapterJson["name"].toString();
                    int chapterStartIdx = chapterJson["startIdx"].toInt();
                    int chapterEndIdx = chapterJson["endIdx"].toInt();
                    int chapterGunIdx = chapterJson["gunIdx"].toInt();
                    int chapterType = chapterJson["type"].toInt();
                    QUuid uuid = QUuid(chapterJson["uuid"].toString());
                    QString clipPath = chapterJson["clipPath"].toString();

                    auto chapter = new Chapter(uuid, chapterStartIdx, chapterEndIdx);
                    chapter->SetName(chapterName.toStdString());
                    chapter->SetGunIdx(chapterGunIdx);
                    chapter->setChapterType(ChapterTypes::ChapterType(chapterType));
                    chapter->setChapterClipFileName(clipPath.toStdString());

                    raceData->insertChapter(chapter);
                }
            }
            m_RaceDataList.push_back(raceData);
        }
    }
}

QJsonObject Project::toJson() const {
    QJsonObject json;
    json["goproPath"] = goproPath();
    json["insta360Path"] = insta360Path();
    json["nmeaPath"] = nmeaPath();
    json["logsType"] = logsType();

    QJsonArray racesArray;

    for( auto raceData : m_RaceDataList){
        QJsonObject raceJson;
        raceJson["name"] = QString::fromStdString(raceData->getName());
        raceJson["startIdx"] = qint64(raceData->getStartIdx());
        raceJson["endIdx"] = qint64(raceData->getEndIdx());

        QJsonArray chaptersArray;
        for(auto chapter: raceData->getChapters()){
            QJsonObject chapterJson;
            chapterJson["name"] = QString::fromStdString(chapter->getName());
            chapterJson["type"] = qint8 (chapter->getChapterType());
            chapterJson["startIdx"] = qint64(chapter->getStartIdx());
            chapterJson["endIdx"] = qint64(chapter->getEndIdx());
            chapterJson["gunIdx"] = qint64(chapter->getGunIdx());
            chapterJson["uuid"] = chapter->getUuid();
            chapterJson["clipPath"] = QString::fromStdString(chapter->getChapterClipFileName());

            chaptersArray.append(chapterJson);
        }
        raceJson["chapters"] = chaptersArray;
        racesArray.append(raceJson);
    }

    json["races"] = racesArray;

    return json;
}

bool Project::load(const QString &url) {
    QString path = QUrl(url).toLocalFile();
    setProjectName(path);
    QFile loadFile(path);

    if( ! loadFile.exists()){
        qWarning("File %s does not exist", path.toStdString().c_str());
        return false;
    }

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    fromJson(loadDoc.object());

    m_isDirty = false;

    return true;
}

bool Project::save()  {
    return storeProject(m_projectName);
}

bool Project::saveAs(const QString &url)  {
    QString path = QUrl(url).toLocalFile();
    return storeProject(path);
}

bool Project::storeProject(const QString path) {
    QFile saveFile(path);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject obj = toJson();
    QJsonDocument saveDoc(obj);
    saveFile.write(saveDoc.toJson());
    m_projectName = path;
    m_isDirty = false;

    return true;
}

void Project::setTwaOffset(const double twaOffset) {
    QSettings settings;
    settings.setValue(SETTINGS_TWA_OFFSET, twaOffset);
    std::cout << "set TWA offset to path to " << std::to_string(twaOffset) << std::endl;

}

double Project::twaOffset() {
    QSettings settings;
    double twaOffset = settings.value(SETTINGS_TWA_OFFSET, 0).toDouble();
    std::cout << "Use TWA offset of " << std::to_string(twaOffset) << " degrees" << std::endl;
    return twaOffset;
}

void Project::setCameraUtcOffset(const int cameraUtcOffsetMs) {
    QSettings settings;
    settings.setValue(SETTINGS_CAMERA_UTC_OFFSET_MS, cameraUtcOffsetMs);
    std::cout << "Set Camera URC offset to " << std::to_string(cameraUtcOffsetMs) << std::endl;
}

int Project::cameraUtcOffset() {
    QSettings settings;
    int utcOffsetMs = settings.value(SETTINGS_CAMERA_UTC_OFFSET_MS, 0).toInt();
    std::cout << "Use Camera URC offset of " << std::to_string(utcOffsetMs) << " ms" << std::endl;
    return utcOffsetMs;
}


void Project::setSurfParams(const SurfParams &params) {
    QSettings settings;
    settings.setValue(SETTINGS_SURF_MIN_TWA_DEG, params.minTwaDeg);
    settings.setValue(SETTINGS_SURF_ENTER_RATIO, params.enterRatio);
    settings.setValue(SETTINGS_SURF_EXIT_RATIO, params.exitRatio);
    settings.setValue(SETTINGS_SURF_MIN_DURATION_SEC, double(params.minDurationMs) / 1000.);
    settings.setValue(SETTINGS_SURF_MERGE_GAP_SEC, double(params.mergeGapMs) / 1000.);
    settings.setValue(SETTINGS_SURF_MAX_PER_RACE, qulonglong(params.maxPerRace));
    settings.setValue(SETTINGS_SURF_MIN_SPACING_SEC, double(params.minSpacingMs) / 1000.);
    settings.setValue(SETTINGS_SURF_PRE_ROLL_SEC, double(params.preRollMs) / 1000.);
    settings.setValue(SETTINGS_SURF_POST_ROLL_SEC, double(params.postRollMs) / 1000.);
}

SurfParams Project::surfParams() {
    QSettings settings;
    SurfParams d;
    SurfParams params;
    params.minTwaDeg = settings.value(SETTINGS_SURF_MIN_TWA_DEG, d.minTwaDeg).toDouble();
    params.enterRatio = settings.value(SETTINGS_SURF_ENTER_RATIO, d.enterRatio).toDouble();
    params.exitRatio = settings.value(SETTINGS_SURF_EXIT_RATIO, d.exitRatio).toDouble();
    params.minDurationMs = uint64_t(settings.value(SETTINGS_SURF_MIN_DURATION_SEC, double(d.minDurationMs) / 1000.).toDouble() * 1000);
    params.mergeGapMs = uint64_t(settings.value(SETTINGS_SURF_MERGE_GAP_SEC, double(d.mergeGapMs) / 1000.).toDouble() * 1000);
    params.maxPerRace = settings.value(SETTINGS_SURF_MAX_PER_RACE, qulonglong(d.maxPerRace)).toULongLong();
    params.minSpacingMs = uint64_t(settings.value(SETTINGS_SURF_MIN_SPACING_SEC, double(d.minSpacingMs) / 1000.).toDouble() * 1000);
    params.preRollMs = uint64_t(settings.value(SETTINGS_SURF_PRE_ROLL_SEC, double(d.preRollMs) / 1000.).toDouble() * 1000);
    params.postRollMs = uint64_t(settings.value(SETTINGS_SURF_POST_ROLL_SEC, double(d.postRollMs) / 1000.).toDouble() * 1000);
    return params;
}
