#ifndef SAILVUE_YDVRREADER_H
#define SAILVUE_YDVRREADER_H


#include <string>
#include <map>
#include <set>

extern "C" {
#include "pgn.h"
}

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

    void processGpsFixPgn(const Pgn *pgn, const uint8_t *data, size_t len) const;

    void processCogSogPgn(const Pgn *pgn, const uint8_t *data, size_t len) const;

    void ResetTime() const;

    void processProductInformationPgn(uint8_t  src, const Pgn *pPgn, const uint8_t *data, uint8_t len);

private:
    // For ech device we keep the set of received PGNs
    std::map<std::string, std::set<uint32_t>> m_mapPgnsByDeviceModelAndSerialNo;
    // Map of device network address to device model and serial number
    std::map<uint8_t, std::string> m_mapDeviceBySrc;
    std::map<uint32_t, uint8_t> m_mapSrcForPgn;
};


#endif //SAILVUE_YDVRREADER_H
