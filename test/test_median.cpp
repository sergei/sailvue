#include <gtest/gtest.h>
#include <fstream>
#include "navcomputer/InstrumentInput.h"

TEST(MedianTests, AngleTest) {
    std::list<Angle> angles;

    uint64_t utcMs = 1000;
    angles.push_back(Angle::fromDegrees(171, utcMs));
    angles.push_back(Angle::fromDegrees(172, utcMs));
    angles.push_back(Angle::fromDegrees(173, utcMs));
    angles.push_back(Angle::fromDegrees(-171, utcMs));
    angles.push_back(Angle::fromDegrees(-172, utcMs));
    angles.push_back(Angle::fromDegrees(-173, utcMs));
    angles.push_back(Angle::fromDegrees(-160, utcMs));
    angles.push_back(Angle::fromDegrees(-150, utcMs));
    angles.push_back(Angle::fromDegrees(90, utcMs));
    angles.push_back(Angle::fromDegrees(91, utcMs));
    angles.push_back(Angle::fromDegrees(-90, utcMs));
    angles.push_back(Angle::fromDegrees(-91, utcMs));

    Angle median = Angle::median(angles, utcMs);
    ASSERT_TRUE(median.isValid(utcMs));
    EXPECT_NEAR( median.getDegrees(),  -172, 1);

}

TEST(MedianTests, DirectionTest) {
    std::list<Direction> dirs;

    uint64_t utcMs = 1000;
    dirs.push_back(Direction::fromDegrees(12, utcMs));
    dirs.push_back(Direction::fromDegrees(2, utcMs));
    dirs.push_back(Direction::fromDegrees(1, utcMs));
    dirs.push_back(Direction::fromDegrees(359, utcMs));
    dirs.push_back(Direction::fromDegrees(350, utcMs));
    dirs.push_back(Direction::fromDegrees(90, utcMs));
    dirs.push_back(Direction::fromDegrees(270, utcMs));

    Direction median = Direction::median(dirs, utcMs);
    ASSERT_TRUE(median.isValid(utcMs));
    EXPECT_NEAR( median.getDegrees(),  1, 1);
}

TEST(MedianTests, SpeedTest) {
    std::list<Speed> speeds;

    uint64_t utcMs = 1000;
    speeds.push_back(Speed::fromKnots(12, utcMs));
    speeds.push_back(Speed::fromKnots(13, utcMs));
    speeds.push_back(Speed::fromKnots(11, utcMs));

    Speed median = Speed::median(speeds, utcMs);
    ASSERT_TRUE(median.isValid(utcMs));
    EXPECT_NEAR( median.getKnots(),  12, 1);
}

TEST(MedianTests, LocTest) {
    std::list<GeoLoc> speeds;

    uint64_t utcMs = 1000;
    speeds.push_back(GeoLoc::fromDegrees(38, -120, utcMs));
    speeds.push_back(GeoLoc::fromDegrees(39, -119, utcMs));
    speeds.push_back(GeoLoc::fromDegrees(37, -121, utcMs));

    GeoLoc median = GeoLoc::median(speeds, utcMs);
    ASSERT_TRUE(median.isValid(utcMs));
    EXPECT_NEAR( median.getLat(),  38, 1);
    EXPECT_NEAR( median.getLon(),  -120, 1);
}

TEST(MedianTests, InstrumentInputTest)
{
    std::vector<InstrumentInput> iiVector;

    std::string iiFile = "./data/ii.csv";
    std::cout << "Reading cached file: " << iiFile << std::endl;
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

    auto begin = iiVector.begin() + 1;
    auto end = iiVector.begin() + 10;

    auto utcMs = begin->utc.getUnixTimeMs();
    InstrumentInput median = InstrumentInput::median(begin, end);

    ASSERT_TRUE(median.loc.isValid(utcMs));
    EXPECT_NEAR( median.loc.getLat(),  37, 1);
    EXPECT_NEAR( median.loc.getLon(),  -122, 1);

    ASSERT_TRUE(median.cog.isValid(utcMs));
    EXPECT_NEAR( median.cog.getDegrees(),  86.2, 0.1);

    ASSERT_TRUE(median.sog.isValid(utcMs));
    EXPECT_NEAR( median.sog.getKnots(),  3.6, 0.1);

    ASSERT_TRUE(median.aws.isValid(utcMs));
    EXPECT_NEAR( median.aws.getKnots(),  5.3, 0.1);

    ASSERT_TRUE(median.awa.isValid(utcMs));
    EXPECT_NEAR( median.awa.getDegrees(),  29.8, 0.1);

    ASSERT_TRUE(median.tws.isValid(utcMs));
    EXPECT_NEAR( median.tws.getKnots(),  3, 0.1);

    ASSERT_TRUE(median.twa.isValid(utcMs));
    EXPECT_NEAR( median.twa.getDegrees(),  62.7, 0.1);

    ASSERT_TRUE(median.mag.isValid(utcMs));
    EXPECT_NEAR( median.mag.getDegrees(),  72.3, 0.1);

    ASSERT_TRUE(median.sow.isValid(utcMs));
    EXPECT_NEAR( median.sow.getKnots(),  3.28, 0.01);

    ASSERT_TRUE(median.rdr.isValid(utcMs));
    EXPECT_NEAR( median.rdr.getDegrees(),  -1.13, 0.01);

    ASSERT_FALSE(median.cmdRdr.isValid(utcMs));

    ASSERT_TRUE(median.yaw.isValid(utcMs));
    EXPECT_NEAR( median.yaw.getDegrees(),  -172, 1);

    ASSERT_TRUE(median.pitch.isValid(utcMs));
    EXPECT_NEAR( median.pitch.getDegrees(),  0.1, 1);

    ASSERT_TRUE(median.roll.isValid(utcMs));
    EXPECT_NEAR( median.roll.getDegrees(),  0.9, 0.1);

    begin = iiVector.begin() + 21;
    end = iiVector.begin() + 113;

    utcMs = begin->utc.getUnixTimeMs();
    median = InstrumentInput::median(begin, end);
    ASSERT_TRUE(median.cmdRdr.isValid(utcMs));
    EXPECT_NEAR( median.cmdRdr.getDegrees(),  0, 0.1);
}

