#include <iostream>
#include <fstream>
#include <string>
#include "YdvrReader.h"
#include "InstrumentInput.h"

class ProgressListener : public IProgressListener{
public:
    void progress(const std::string& state, int progress) override {};
    bool stopRequested() override {return false;};
};

int main(int argc, char* argv[]) {
    ProgressListener progressListener;

    if (argc != 3) {
        std::cerr << "Usage: ydvr2csv <YDVR folder> <output.csv>" << std::endl;
        return 1;
    }
    std::string ydvrFolder = argv[1];
    std::string outCsv = argv[2];

    std::list<InstrumentInput> instrData;
    YdvrReader reader(ydvrFolder, "/tmp/sailvue/ydvr2csv",
        "/Users/sergei/Documents/sailing/pgns/sun-dragons-pgns.csv"
        , false, false, progressListener, false);

    reader.read(0, UINT64_MAX, instrData);

    if (instrData.empty()) {
        std::cerr << "No instrument data found in YDVR folder." << std::endl;
        return 2;
    }

    std::ofstream ofs(outCsv);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open output file: " << outCsv << std::endl;
        return 3;
    }

    // Write CSV header
    InstrumentInput first = instrData.front();
    ofs << first.toCsv(true) << std::endl;
    // Write data
    for (const auto& ii : instrData) {
        ofs << ii.toCsv(false) << std::endl;
    }
    ofs.close();
    std::cout << "Exported " << instrData.size() << " records to " << outCsv << std::endl;
    return 0;
}
