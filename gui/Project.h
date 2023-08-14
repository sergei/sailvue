#ifndef SAILVUE_PROJECT_H
#define SAILVUE_PROJECT_H


#include <iostream>
#include <qqml.h>
#include "navcomputer/RaceData.h"

class Project {
public:
    explicit Project(std::list<RaceData *> &mRaceDataList);

    void setProjectName(const QString &name){
        m_projectName = name;
        m_isDirty = true;
    }

    void setGoProPath(const QString &path){
        m_goproPath = path;
        m_isDirty = true;
        std::cout << "set gopro path to " << path.toStdString() << std::endl;
    }

    void setNmeaPath(const QString &path){
        m_nmeaPath = path;
        m_isDirty = true;
        std::cout << "set nmea path to " << path.toStdString() << std::endl;
    }

    void setPolarPath(const QString &path){
        m_polarPath = path;
        m_isDirty = true;
        std::cout << "set polar path to " << path.toStdString() << std::endl;
    }

    [[nodiscard]] QString projectName() const{
        return m_projectName;
    }

    [[nodiscard]] bool isDirty() const {
        return m_isDirty;
    }

    [[nodiscard]] QString goproPath() const{
        return m_goproPath;
    }

    [[nodiscard]] QString nmeaPath() const{
        return m_nmeaPath;
    }

    [[nodiscard]] QString polarPath() const{
        return m_polarPath;
    }

    bool load(const QString &path);
    bool save();
    bool saveAs(const QString &path);

    void fromJson(const  QJsonObject &json);
    [[nodiscard]] QJsonObject toJson() const;

    void raceDataChanged(){
        m_isDirty = true;
    }

private:
    std::list<RaceData *> &m_RaceDataList;
    QString m_projectName = "Untitled";
    QString m_goproPath = "";
    QString m_nmeaPath = "";
    QString m_polarPath = "";
    bool m_isDirty = false;

    bool storeProject(const QString path);
};


#endif //SAILVUE_PROJECT_H
