#include <iostream>
#include "cxxopts.hpp"
#include "n2k/YdvrReader.h"


class EncodingProgressListener : public IProgressListener {
public:
    void progress(const std::string& state, int progress) override {
    }
    bool stopRequested() override {
        return false;
    }
};

int main(int argc, char** argv) {
    cxxopts::Options options("sailvue", "produce video overlay for GOPRO clips using NMEA2000 data");
    options.add_options()
            ("d,ydvr-dir", "YDVR directory",
             cxxopts::value<std::string>())
            ("c,csv", "CSV file",
             cxxopts::value<std::string>())
            ("a,cache-dir", "Cache directory",
             cxxopts::value<std::string>()->default_value("/tmp/ydn-csv"))
            ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    std::string stYdvrDir = result["ydvr-dir"].as<std::string>();
    std::string stCacheDir = result["cache-dir"].as<std::string>();
    std::string stCsvFile = result["csv"].as<std::string>();

    EncodingProgressListener progressListener;
    YdvrReader ydvrReader(stYdvrDir, stCacheDir, "", false, true, progressListener);

}