#include <gtest/gtest.h>
#include <fstream>
#include "navcomputer/InstrumentInput.h"
#include "adobe_premiere/insta360/MarkerReaderInsta360.h"

TEST(AdobeMarkersTests, ReadTest) {

    std::filesystem::path markersDir("./data/02-MARKERS");
    std::filesystem::path insta360Dir("/Volumes/SailingVideos2/2024-Coastal-Cup/01-FOOTAGE/01-INSTA-X4/Camera01/");

    MarkerReaderInsta360 markerReader;

    markerReader.read(markersDir, insta360Dir);

    std::string iiFile = "./data/00010030.DAT.csv";
    std::cout << "Reading data file: " << iiFile << std::endl;
    std::ifstream cache (iiFile, std::ios::in);
    std::string line;
    std::vector<InstrumentInput> iiVector;

    while (std::getline(cache, line)) {
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        InstrumentInput ii = InstrumentInput::fromString(line);
        iiVector.push_back(ii);
    }
    ASSERT_FALSE(iiVector.empty());

    std::list<Chapter> chapters;
    markerReader.makeChapters(iiVector, chapters);


    ASSERT_EQ(5, chapters.size());
}


