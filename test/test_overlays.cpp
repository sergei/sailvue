#include <gtest/gtest.h>
#include <fstream>

#include "navcomputer/InstrumentInput.h"
#include "../navcomputer/Polars.h"
#include "movie/PolarOverlayMaker.h"
#include "movie/OverlayMaker.h"
#include "movie/RudderOverlayMaker.h"
#include "movie/TargetsOverlayMaker.h"
#include "movie/InstrOverlayMaker.h"
#include "navcomputer/Performance.h"
#include "movie/PerformanceOverlayMaker.h"

TEST(MedianTests, PolarTest)
{
    std::vector<InstrumentInput> iiVector;

    std::string iiFile = "./data/ii-polar.csv";
    std::cout << "Reading data file: " << iiFile << std::endl;
    std::ifstream cache (iiFile, std::ios::in);
    std::string line;
    while (std::getline(cache, line)) {
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        InstrumentInput ii = InstrumentInput::fromString(line);
        iiVector.push_back(ii);
    }
    ASSERT_FALSE(iiVector.empty());

    Polars polars;

    polars.loadPolar("./data/polars-arkana.csv");

    Chapter chapter(0, iiVector.size()-1);
    std::list<InstrumentInput> chapterEpochs;
    for(auto & epoch: iiVector) {
        chapterEpochs.push_back(epoch);
    }

    PolarOverlayMaker polarOverlayMaker(polars, iiVector, 400, 400, 0, 0);

    const char *const overlayDir = "./overlays_polar";
    std::filesystem::remove_all(overlayDir);
    OverlayMaker overlayMaker(overlayDir, 1920, 1080);

    overlayMaker.addOverlayElement(polarOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0;  i < iiVector.size() && i < 3; i++) {
        overlayMaker.addEpoch(iiVector[i], true);
    }

}

TEST(MedianTests, RudderTest) {
    std::vector<InstrumentInput> iiVector;

    std::string iiFile = "./data/ii.csv";
    std::cout << "Reading data file: " << iiFile << std::endl;
    std::ifstream cache (iiFile, std::ios::in);
    std::string line;
    while (std::getline(cache, line)) {
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        InstrumentInput ii = InstrumentInput::fromString(line);
        iiVector.push_back(ii);
    }
    ASSERT_FALSE(iiVector.empty());

    Chapter chapter(0, iiVector.size()-1);
    std::list<InstrumentInput> chapterEpochs;
    for(auto & epoch: iiVector) {
        chapterEpochs.push_back(epoch);
    }

    RudderOverlayMaker rudderOverlayMaker(400, 200, 0, 0);

    const char *const overlayDir = "./overlays_rudder";
    std::filesystem::remove_all(overlayDir);
    OverlayMaker overlayMaker(overlayDir, 1920, 1080);

    overlayMaker.addOverlayElement(rudderOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0;  i < iiVector.size() && i < 50; i++) {
        overlayMaker.addEpoch(iiVector[i], true);
    }

}

TEST(MedianTests, TargetsOverlayTest) {
    std::vector<InstrumentInput> iiVector;

    std::string iiFile = "./data/ii.csv";
    std::cout << "Reading data file: " << iiFile << std::endl;
    std::ifstream cache (iiFile, std::ios::in);
    std::string line;
    while (std::getline(cache, line)) {
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        InstrumentInput ii = InstrumentInput::fromString(line);
        iiVector.push_back(ii);
    }
    ASSERT_FALSE(iiVector.empty());

    Chapter chapter(0, iiVector.size()-1);
    std::list<InstrumentInput> chapterEpochs;
    for(auto & epoch: iiVector) {
        chapterEpochs.push_back(epoch);
    }

    Polars polars;
    polars.loadPolar("./data/polars-arkana.csv");

    int movieWidth = 1920;
    int target_ovl_width = movieWidth;
    int target_ovl_height = 128;

    int startIdx = 0;
    int endIdx = int(iiVector.size());
    TargetsOverlayMaker targetsOverlayMaker(polars, iiVector, target_ovl_width, target_ovl_height, 0, 0,
                                            startIdx, endIdx);

    const char *const overlayDir = "./overlays_targets";
    std::filesystem::remove_all(overlayDir);
    OverlayMaker overlayMaker(overlayDir, 1920, 1080);

    overlayMaker.addOverlayElement(targetsOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0;  i < iiVector.size() && i < 200; i++) {
        overlayMaker.addEpoch(iiVector[i], true);
    }

}

TEST(MedianTests, InstrumentsOverlayTest) {
    std::vector<InstrumentInput> iiVector;

    std::string iiFile = "./data/ii.csv";
    std::cout << "Reading data file: " << iiFile << std::endl;
    std::ifstream cache (iiFile, std::ios::in);
    std::string line;
    while (std::getline(cache, line)) {
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        InstrumentInput ii = InstrumentInput::fromString(line);
        iiVector.push_back(ii);
    }
    ASSERT_FALSE(iiVector.empty());

    Chapter chapter(0, iiVector.size()-1);
    std::list<InstrumentInput> chapterEpochs;
    for(auto & epoch: iiVector) {
        chapterEpochs.push_back(epoch);
    }

    int movieWidth = 1920;
    int target_ovl_width = movieWidth;
    int target_ovl_height = 128;

    InstrOverlayMaker instrumentsOverlayMaker(iiVector, target_ovl_width, target_ovl_height, 0, 0);
    const char *const overlayDir = "./overlays_instruments";
    std::filesystem::remove_all(overlayDir);
    OverlayMaker overlayMaker(overlayDir, 1920, 1080);

    overlayMaker.addOverlayElement(instrumentsOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0; i < iiVector.size() && i < 200; i++) {
        overlayMaker.addEpoch(iiVector[i], true);
    }

}

TEST(MedianTests, PerformanceOverlayTest) {
    std::vector<InstrumentInput> iiVector;

    std::string iiFile = "./data/ii.csv";
    std::cout << "Reading data file: " << iiFile << std::endl;
    std::ifstream cache (iiFile, std::ios::in);
    std::string line;
    while (std::getline(cache, line)) {
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        InstrumentInput ii = InstrumentInput::fromString(line);
        iiVector.push_back(ii);
    }
    ASSERT_FALSE(iiVector.empty());

    Chapter chapter(0, iiVector.size()-1);
    std::list<InstrumentInput> chapterEpochs;
    for(auto & epoch: iiVector) {
        chapterEpochs.push_back(epoch);
    }

    Performance bad;
    bad.raceTimeLostToTargetSec = 10;
    bad.legTimeLostToTargetSec = 5;
    bad.legDistLostToTargetMeters = 15;
    bad.isValid = true;
    Performance good;
    good.raceTimeLostToTargetSec = -10;
    good.legTimeLostToTargetSec = -5;
    good.legDistLostToTargetMeters = -6;
    good.isValid = true;
    std::map<uint64_t, Performance> performanceMap;
    performanceMap[iiVector[0].utc.getUnixTimeMs()] = bad;
    performanceMap[iiVector[1].utc.getUnixTimeMs()] = good;

    PerformanceOverlayMaker performanceOverlayMaker(performanceMap, 200, 200, 0, 0);
    const char *const overlayDir = "./overlays_performance";
    std::filesystem::remove_all(overlayDir);
    OverlayMaker overlayMaker(overlayDir, 1920, 1080);

    overlayMaker.addOverlayElement(performanceOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    overlayMaker.addEpoch(iiVector[0], true);
    overlayMaker.addEpoch(iiVector[1], true);
}

TEST(MedianTests, DescriptionFormatTest) {
    std::stringstream ss;
    double tws;
    tws = 10.1;
    ss << "TWS " << std::fixed << std::setprecision(0) << tws;
    ASSERT_EQ(ss.str(), "TWS 10");
}