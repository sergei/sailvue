#include <gtest/gtest.h>
#include "navcomputer/IProgressListener.h"
#include "n2k/YdvrReader.h"

TEST(N2kTests, YdvrTest)
{
    class EncodingProgressListener : public IProgressListener {
    public:
        void progress(const std::string& state, int progress) override {
            std::cout << state << " " << progress << std::endl;
        }
        bool stopRequested() override {
            return false;
        }
    };


    EncodingProgressListener progressListener;

    std::string stYdvrDir="/Users/sergei/SailingVideos/2025-SSS-DRAKES-BAY/RACE-1/20-DATA/10-YDVR";
    // UTC time stamps for start and finish
    UtcTime raceStart = UtcTime::fromString("2025-08-16T09:35:00 PDT");
    UtcTime raceFinish = UtcTime::fromString("2025-08-16T17:30:00 PDT");

    const std::string stCacheDir="/tmp/sailvue-unit-test";
    const std::string stPgnSrcCsv = "/Users/sergei/Documents/sailing/pgns/sun-dragons-pgns.csv";

//    std::filesystem::remove_all(stCacheDir);

    setLogLevel(LOGLEVEL_INFO);

    YdvrReader ydvrReader(stYdvrDir, stCacheDir, stPgnSrcCsv, false, false,
                          progressListener, true);
    std::list<InstrumentInput> ii;
    ydvrReader.read(0, 0xFFFFFFFFFFF, ii);


    ASSERT_NE( ii.size() , 0);

    // Create the directory to keep CSV files
    std::filesystem::path csvDir = std::filesystem::path(stCacheDir) / "csv";
    std::filesystem::create_directories(csvDir);
    std::string csvFile = (csvDir / "2025-08-16-sss-drakes-bay-race-1.csv").string();
    std::ofstream ofs(csvFile);
    InstrumentInput iiFirst = ii.front();
    ofs << iiFirst.toCsv(true) << std::endl;

    for (const auto& input : ii){
        if (input.utc.getUnixTimeMs() < raceStart.getUnixTimeMs() || input.utc.getUnixTimeMs() > raceFinish.getUnixTimeMs())
            continue; // Skip inputs outside
        ofs << input.toCsv(false) << std::endl;
    }
}

