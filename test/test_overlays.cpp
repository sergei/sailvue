#include <gtest/gtest.h>
#include <fstream>

#include "navcomputer/InstrumentInput.h"
#include "../navcomputer/Polars.h"
#include "movie/PolarOverlayMaker.h"
#include "movie/OverlayMaker.h"
#include "movie/RudderOverlayMaker.h"

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

    OverlayMaker overlayMaker("./overlays", 1920, 1080);

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

    OverlayMaker overlayMaker("./overlays", 1920, 1080);

    overlayMaker.addOverlayElement(rudderOverlayMaker);
    overlayMaker.setChapter(chapter, chapterEpochs);

    for(int i = 0; i < 50; i++) {
        overlayMaker.addEpoch(iiVector[i], true);
    }

}