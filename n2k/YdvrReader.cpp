#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>

#include "YdvrReader.h"
#include "InitCanBoat.h"
#include "geo/Angle.h"
#include "geo/Speed.h"

std::string trim(const std::string& str,
                 const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

YdvrReader::YdvrReader(const std::string& stYdvrDir, const std::string& stCacheDir)
{
    m_mapDeviceForPgn = {
            {129029, "ZG100        Antenna-100022#"     },  // GNSS Position Data
            {129025, "ZG100        Antenna-100022#"     },  // Position, Rapid Update
            {129026, "ZG100        Antenna-100022#"     },  // COG & SOG, Rapid Update
            {127250, "Precision-9 Compass-120196210"    },  // Vessel Heading
            {128259, "H5000    CPU-007060#"             },  // Speed
            {130306, "H5000    CPU-007060#"             },  // Wind Data
            {127245, "RF25    _Rudder feedback-038249#" },  // Rudder
    };


    initCanBoat();
    processYdvrDir(stYdvrDir, stCacheDir);
}

YdvrReader::~YdvrReader() = default;

void YdvrReader::read(time_t tStart, time_t tEnd) {

}

void YdvrReader::processYdvrDir(const std::string& stYdvrDir, const std::string& stCacheDir) {
    // Create list of all files with extension .DAT in stYdvrDir
    // Craete empty list
    std::vector<std::string> ydvrFiles;
    auto files = std::filesystem::recursive_directory_iterator(stYdvrDir);
    for( const auto& file : files ) {
        if (file.path().extension() == ".DAT") {
            ydvrFiles.push_back(file.path().string());
        }
    }

    // For each file in list
    for (auto & ydvrFile : ydvrFiles) {
        processDatFile(ydvrFile);
    }

    std::cout << "---------------------------------------" << std::endl;
    std::cout << "DeviceName SerialNo, PGN, Description" << std::endl;
    for (auto & mapIt : m_mapPgnsByDeviceModelAndSerialNo) {
        for (auto setIt = mapIt.second.begin(); setIt != mapIt.second.end(); setIt++) {
            int pgn = int(*setIt);
            auto deviceName = mapIt.first;
            std::cout << deviceName << "," << pgn ;
            const Pgn *pPgn = searchForPgn(pgn);
            if (pPgn != nullptr && pPgn->description != nullptr) {
                std::cout << ",\"" << pPgn->description << "\"";
            }
            std::cout << std::endl;
        }
    }
    std::cout << "---------------------------------------" << std::endl;

}

void YdvrReader::processDatFile(const std::string &ydvrFile){
    std::cout << ydvrFile << std::endl;
    std::ifstream f (ydvrFile, std::ios::in | std::ios::binary);

    while (f) {
        uint16_t ts;
        size_t offset = f.tellg();

        if ( ! f.read((char*)&ts, sizeof(ts)) ) {
            break;
        }
        uint32_t msg_id;
        if ( ! f.read((char*)&msg_id, sizeof(msg_id)) ) {
            break;
        }

        if( msg_id == 0xFFFFFFFF ) {  // YDVR service record
            uint8_t data[8];
            if ( ! f.read((char*)&data, sizeof(data)) ) {
                break;
            }
            if( !memcmp(data, "YDVR", 4) ) {
                std::cout << "Start of file service record" << std::endl;
                ResetTime();
            } else if( data[0] == 'E') {
                std::cout << "End of file  service record: " << data[0] << std::endl;
                ResetTime();
            } else if ( data[0] == 'T'){
                std::cout << "Time gap service record: " << data[0] << std::endl;
                ResetTime();
            } else {
                std::cout << "Unknown service record: " << std::endl;
                ResetTime();
            }
        }else {
            RawMessage m;
            canIdToN2k(msg_id, m.prio, m.pgn, m.src, m.dst);

            if ( m.pgn < 59392 ){
                std::cerr << "Invalid PGN: " << m.pgn << " file " << ydvrFile << " offset " << offset << std::endl;
                ResetTime();
            }
            const Pgn * pgn = searchForPgn(m.pgn);

            if ( m.pgn == 59904 ) {
                m.len = 3;
                if ( ! f.read((char*)&m.data, m.len) ) {
                    break;
                }
            }else if ( pgn != nullptr && pgn->type == PACKET_FAST && pgn->complete == PACKET_COMPLETE){
                uint8_t seqNo;
                if ( ! f.read((char*)&seqNo, 1) ) {
                    break;
                }
                if ( ! f.read((char*)&m.len, 1) ) {
                    break;
                }
                if( m.len > sizeof(m.data) ) {
                    std::cerr << "Invalid length: " << m.len <<  " offset " << offset << std::endl;
                    ResetTime();
                    continue;
                }
                if ( ! f.read((char*)&m.data, m.len) ) {
                    break;
                }
            }else {
                m.len = 8;
                if ( ! f.read((char*)&m.data, m.len) ) {
                    break;
                }
            }
//            std::cout << "PGN: " << m.pgn << " len: " << int(m.len) <<  " offset " << offset << std::endl;
            UnrollTimeStamp(ts);
            ProcessPgn(m);
        }

    }

    f.close();

}

void YdvrReader::canIdToN2k(uint32_t can_id, uint8_t &prio, uint32_t &pgn, uint8_t &src, uint8_t &dst) const {
    uint8_t can_id_pf = (can_id >> 16) & 0x00FF;
    uint8_t can_id_ps = (can_id >> 8) & 0x00FF;
    uint8_t can_id_dp = (can_id >> 24) & 1;

    src = can_id >> 0 & 0x00FF;
    prio = ((can_id >> 26) & 0x7);

    if( can_id_pf < 240) {
        /* PDU1 format, the PS contains the destination address */
        dst = can_id_ps;
        pgn = (can_id_dp << 16) | (can_id_pf << 8);
    } else {
        /* PDU2 format, the destination is implied global and the PGN is extended */
        dst = 0xff;
        pgn = (can_id_dp << 16) | (can_id_pf << 8) | can_id_ps;
    }

}

void YdvrReader::ProcessPgn(const RawMessage &msg)  {

    auto data = msg.data;
    auto len = msg.len;
    const Pgn *pgn = getMatchingPgn(msg.pgn, data, len);
    if ( pgn == nullptr ) {
        return;
    }

    // Find device by the source address
    if ( m_mapDeviceBySrc.find(msg.src) != m_mapDeviceBySrc.end() )
    {
        auto modelIdAndSerialCode = m_mapDeviceBySrc[msg.src];
        m_mapPgnsByDeviceModelAndSerialNo[modelIdAndSerialCode].insert(pgn->pgn);
    }

    if (pgn->pgn ==  126996){
        // We process this PGN coming from any source
        processProductInformationPgn(msg.src, pgn, data, len);
    }else {
        // Here we check if it's coming from approved source address
        if ( m_mapSrcForPgn[pgn->pgn] == msg.src )
        {
            switch(pgn->pgn){
                case 129029:
                    processGpsFixPgn(pgn, data, len);
                    PrintEpoch();
                    break;
                case 129026:
                    processCogSogPgn(pgn, data, len);
                    break;
                case 129025:
                    processPosRapidUpdate(pgn, data, len);
                    break;
                case 127250:
                    processVesselHeading(pgn, data, len);
                    break;
                case 128259:
                    processBoatSpeed(pgn, data, len);
                    break;
                case 130306:
                    processWindData(pgn, data, len);
                    break;
                case 127245:
                    processRudder(pgn, data, len);
                    break;
            }
        }
    }
}

/// COG & SOG, Rapid Update
void YdvrReader::processCogSogPgn(const Pgn *pgn, const uint8_t *data, size_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 4, data, len, &val);
    double rad = double(val) *  RES_RADIANS;
    m_epoch.cog = rad < 7 ? Direction::fromRadians(rad) : Direction::INVALID;
    extractNumberByOrder(pgn, 5, data, len, &val);
    m_epoch.sog = val < 65532 ? Speed::fromMetersPerSecond(double(val) * RES_MPS) : Speed::INVALID;
}

/// GNSS Position Data
void YdvrReader::processGpsFixPgn(const Pgn *pgn, const uint8_t *data, size_t len) {
    int64_t fixQuality;
    extractNumberByOrder(pgn, 8, data, len, &fixQuality);
    if( fixQuality > 0 ){
        int64_t date;
        extractNumberByOrder(pgn, 2, data, len, &date);
        if( date > 40000){
            std::cerr << "Invalid date: " << date << std::endl;
            return;
        }
        int64_t time;
        extractNumberByOrder(pgn, 3, data, len, &time);

        m_ulGpsFixUnixTimeMs = date * 86400 * 1000 + time / 10;
        // Make it integer number of 200ms intervals
        m_ulGpsFixUnixTimeMs = (m_ulGpsFixUnixTimeMs / 200) * 200;
        m_ulGpsFixLocalTimeMs = m_ulUnrolledTsMs;

        m_epoch.utc = UtcTime::fromUnixTimeMs(m_ulGpsFixUnixTimeMs);

        int64_t val;
        extractNumberByOrder(pgn, 4, data, len, &val);
        double lat = double(val) * RES_LL_64;
        extractNumberByOrder(pgn, 5, data, len, &val);
        double lon = double(val) * RES_LL_64;
        m_epoch.loc = GeoLoc::fromDegrees(lat, lon);
    }
}

/// Position, Rapid Update
void YdvrReader::processPosRapidUpdate(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 1, data, len, &val);
    double lat = double(val) *  RES_LL_32;
    extractNumberByOrder(pgn, 2, data, len, &val);
    double lon = double(val) *  RES_LL_32;
    m_epoch.loc = GeoLoc::fromDegrees(lat, lon);

}

void YdvrReader::processVesselHeading(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    double hdg = double(val) *  RES_RADIANS;
    extractNumberByOrder(pgn, 5, data, len, &val);
    if ( val == 1 && hdg < 7) { // Magnetic
        m_epoch.mag = Direction::fromRadians(hdg);
    }
}

void YdvrReader::processBoatSpeed(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    m_epoch.sow = val < 65532 ? Speed::fromMetersPerSecond(double(val) * RES_MPS) : Speed::INVALID;

    extractNumberByOrder(pgn, 3, data, len, &val);
    extractNumberByOrder(pgn, 4, data, len, &val);


}

void YdvrReader::processWindData(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    Speed windSpeed = val < 65532 ? Speed::fromMetersPerSecond(double(val) * RES_MPS) : Speed::INVALID;
    extractNumberByOrder(pgn, 3, data, len, &val);
    Angle windAngle = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS) : Angle::INVALID;
    extractNumberByOrder(pgn, 4, data, len, &val);
    if( val == 2 ){  // Apparent Wind (relative to the vessel centerline)
        m_epoch.awa = windAngle;
        m_epoch.aws = windSpeed;
    }else if ( val == 3 || val == 4 ){ // True Wind
        m_epoch.twa = windAngle;
        m_epoch.tws = windSpeed;
    }

}

void YdvrReader::processRudder(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 5, data, len, &val);
    m_epoch.rdr = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS) : Angle::INVALID;
}

void YdvrReader::ResetTime() {
    m_ulGpsFixUnixTimeMs = 0;  // Time of last GPS fix in Unix time (ms)
    m_ulGpsFixLocalTimeMs = 0; // Time of last GPS fix in local time (ms)
    m_usLastTsMs = 0;          // Last time stamp in ms
}

void YdvrReader::UnrollTimeStamp(uint16_t ts) {
    int32_t tsDiff = int32_t(ts) - m_usLastTsMs;
    m_usLastTsMs = ts;
    if (tsDiff < 0) {
        tsDiff += TS_WRAP;
    }
    m_ulUnrolledTsMs += tsDiff;
}

void YdvrReader::processProductInformationPgn(uint8_t  src, const Pgn *pgn, const uint8_t *data, uint8_t len)  {

    char acModelId[32];
    extractStringByOrder(pgn, 3, data, len, acModelId, sizeof(acModelId));

    char asSerialCode[32];
    extractStringByOrder(pgn, 6, data, len, asSerialCode, sizeof(asSerialCode));

    std::string serialCode(asSerialCode);
    std::string modelId(acModelId);

    std::string modelIdAndSerialCode = trim(modelId) + "-" + trim(serialCode);
    if (m_mapPgnsByDeviceModelAndSerialNo.find(modelIdAndSerialCode) == m_mapPgnsByDeviceModelAndSerialNo.end() ) {
        std::set<uint32_t> s;
        m_mapPgnsByDeviceModelAndSerialNo[modelIdAndSerialCode] = s;
    }
    m_mapDeviceBySrc[src] = modelIdAndSerialCode;

    // Update mapping src to PGNs
    for( auto & mapIt :m_mapDeviceForPgn){
        if( mapIt.second == modelIdAndSerialCode ){
            m_mapSrcForPgn[mapIt.first] = src;
        }
    }

    std::cout << "acModelId=|" << acModelId << "|" << asSerialCode << "|" << int(src) << std::endl;

}

void YdvrReader::ResetEpoch() {
    m_epoch.awa = Angle::INVALID;
    m_epoch.twa = Angle::INVALID;
}

void YdvrReader::PrintEpoch() {
    std::cout << static_cast<std::string>(m_epoch.utc)
        << ",loc," << static_cast<std::string>(m_epoch.loc)
        << ",cog," << static_cast<std::string>(m_epoch.cog)
        << ",sog," << static_cast<std::string>(m_epoch.sog)
        << ",aws," << static_cast<std::string>(m_epoch.aws)
        << ",awa," << static_cast<std::string>(m_epoch.awa)
        << ",tws," << static_cast<std::string>(m_epoch.tws)
        << ",twa," << static_cast<std::string>(m_epoch.twa)
        << ",mag," << static_cast<std::string>(m_epoch.mag)
        << ",sow," << static_cast<std::string>(m_epoch.sow)
        << ",rdr," << static_cast<std::string>(m_epoch.rdr)
        << std::endl;
}






