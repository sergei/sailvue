#include <gtest/gtest.h>
#include <fstream>

#include "navcomputer/InstrumentInput.h"
#include "../navcomputer/Polars.h"
#include "movie/PolarOverlayMaker.h"
#include "movie/OverlayMaker.h"
#include "movie/RudderOverlayMaker.h"
#include "movie/TargetsOverlayMaker.h"

TEST(MedianTests, PolarTest) {
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

    Polars polars;

    polars.loadPolar("./data/polars-arkana.csv");

    Chapter chapter(0, iiVector.size()-1);
    std::list<InstrumentInput> chapterEpochs;
    for(auto & epoch: iiVector) {
        chapterEpochs.push_back(epoch);
    }

    PolarOverlayMaker polarOverlayMaker(polars, iiVector, 400, 400, 0, 0);

    OverlayMaker overlayMaker("./overlays_perf", 1920, 1080);

    overlayMaker.addOverlayElement(polarOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0; i < 3; i++) {
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

    OverlayMaker overlayMaker("./overlays_rudder", 1920, 1080);

    overlayMaker.addOverlayElement(rudderOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0; i < 50; i++) {
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
    int endIdx = iiVector.size();
    TargetsOverlayMaker targetsOverlayMaker(polars, iiVector, target_ovl_width, target_ovl_height, 0, 0,
                                            startIdx, endIdx);

    OverlayMaker overlayMaker("./overlays_targets", 1920, 1080);

    overlayMaker.addOverlayElement(targetsOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0; i < 200; i++) {
        overlayMaker.addEpoch(iiVector[i], true);
    }

}

TEST(MedianTests, DescriptionFormatTest) {
    std::stringstream ss;
    double tws;
    tws = 10.1;
    ss << "TWS " << std::fixed << std::setprecision(0) << tws;
    ASSERT_EQ(ss.str(), "TWS 10");
}