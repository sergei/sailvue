#ifndef SAILVUE_YDVRREADER_H
#define SAILVUE_YDVRREADER_H


#include <string>
#include <map>
#include <set>
#include "InstrumentInput.h"

extern "C" {
#include "pgn.h"
}

static const uint32_t TS_WRAP = 60000;
static const double RES_LL_64 = 1e-16;
static const double RES_LL_32 = 1e-7;
static const double RES_MPS = 0.01;

class YdvrReader {
public:
    YdvrReader(const std::string& stYdvrDir, const std::string& stCacheDir);
    ~YdvrReader();

    void read(time_t tStart, time_t tEnd);

    void processYdvrDir(const std::string& stYdvrDir, const std::string& stCacheDir);

    void processDatFile(const std::string &ydvrFile);

    void canIdToN2k(uint32_t id, uint8_t &prio, uint32_t &pgn, uint8_t &src, uint8_t &dst) const;
private:
    void ProcessPgn(const RawMessage &m) ;
        void processGpsFixPgn(const Pgn *pgn, const uint8_t *data, size_t len);
        void processCogSogPgn(const Pgn *pgn, const uint8_t *data, size_t len);
        void processPosRapidUpdate(const Pgn *pgn, const uint8_t *data, uint8_t len);
        void processVesselHeading(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processBoatSpeed(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processWindData(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processRudder(const Pgn *pPgn, const uint8_t *data, uint8_t len);

    void ResetTime();

    void processProductInformationPgn(uint8_t  src, const Pgn *pPgn, const uint8_t *data, uint8_t len);

    void UnrollTimeStamp(uint16_t ts);

    void ResetEpoch();

private:
    // For ech device we keep the set of received PGNs
    std::map<std::string, std::set<uint32_t>> m_mapPgnsByDeviceModelAndSerialNo;
    // Map of device network address to device model and serial number
    std::map<uint8_t, std::string> m_mapDeviceBySrc;

    // Specify what devices to use for each pgn
    std::map<uint32_t, std::string> m_mapDeviceForPgn;

    // Dynamically build this map once src to device mapping becomes known
    // Use this map to accept PGNs coming from this source only
    std::map<uint32_t, uint8_t> m_mapSrcForPgn;

    // Time maintenance
    uint64_t m_ulGpsFixUnixTimeMs = 0;  // Time of last GPS fix in Unix time (ms)
    uint64_t m_ulGpsFixLocalTimeMs = 0; // Time of last GPS fix in local time (ms)
    uint16_t m_usLastTsMs = 0;          // Last time stamp in ms
    uint64_t m_ulUnrolledTsMs = 0;        // Unrolled local time (ms)

    InstrumentInput m_epoch;

    void PrintEpoch();
};


#endif //SAILVUE_YDVRREADER_H
