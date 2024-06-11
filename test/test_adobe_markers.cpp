#include <gtest/gtest.h>
#include <fstream>
#include "navcomputer/InstrumentInput.h"
#include "adobe_premiere/insta360/MarkerReaderInsta360.h"
#include "Insta360/Insta360.h"


class TestInstrDataReader : public InstrDataReader {
public:
    explicit TestInstrDataReader(const std::string& iiFile){
        std::cout << "Reading data file: " << iiFile << std::endl;
        std::ifstream cache (iiFile, std::ios::in);
        std::string line;

        while (std::getline(cache, line)) {
            std::stringstream ss(line);
            std::string item;
            std::getline(ss, item, ',');
            InstrumentInput ii = InstrumentInput::fromString(line);
            m_allInputs.push_back(ii);
        }
    }

    void read(uint64_t ulStartUtcMs, uint64_t ulEndUtcMs, std::list<InstrumentInput> &listInputs) override{
        for( const auto& ii: m_allInputs){
            if ( ii.utc.getUnixTimeMs() >= ulStartUtcMs &&  ii.utc.getUnixTimeMs() < ulEndUtcMs){
                listInputs.push_back(ii);
            }
        }
    };
private:
    std::list<InstrumentInput> m_allInputs;
};

class TestProgressListener : public IProgressListener {
public:
    void progress(const std::string& state, int progress) override {};
    bool stopRequested() override {return false; };
};

TEST(AdobeMarkersTests, ReadTest) {

    std::filesystem::path markersDir("./data/02-MARKERS");
    std::filesystem::path insta360Dir("/Volumes/SailingVideos2/2024-Coastal-Cup/01-FOOTAGE/01-INSTA-X4/Camera01/");


    std::string iiFile = "./data/00010030.DAT.csv";
    TestInstrDataReader testInstrDataReader(iiFile);
    TestProgressListener testProgressListener;

    auto camera = new Insta360(testInstrDataReader, testProgressListener);
    camera->processClipsDir(insta360Dir, "/tmp");

    std::list<Chapter *> chapters;

    MarkerReaderInsta360 markerReader;
    markerReader.setTimeAdjustmentMs(5000);
    markerReader.read(markersDir, camera->getClipList());

    const std::list<ClipMarker>  markers =  markerReader.getMarkersList();
    ASSERT_EQ(5, markers.size());
    ASSERT_EQ("Start", markers.begin()->getName());
    ASSERT_EQ(1716836639, markers.begin()->getUtcMsIn() / 1000);

    std::vector<InstrumentInput> instrDataVector;

    for (auto clip : camera->getClipList()) {
        for( auto &ii : *clip -> getInstrData()) {
            instrDataVector.push_back(ii);
        }
    }

    markerReader.makeChapters(chapters, instrDataVector);

    ASSERT_EQ(5, chapters.size());
}


