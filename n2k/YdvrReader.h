#ifndef SAILVUE_YDVRREADER_H
#define SAILVUE_YDVRREADER_H


#include <string>
#include <map>
#include <set>
#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>
#include <list>


#include "navcomputer/InstrumentInput.h"
#include "../InstrDataReader.h"
#include "navcomputer/IProgressListener.h"

extern "C" {
#include "pgn.h"
}

static const uint32_t TS_WRAP = 60000;
static const double RES_LL_64 = 1e-16;
static const double RES_LL_32 = 1e-7;
static const double RES_MPS = 0.01;

typedef struct
{
    uint8_t  prio;
    uint32_t pgn;
    uint8_t  dst;
    uint8_t  src;
    uint8_t  len;
    uint8_t  *data;
} YdvrMessage;


class DatFileInfo {
public:
    std::string stYdvrFile;
    std::string stCacheFile;
    uint64_t m_ulStartGpsTimeMs = 0;
    uint64_t m_ulEndGpsTimeMs = 0;
    uint32_t m_ulEpochCount = 0;
};

static const int PGN_BANG_KEY_VALUE = 130824;

const std::set<uint32_t> REQUIRED_PGNS({
    129029, // GNSS Position Data
    129025, // GNSS Position Rapid Update
    129026, // COG & SOG Rapid Update
    127250, // Vessel Heading
    128259, // Boat Speed
    130306, // Wind Data
    127245, // Rudder
    127257, // Attitude
    127258, // Magnetic Variation
    127237, // Heading/Track Control
    128275, // Distance Log
    128267, // Water depth
    PGN_BANG_KEY_VALUE, // Bang proprietary "B&G: key-value data",
});

enum BangKeys{
    BANG_KEY_RACE_TIMER = 117,
    BANG_KEY_TARGET_SPEED = 125,
    BANG_KEY_POLAR_SPEED = 126,
    BANG_KEY_VMG  = 127,
    BANG_KEY_LEEWAY = 130,
    BANG_KEY_CURRENT_DRIFT = 131,
    BANG_KEY_CURRENT_SET = 132,
    BANG_KEY_DIST_TO_START = 152,
    BANG_KEY_PILOT_TARGET_TWA = 385,
    BANG_KEY_START_LINE_PORT_END_LAT = 0x114,
    BANG_KEY_START_LINE_PORT_END_LON = 0x115,
    BANG_KEY_START_LINE_STBD_END_LAT = 0x116,
    BANG_KEY_START_LINE_STBD_END_LON = 0x117,
};

typedef struct
{
    size_t   size;
    uint8_t  data[FASTPACKET_MAX_SIZE];
    uint32_t frames;    // Bit is one when frame is received
    uint32_t allFrames; // Bit is one when frame needs to be present
    int      pgn;
    int      src;
    bool     used;
} Packet;

class BangStartLineData
{
public:
    double portEndLat = DBL_MAX;
    double portEndLon = DBL_MAX;
    double stbdEndLat = DBL_MAX;
    double stbdEndLon = DBL_MAX;
};

const size_t  REASSEMBLY_BUFFER_SIZE = 64;


class YdvrReader : public InstrDataReader {
public:
    YdvrReader(const std::string& stYdvrDir, const std::string& stCacheDir, const std::string& stPgnSrcCsv,
               bool bSummaryOnly, bool bMappingOnly, IProgressListener& rProgressListener, bool ignoreSourcesMap = false);
    ~YdvrReader();

    void read(uint64_t ulStartUtcMs, uint64_t ulEndUtcMs, std::list<InstrumentInput> &listInputs) override;

    void getPgnData(std::map<uint32_t, std::vector<std::string>> &mapPgnDevices,
                    std::map<uint32_t, std::string> &mapPgnDescription);
private:
    void processYdvrDir(const std::string& stYdvrDir, const std::string& stCacheDir, bool bSummaryOnly, bool bMappingOnly);
    void processDatFile(const std::string &ydvrFile, const std::string& stWorkDir,  const std::string& stSummaryDir, bool bSummaryOnly, bool bMappingOnly);
    void readDatFile(const std::string &ydvrFile, const std::filesystem::path &stCacheFile,
                     const std::filesystem::path &stSummaryFile);
    static void canIdToN2k(uint32_t id, uint8_t &prio, uint32_t &pgn, uint8_t &src, uint8_t &dst);
    void ReadPgnSrcTable(const std::string &basicString);

    void ProcessPgn(const YdvrMessage &m, std::ofstream &cache) ;
        void processProductInformationPgn(uint8_t  src, const Pgn *pPgn, const uint8_t *data, uint8_t len);

        bool processGpsFixPgn(const Pgn *pgn, const uint8_t *data, size_t len);
        void processCogSogPgn(const Pgn *pgn, const uint8_t *data, size_t len);
        void processPosRapidUpdate(const Pgn *pgn, const uint8_t *data, uint8_t len);
        void processVesselHeading(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processBoatSpeed(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processWindData(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processRudder(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processAttitude(const Pgn *pPgn, const uint8_t *data, uint8_t len);
        void processMagneticVariation(const Pgn *pgn, uint8_t *data, uint8_t len);
        void processHeadingControl(const Pgn *pgn, uint8_t *data, uint8_t len);
        void processDistanceLog(const Pgn *pgn, uint8_t *data, uint8_t len);

    void ProcessEpoch();
    void ProcessEpochBatch(std::ofstream &cache);
    bool isUint16Valid(int64_t val) const { return val < 65532; }
    bool isUint32Valid(int64_t val) const { return val < 4294967293; }

    bool isInt32Valid(int64_t val) const { return val != 0x7FFFFFFF; }
    bool isInt16Valid(int64_t val) const { return val != 0x7FFFF; }

private:
    static bool m_sCanBoatInitialized;
    IProgressListener& m_rProgressListener;

    // For ech device we keep the set of received PGNs
    std::map<std::string, std::set<uint32_t>> m_mapPgnsByDeviceModelAndSerialNo;
    // Map of device network address to device model and serial number
    std::map<uint8_t, std::string> m_mapDeviceBySrc;

    // Specify what devices to use for each pgn
    std::map<uint32_t, std::string> m_mapDeviceForPgn;

    // Dynamically build this map once src to device mapping becomes known
    // Use this map to accept PGNs coming from this source only
    std::map<uint32_t, uint8_t> m_mapSrcForPgn;

    bool m_ignoreSourcesMap = false;  // Used for unite tests

    uint64_t m_ulLatestGpsTimeMs = 0;  // Time of last GPS fix in Unix time (ms) unlike m_ulGpsFixUnixTimeMs it is carried from file to file

    uint64_t m_ulStartGpsTimeMs = 0;
    uint64_t m_ulEndGpsTimeMs = 0;
    uint32_t m_ulEpochCount = 0;

    InstrumentInput m_epoch;
    uint64_t m_ulPrevEpochUtcMs = 0;

    std::list<InstrumentInput> m_epochBatch;

    std::list<DatFileInfo> m_listDatFiles;
    Angle m_magVar = Angle::INVALID;

    void processWaterDepth(const Pgn *pgn, uint8_t *data, uint8_t len);

    void processBangProprietary(const Pgn *pgn, uint8_t *data, uint8_t len);

    Packet *assembleFastPacket(YdvrMessage *msg);

    Packet reassemblyBuffer[REASSEMBLY_BUFFER_SIZE];

    void lookupBangField(int64_t val, int64_t key, BangStartLineData &startLineData);
};


#endif //SAILVUE_YDVRREADER_H
