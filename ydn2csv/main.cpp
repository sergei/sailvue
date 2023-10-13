#include <iostream>
#include "cxxopts.hpp"
#include "n2k/YdvrReader.h"


struct Source{
    std::string name;
    int pgn;
    std::string device;
};


class EncodingProgressListener : public IProgressListener {
public:
    void progress(const std::string& state, int progress) override {
    }
    bool stopRequested() override {
        return false;
    }
};

void makeNewPgnCsvFile(const std::string &oldPgnCsvFile, const std::string &newPgnCsvFile, int pgnToReplace, const std::string &deviceName) {
    std::ifstream f (oldPgnCsvFile, std::ios::in);
    std::ofstream fout (newPgnCsvFile, std::ios::out);
    std::string line;

    std::cout  << "Reading PGN sources from " << oldPgnCsvFile << std::endl;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        if ( tokens.size() > 3 ){
            uint32_t pgn = std::stoul(tokens[0]);
            std::string desc = tokens[1];
            if( pgn == pgnToReplace ){
                // Replace with the line that has this device only
                fout << pgn << "," << desc << "," << 0 << "," << deviceName << std::endl;
            } else {
                fout << line << std::endl;
            }
        }
    }

    std::cout << "New PGN sources written to " << newPgnCsvFile << std::endl;
}

int main(int argc, char** argv) {
    cxxopts::Options options("sailvue", "produce video overlay for GOPRO clips using NMEA2000 data");
    options.add_options()
            ("d,ydvr-dir", "YDVR directory",
             cxxopts::value<std::string>())
            ("a,cache-dir", "Cache directory",
             cxxopts::value<std::string>()->default_value("/tmp/ydn-csv"))
            ("p,pgn-src", "CSV file describing sources for PGNs",
             cxxopts::value<std::string>())
            ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    std::string stYdvrDir = result["ydvr-dir"].as<std::string>();
    std::string stCacheDir = result["cache-dir"].as<std::string>();
    std::string stPgnCsvFile = result["pgn-src"].as<std::string>();
    std::filesystem::create_directories(stCacheDir);

    Source sources[] = {
        // Speed over water
        {
            "speed-DST810",
            128259,
            "DST810-A00089FC"
        },
        {
                "speed-H5000-BS",
                128259,
                "H5000    Boat Speed-007060#"
        },
        {
                "speed-H5000-CPU",
                128259,
                "H5000    CPU-007060#"
        },

        // Wind
        {
                "wind-H5000-CPU",
                130306,
                "H5000    CPU-007060#"
        },
        {
                "wind-H5000-MHU",
                130306,
                "H5000    MHU-007060#"
        },
        {
                "wind-WS310",
                130306,
                "WS310-008840#"
        },

        // Heading
        {
                "heading-Precision-9",
                127250,
                "Precision-9 Compass-120196210"
        },
        {
                "heading-ZG100",
                127250,
                "ZG100        Compass-100022#"
        },

    };

    for(Source &src: sources) {
        std::string newPgnCsvFile = stCacheDir + "/" + src.name + "-pgn-src.csv";
        std::string newCacheDir = stCacheDir + "/" + src.name;
        makeNewPgnCsvFile(stPgnCsvFile, newPgnCsvFile, src.pgn, src.device);
        EncodingProgressListener progressListener;
        YdvrReader ydvrReader(stYdvrDir, newCacheDir, newPgnCsvFile, false, false, progressListener);
        std::list<InstrumentInput> inputs;
        ydvrReader.read(0, 0xFFFFFFFFFFFFFFFF, inputs);


        std::string instrCsvFile = stCacheDir + "/" + src.name + "-instr.csv";
        std::ofstream instrCsvOut (instrCsvFile, std::ios::out);

        for(const auto& ii: inputs){
            instrCsvOut << std::string(ii) << std::endl;
        }
        instrCsvOut.close();
        std::cout << "Instrument data written to " << instrCsvFile << std::endl;
    }


}