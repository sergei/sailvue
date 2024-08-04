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

    std::string stYdvrDir="data/ydvr";
    const std::string stCacheDir="/tmp/sailvue-unit-test";
    const std::string stPgnSrcCsv = "/Users/sergei/Documents/sailing/pgns/sun-dragons-pgns.csv";

    std::filesystem::remove_all(stCacheDir);

    setLogLevel(LOGLEVEL_INFO);

    YdvrReader ydvrReader(stYdvrDir, stCacheDir, stPgnSrcCsv, false, false, progressListener);
    std::list<InstrumentInput> ii;
    ydvrReader.read(0, 0xFFFFFFFFFFF, ii);


    ASSERT_EQ( 1 , 0);
}

