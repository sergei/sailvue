#include <gtest/gtest.h>
#include "navcomputer/SurfDetector.h"

static const uint64_t T0_MS = 1700000000000;
static const uint64_t DT_MS = 100;           // 10 Hz
static const double POLAR_KTS = 6.7;         // polars-arkana.csv at TWA 150, TWS 10

struct Burst {
    double startSec;
    double durationSec;
    double ratio;
};

static std::vector<InstrumentInput> makeData(double totalSec, const std::vector<Burst> &bursts,
                                             double twa = 150, bool useSog = false) {
    std::vector<InstrumentInput> data;
    auto n = uint64_t(totalSec * 1000 / DT_MS);
    for (uint64_t i = 0; i < n; i++) {
        uint64_t ms = T0_MS + i * DT_MS;
        double sec = double(i * DT_MS) / 1000.;
        double ratio = 1.0;
        for (const auto &b: bursts) {
            if (sec >= b.startSec && sec < b.startSec + b.durationSec)
                ratio = b.ratio;
        }
        InstrumentInput ii;
        ii.utc = UtcTime::fromUnixTimeMs(ms);
        ii.twa = Angle::fromDegrees(twa, ms);
        ii.tws = Speed::fromKnots(10, ms);
        if (useSog)
            ii.sog = Speed::fromKnots(POLAR_KTS * ratio, ms);
        else
            ii.sow = Speed::fromKnots(POLAR_KTS * ratio, ms);
        data.push_back(ii);
    }
    return data;
}

class SurfDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        polars.loadPolar("./data/polars-arkana.csv");
    }

    // Parameters the expectations below were written for
    static SurfParams testParams() {
        SurfParams params;
        params.enterRatio = 1.15;
        params.exitRatio = 1.08;
        params.maxPerRace = 5;
        params.minSpacingMs = 0;
        return params;
    }

    std::vector<SurfEpisode> detect(const std::vector<InstrumentInput> &data, SurfParams params = testParams()) {
        SurfDetector detector(polars, data, params);
        return detector.detect(0, data.size());
    }

    static double sec(const std::vector<InstrumentInput> &data, uint64_t idx) {
        return double(data[idx].utc.getUnixTimeMs() - T0_MS) / 1000.;
    }

    Polars polars;
};

TEST_F(SurfDetectorTest, SingleSurf) {
    auto data = makeData(60, {{20, 6, 1.3}});
    auto episodes = detect(data);
    ASSERT_EQ(episodes.size(), 1);
    // Surf from 20 to 26 sec plus pre and post roll
    EXPECT_NEAR(sec(data, episodes[0].startIdx), 15, 0.6);
    EXPECT_NEAR(sec(data, episodes[0].endIdx), 29, 0.6);
    EXPECT_NEAR(episodes[0].maxSpeedKts, POLAR_KTS * 1.3, 0.01);
    EXPECT_NEAR(episodes[0].maxRatio, 1.3, 0.05);
    EXPECT_NEAR(episodes[0].score, 6 * 0.3, 0.3);
}

TEST_F(SurfDetectorTest, TooShort) {
    auto data = makeData(60, {{20, 2, 1.3}});
    EXPECT_TRUE(detect(data).empty());
}

TEST_F(SurfDetectorTest, MergeClose) {
    auto data = makeData(80, {{20, 5, 1.3}, {30, 5, 1.3}});
    auto episodes = detect(data);
    ASSERT_EQ(episodes.size(), 1);
    EXPECT_NEAR(sec(data, episodes[0].startIdx), 15, 0.6);
    EXPECT_NEAR(sec(data, episodes[0].endIdx), 38, 0.6);
}

TEST_F(SurfDetectorTest, KeepBest) {
    std::vector<Burst> bursts;
    for (int i = 0; i < 8; i++) {
        bursts.push_back({20. + i * 30., 6, 1.2 + 0.05 * i});
    }
    auto data = makeData(300, bursts);
    auto episodes = detect(data);
    ASSERT_EQ(episodes.size(), testParams().maxPerRace);
    // The strongest are the last five, reported in time order
    for (size_t i = 0; i < episodes.size(); i++) {
        EXPECT_NEAR(sec(data, episodes[i].startIdx), 20. + double(i + 3) * 30. - 5, 0.6);
    }
}

TEST_F(SurfDetectorTest, IgnoreUpwind) {
    auto data = makeData(60, {{20, 6, 3.0}}, 45);
    EXPECT_TRUE(detect(data).empty());
}

TEST_F(SurfDetectorTest, FallbackToSog) {
    auto data = makeData(60, {{20, 6, 1.3}}, 150, true);
    EXPECT_EQ(detect(data).size(), 1);
}

TEST_F(SurfDetectorTest, MinSpacing) {
    // Cluster of four surfs 30 sec apart, the strongest one is the third, and a weak one 5 min later
    auto data = makeData(600, {{20, 6, 1.3}, {50, 6, 1.35}, {80, 6, 1.5}, {110, 6, 1.3}, {400, 6, 1.2}});
    SurfParams params = testParams();
    params.maxPerRace = 10;
    params.minSpacingMs = 120000;
    auto episodes = detect(data, params);
    ASSERT_EQ(episodes.size(), 2);
    EXPECT_NEAR(sec(data, episodes[0].startIdx), 75, 0.6);
    EXPECT_NEAR(sec(data, episodes[1].startIdx), 395, 0.6);
}
