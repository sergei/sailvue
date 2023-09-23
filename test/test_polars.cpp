#include <gtest/gtest.h>
#include "../navcomputer/Polars.h"
#include <libInterpolate/Interpolate.hpp>

TEST(PolarTests, InterpolatorTest)
{
    int nx = 10;
    int ny = 5;

    double xmin = -1;
    double xmax = 8;

    double ymin = -1;
    double ymax = 3;

    double dx = (xmax - xmin)/(nx - 1);
    double dy = (ymax - ymin)/(ny - 1);

    auto f  = [](double x, double y){return x*y + 2*x + 3*y;};

    std::vector<double> xx, yy, zz;
    for( int i = 0; i < nx*ny; i++)
    {
        // gnuplot format is essentially row-major
        xx.push_back( xmin+dx*int(i/ny) );
        yy.push_back( ymin+dy*(i%ny) );
        zz.push_back( f(xx[i],yy[i]) );
    }

    _2D::BilinearInterpolator<double> interp;
    interp.setData(  xx, yy, zz );

    ASSERT_EQ( interp(0,0),  f(0,0));
    ASSERT_EQ( interp(1,2),  f(1,2));
    ASSERT_EQ( interp(2,1),  f(2,1));
    ASSERT_EQ( interp(2,-1), f(2,-1));
    ASSERT_EQ( interp(8,3),  f(8,3));

    ASSERT_EQ( interp(-2,-1), 0);
    ASSERT_EQ( interp(10,3) , 0);
}

TEST(PolarTests, LoadCsvPolarsTest)
{
    Polars polars;

    polars.loadPolar("./data/polars-arkana.csv");

    // Now try to interpolate
    // First get the node
    double twa = 60;
    double tws = 8;
    double spd = polars.getSpeed(twa, tws);
    EXPECT_NEAR(spd, 6.9, 0.01);

    // Then interpolate
    twa = 65;
    tws = 7;
    spd = polars.getSpeed(twa, tws);
    EXPECT_NEAR(spd, 6.47, 0.01);

    // Then extrapolate (should go on a rail)
    twa = 155;
    tws = 8;
    spd = polars.getSpeed(twa, tws);
    EXPECT_NEAR(spd, 5.09, 0.01);
}

TEST(PolarTests, LoadPolPolarsTest)
{
    Polars polars;

    polars.loadPolar("./data/polars-javelin.pol");

    // Now try to interpolate
    // First get the node
    double twa = 60;
    double tws = 8;
    double spd = polars.getSpeed(twa, tws);
    EXPECT_NEAR(spd, 6.38, 0.01);

    // Then interpolate
    twa = 65;
    tws = 7;
    spd = polars.getSpeed(twa, tws);
    EXPECT_NEAR(spd, 6.0, 0.01);

    // Then extrapolate (should go on a rail)
    twa = 155;
    tws = 8;
    spd = polars.getSpeed(twa, tws);
    EXPECT_NEAR(spd, 5.0, 0.01);
}

TEST(PolarTests, PolarsCsvTargetTest)
{
    Polars polars;
    polars.loadPolar("./data/polars-arkana.csv");

    // Check upwind targets
    double tws = 6;
    std::pair<double, double> targets = polars.getTargets(tws, true);
    double targetTwa = targets.first;
    double targetVmg = targets.second;
    double targetSpd = polars.getSpeed(targetTwa, tws);

    EXPECT_NEAR(targetTwa, 50, 0.1);
    EXPECT_NEAR(targetVmg, 3.4, 0.01);
    EXPECT_NEAR(targetSpd, 5.3, 0.01);

    // Check downwind targets
    targets = polars.getTargets(tws, false);
    targetTwa = targets.first;
    targetVmg = targets.second;
    targetSpd = polars.getSpeed(targetTwa, tws);

    EXPECT_NEAR(targetTwa, 140, 0.1);
    EXPECT_NEAR(targetVmg, -3.6, 0.1);
    EXPECT_NEAR(targetSpd, 4.8, 0.1);
}

TEST(PolarTests, PolarsPolTargetTest)
{
    Polars polars;
    polars.loadPolar("./data/polars-javelin.pol");

    // Check upwind targets
    double tws = 6;
    std::pair<double, double> targets = polars.getTargets(tws, true);
    double targetTwa = targets.first;
    double targetVmg = targets.second;
    double targetSpd = polars.getSpeed(targetTwa, tws);

    EXPECT_NEAR(targetTwa, 46, 0.1);
    EXPECT_NEAR(targetVmg, 3.23, 0.01);
    EXPECT_NEAR(targetSpd, 4.67, 0.01);

    // Check downwind targets
    targets = polars.getTargets(tws, false);
    targetTwa = targets.first;
    targetVmg = targets.second;
    targetSpd = polars.getSpeed(targetTwa, tws);

    EXPECT_NEAR(targetTwa, 140, 0.1);
    EXPECT_NEAR(targetVmg, -3.6, 0.1);
    EXPECT_NEAR(targetSpd, 4.62, 0.1);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}