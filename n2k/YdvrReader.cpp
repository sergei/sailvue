#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>

#include "YdvrReader.h"
#include "InitCanBoat.h"

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
    std::map<uint32_t, std::string> mapDeviceForPgn;
    mapDeviceForPgn = {
            {129025, "ZG100        Antenna           100022#                        "},  // Position, Rapid Update
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
            } else if( data[0] == 'E') {
                std::cout << "End of file  service record: " << data[0] << std::endl;
            } else if ( data[0] == 'T'){
                std::cout << "Time gap service record: " << data[0] << std::endl;
            } else {
                std::cout << "Unknown service record: " << std::endl;
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

    switch(pgn->pgn){
        case 126996:
            processProductInformationPgn(msg.src, pgn, data, len);
            break;
        case 129029:
            processGpsFixPgn(pgn, data, len);
            break;
        case 129026:
            processCogSogPgn(pgn, data, len);
            break;
    }
}

void YdvrReader::processCogSogPgn(const Pgn *pgn, const uint8_t *data, size_t len) const {
    int64_t val;
    extractNumberByOrder(pgn, 4, data, len, &val);
}

void YdvrReader::processGpsFixPgn(const Pgn *pgn, const uint8_t *data, size_t len) const {
    int64_t fixQuality;
    extractNumberByOrder(pgn, 8, data, len, &fixQuality);
//    std::cout << "fixQuality: " << fixQuality << std::endl;
    if( fixQuality > 0 ){
        int64_t date;
        extractNumberByOrder(pgn, 2, data, len, &date);
        if( date > 40000){
            std::cerr << "Invalid date: " << date << std::endl;
            return;
        }
        int64_t time;
        extractNumberByOrder(pgn, 3, data, len, &time);
        time_t t  = date * 86400 + time / 10000;
        struct tm *tm;
        tm = gmtime(&t);
        char dateStr[20];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", tm);
//        std::cout << "date: " << dateStr << std::endl;

        int64_t val;
        extractNumberByOrder(pgn, 4, data, len, &val);
        double lat = double(val) *  1e-16;
//        std::cout << "latitude: " << lat << std::endl;
        extractNumberByOrder(pgn, 5, data, len, &val);
        double lon = double(val) *  1e-16;
//        std::cout << "longitude: " << lon << std::endl;
    }
}

void YdvrReader::ResetTime() const {

}

void YdvrReader::processProductInformationPgn(uint8_t  src, const Pgn *pgn, const uint8_t *data, uint8_t len)  {

    char acModelId[32];
    extractStringByOrder(pgn, 3, data, len, acModelId, sizeof(acModelId));

    char asSerialCode[32];
    extractStringByOrder(pgn, 6, data, len, asSerialCode, sizeof(asSerialCode));

    std::cout << "acModelId=|" << acModelId << "|" << asSerialCode << "|" << int(src) << std::endl;

    std::string serialCode(asSerialCode);
    std::string modelId(acModelId);

    std::string modelIdAndSerialCode = trim(modelId) + "-" + trim(serialCode);
    if (m_mapPgnsByDeviceModelAndSerialNo.find(modelIdAndSerialCode) == m_mapPgnsByDeviceModelAndSerialNo.end() ) {
        std::set<uint32_t> s;
        m_mapPgnsByDeviceModelAndSerialNo[modelIdAndSerialCode] = s;
    }
    m_mapDeviceBySrc[src] = modelIdAndSerialCode;
}

