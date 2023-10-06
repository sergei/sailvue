#include <iostream>
#include "cxxopts.hpp"
#include "n2k/YdvrReader.h"
#include "gopro/GoPro.h"
#include "ffmpeg/FFMpeg.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

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
    cxxopts::Options options("sailvue", "produce video overlay for GOPRO clips using NMEA2000 data");
    options.add_options()
            ("d,ydvr-dir", "YDVR directory",
                    cxxopts::value<std::string>())
            ("g,gopro-dir", "YDVR directory",
                    cxxopts::value<std::string>())
            ("c,cache-dir", "Cache directory",
                    cxxopts::value<std::string>()->default_value("/tmp/sailvue"))
            ("p,pgn-src", "CSV file describing sources for PGNs",
                    cxxopts::value<std::string>()->default_value("data/pgn-src.csv"))
            ("s,summary", "Print summary only",
                    cxxopts::value<bool>()->default_value("false"))
            ("h,help", "Print usage")
            ;

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    std::string stYdvrDir = result["ydvr-dir"].as<std::string>();
    std::string stGoProDir = result["gopro-dir"].as<std::string>();
    std::string stCacheDir = result["cache-dir"].as<std::string>();
    std::string stPgnSrcCsv = result["pgn-src"].as<std::string>();
    bool bSummaryOnly = result["summary"].as<bool>();

//    Test(stYdvrDir, stGoProDir, stCacheDir, stPgnSrcCsv, bSummaryOnly);
    if (! FFMpeg::setBinDir("./ffmpeg/bin/osx") ){
        std::cerr << "FFMpeg::setBinDir failed" << std::endl;
        exit(1);
    }

    QCoreApplication::setOrganizationName("Santa Cruz Instruments");
    QCoreApplication::setOrganizationDomain("www.santacruzinstruments.com");
    QCoreApplication::setApplicationName("sailvue");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/gui/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;


    return QGuiApplication::exec();

}
