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

    if(  QJsonValue v = json["nmeaPath"]; v.isString()){
        setNmeaPath(v.toString());
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
                    int chapterType = chapterJson["type"].toInt();

                    auto chapter = new Chapter(chapterStartIdx, chapterEndIdx);
                    chapter->SetName(chapterName.toStdString());
                    if(  QJsonValue vg = json["gunIdx"]; vg.isDouble()){
                        int chapterGunIdx = vg.toInt();
                        chapter->SetGunIdx(chapterGunIdx);
                    }
                    chapter->setChapterType(ChapterTypes::ChapterType(chapterType));

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
    json["nmeaPath"] = nmeaPath();

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

