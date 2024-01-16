#include <iostream>
#include "n2k/YdvrReader.h"
#include "gopro/GoPro.h"
#include "ffmpeg/FFMpeg.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>

class EncodingProgressListener : public IProgressListener {
public:
    void progress(const std::string& state, int progress) override {
        std::cout << state << " " << progress << std::endl;
    }
    bool stopRequested() override {
        return false;
    }
};

static time_t strToTime(const std::string& str) {
    struct tm tm{};
    strptime(str.c_str(), "%Y-%m-%dT%H:%M", &tm);
    return   mktime(&tm) + tm.tm_gmtoff;
}

void Test(const std::string& stYdvrDir, const std::string& stGoProDir, const std::string& stCacheDir, const std::string& stPgnSrcCsv, bool bSummaryOnly) {
    EncodingProgressListener progressListener;
    YdvrReader ydvrReader(stYdvrDir, stCacheDir, stPgnSrcCsv, bSummaryOnly, false, progressListener);
    GoPro goPro(stGoProDir, stCacheDir, ydvrReader, progressListener);

    int clipCount = 0;
    int pointsCount = 0;
    for ( auto& clip : goPro.getGoProClipList()) {
        clipCount++;
        for( auto &ii : *clip.getInstrData()) {
            pointsCount++;
        }
    }

    std::cout << "clipCount " << clipCount << std::endl;
    std::cout << "pointsCount " << pointsCount << std::endl;
    exit(0);
}

int main(int argc, char** argv) {

    QCoreApplication::setOrganizationName("Santa Cruz Instruments");
    QCoreApplication::setOrganizationDomain("www.santacruzinstruments.com");
    QCoreApplication::setApplicationName("sailvue");

    QGuiApplication app(argc, argv);

    QString appPath = QCoreApplication::applicationDirPath();
    if (! FFMpeg::setBinDir(appPath.toStdString()) ){
        std::cerr << "FFMpeg::setBinDir failed" << std::endl;
        exit(1);
    }

    QQmlApplicationEngine engine;
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));
    engine.load(QUrl(QStringLiteral("qrc:/gui/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;


    return QGuiApplication::exec();
}
