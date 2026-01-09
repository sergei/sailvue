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

void write_csv_data(std::list<InstrumentInput> &instrData, std::ofstream &ofs) {
    // Write CSV header
    InstrumentInput first = instrData.front();
    ofs << first.toCsv(true) << std::endl;
    // Write data
    for (const auto& ii : instrData) {
        ofs << ii.toCsv(false) << std::endl;
    }
}

void write_expedition_csv_data(std::list<InstrumentInput> &instrData, std::ofstream &ofs) {
    // Write CSV header
    // Write Expedition header
    ofs << "Utc,Bsp,Awa,Aws,Twa,Tws,Twd,Rudder2,Leeway,Set,Drift,Hdg,AirTmp,SeaTmp,Baro,Depth,Heel,Trim,Rudder,Tab,"
           "Forestay,Downhaul,MastAng,FrstyLen,MastButt,StbJmpr,PrtJmpr,Rake,Volts,ROT,"
           "GpsQual,PDOP,GpsNum,GpsAge,GpsGeoHt,GpsAntHt,GpsPosFx,Lat,Lon,Cog,Sog" << std::endl;

        for (const auto& ii : instrData){
            // Convert UNIX ms to string Excel format like 38249.70139
            double excelDate = ii.utc.getUnixTimeMs() / 86400000.0 + 25569.0;
            ofs << std::fixed << std::setprecision(5) << excelDate << ",";
            ofs << ii.sow.toString(ii.utc.getUnixTimeMs()) << ","; // sow
            ofs << ii.awa.toString(ii.utc.getUnixTimeMs()) << ","; // awa
            ofs << ii.aws.toString(ii.utc.getUnixTimeMs()) << ","; // aws
            ofs << ii.twa.toString(ii.utc.getUnixTimeMs()) << ","; // twa
            ofs << ii.tws.toString(ii.utc.getUnixTimeMs()) << ","; // tws
            Direction twd = Direction::fromDegrees(ii.twa.getDegrees() + ii.mag.getDegrees(), ii.utc.getUnixTimeMs());
            ofs << twd.toString(ii.utc.getUnixTimeMs()) << ","; // twd
            ofs <<  ","; // rudder2
            ofs << ii.leeway.toString(ii.utc.getUnixTimeMs()) << ","; // leeway
            ofs << ","; // set
            ofs << ","; // drift
            ofs << ","; // hdg
            ofs << ","; // airTmp
            ofs << ","; // seaTmp
            ofs << ","; // baro
            ofs << ","; // depth
            ofs << ","; // heel
            ofs <<",";  // trim
            ofs << ","; // rudder
            ofs << ","; // tab
            ofs << ","; // forestay
            ofs << ","; // downhaul
            ofs << ","; // mastAng
            ofs << ","; // frstyLen
            ofs << ","; // mastButt
            ofs << ","; // stbJmpr
            ofs << ","; // prtJmpr
            ofs << ","; // rake
            ofs << ","; // volts
            ofs << ","; // rot
            ofs << ","; // gpsQual
            ofs << ","; // pdop
            ofs << ","; // gpsNum
            ofs << ","; // gpsAge
            ofs << ","; // gpsGeoHt
            ofs << ","; // gpsAntHt
            ofs << ","; // gpsPosFx
            ofs << ii.loc.getLat() << ",";
            ofs << ii.loc.getLon() << ",";
            ofs << ii.cog.toString(ii.utc.getUnixTimeMs()) << ",";
            ofs << ii.sog.toString(ii.utc.getUnixTimeMs()) << std::endl;
        }
}

int main(int argc, char* argv[]) {
    ProgressListener progressListener;

    bool expedition = false;
    std::string ydvrFolder;
    std::string outCsv;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-expedition") {
            expedition = true;
        } else if (ydvrFolder.empty()) {
            ydvrFolder = arg;
        } else if (outCsv.empty()) {
            outCsv = arg;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    if (ydvrFolder.empty() || outCsv.empty()) {
        std::cerr << "Usage: ydvr2csv [-expedition] <YDVR folder> <output.csv>" << std::endl;
        return 1;
    }

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

    if (expedition) {
        write_expedition_csv_data(instrData, ofs);
    } else {
        write_csv_data(instrData, ofs);
    }
    ofs.close();
    std::cout << "Exported " << instrData.size() << " records to " << outCsv << std::endl;
    return 0;


}
