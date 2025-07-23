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

    std::string stYdvrDir="/Users/sergei/SailingVideos/2025-CORW/2025-Coastal-Cup/20-DATA/10-YDVR";
    const std::string stCacheDir="/tmp/sailvue-unit-test";
    const std::string stPgnSrcCsv = "/Users/sergei/Documents/sailing/pgns/sun-dragons-pgns.csv";

    std::filesystem::remove_all(stCacheDir);

    setLogLevel(LOGLEVEL_INFO);

    YdvrReader ydvrReader(stYdvrDir, stCacheDir, stPgnSrcCsv, false, false,
                          progressListener, true);
    std::list<InstrumentInput> ii;
    ydvrReader.read(0, 0xFFFFFFFFFFF, ii);


    ASSERT_NE( ii.size() , 0);

    // Create the directory to keep CSV files
    std::filesystem::path csvDir = std::filesystem::path(stCacheDir) / "csv";
    std::filesystem::create_directories(csvDir);
    std::string csvFile = (csvDir / "2025-spin-cup.csv").string();
    std::ofstream ofs(csvFile);
    InstrumentInput iiFirst = ii.front();
    ofs << iiFirst.toCsv(true) << std::endl;
    UtcTime raceStart = UtcTime::fromUnixTimeMs(1748286000L * 1000); // 05/26/2025 12:00 PM PDT
    UtcTime raceFinish = UtcTime::fromUnixTimeMs(1748378400L * 1000); // 05/27/2025 13:40 PM PDT
    for (const auto& input : ii){
        if (input.utc.getUnixTimeMs() < raceStart.getUnixTimeMs() || input.utc.getUnixTimeMs() > raceFinish.getUnixTimeMs())
            continue; // Skip inputs outside
        ofs << input.toCsv(false) << std::endl;
    }
}

