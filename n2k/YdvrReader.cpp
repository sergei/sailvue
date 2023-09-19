#include "YdvrReader.h"
#include "InitCanBoat.h"
#include "geo/Angle.h"
#include "geo/Speed.h"

bool YdvrReader::m_sCanBoatInitialized = false;

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

YdvrReader::YdvrReader(const std::string& stYdvrDir, const std::string& stCacheDir, const std::string& stPgnSrcCsv,
                       bool bSummaryOnly, IProgressListener& rProgressListener)
: m_rProgressListener(rProgressListener)
{
    if ( ! m_sCanBoatInitialized ){
        initCanBoat();  // Initialize canboat library
        m_sCanBoatInitialized = true;
    }
    ReadPgnSrcTable(stPgnSrcCsv); // Decide on what devices to use to get each PGN
    processYdvrDir(stYdvrDir, stCacheDir, bSummaryOnly);
}

YdvrReader::~YdvrReader() = default;

void YdvrReader::read(uint64_t ulStartUtcMs, uint64_t ulEndUtcMs, std::list<InstrumentInput> &listInputs) {
    for( const DatFileInfo &di : m_listDatFiles){
        if( di.m_ulEndGpsTimeMs < ulStartUtcMs)
            continue;
        if( di.m_ulStartGpsTimeMs > ulEndUtcMs)
            break;
        std::cout << "Reading cached file: " << di.stCacheFile << std::endl;
        std::ifstream cache (di.stCacheFile, std::ios::in);
        std::string line;
        while (std::getline(cache, line)) {
            std::stringstream ss(line);
            std::string item;
            std::getline(ss, item, ',');
            uint64_t ulGpsTimeMs = std::stoull(item);
            if( ulGpsTimeMs < ulStartUtcMs)
                continue;
            if( ulGpsTimeMs > ulEndUtcMs)
                break;
            InstrumentInput ii = InstrumentInput::fromString(line);
            listInputs.push_back(ii);
        }
    }
}

static bool CompareDatInfo(const DatFileInfo& a, const DatFileInfo& b) {
    return a.m_ulStartGpsTimeMs < b.m_ulStartGpsTimeMs;
}

void YdvrReader::processYdvrDir(const std::string& stYdvrDir, const std::string& stWorkDir, bool bSummaryOnly) {

    // Read all .DAT files in the directory and subdirectories

    std::filesystem::path stCacheDir = std::filesystem::path(stWorkDir) / "ydvr" /"cache" ;
    std::filesystem::create_directories(stCacheDir);
    std::filesystem::path stSummaryDir = std::filesystem::path(stWorkDir) / "ydvr"/ "summary" ;
    std::filesystem::create_directories(stSummaryDir);

    auto files = std::filesystem::recursive_directory_iterator(stYdvrDir);
    float filesCount = 0;
    for( const auto& file : files )
        if (file.path().extension() == ".DAT")
            filesCount++;

    files = std::filesystem::recursive_directory_iterator(stYdvrDir);
    float fileNo = 0;
    for( const auto& file : files ) {
        if (file.path().extension() == ".DAT") {
            processDatFile(file.path().string(), stCacheDir, stSummaryDir, bSummaryOnly);
            int progress = (int)round(fileNo / filesCount * 100.f);
            m_rProgressListener.progress(file.path().filename(), progress);
            if( m_rProgressListener.stopRequested() ) {
                m_rProgressListener.progress("Terminated", 100);
                break;
            }
            fileNo++;
        }
    }

    if( !bSummaryOnly ) {
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

    // Sort by UTC time
    m_listDatFiles.sort(CompareDatInfo);
}

void YdvrReader::processDatFile(const std::string &ydvrFile, const std::string& stCacheDir, const std::string& stSummaryDir, bool bSummaryOnly){
    std::cout << ydvrFile << std::endl;

    std::string csvFileName = std::filesystem::path(ydvrFile).filename().string() + ".csv";
    std::filesystem::path stCacheFile = std::filesystem::path(stCacheDir)  / csvFileName;
    std::filesystem::path stSummaryFile = std::filesystem::path(stSummaryDir)  / csvFileName;

    auto haveCache = std::filesystem::exists(stCacheFile) && std::filesystem::is_regular_file(stCacheFile) ;

    auto haveSummary = std::filesystem::exists(stSummaryFile) && std::filesystem::is_regular_file(stSummaryFile) &&
                     std::filesystem::file_size(stSummaryFile) > 0;
    if (haveCache && haveSummary) {
        std::cout << "Reading cached file: " << stSummaryFile << std::endl;
        std::ifstream summary (stSummaryFile, std::ios::in);
        std::string line;
        std::getline(summary, line); // Skip header
        std::getline(summary, line);
        std::stringstream ss(line);
        std::string item;
        std::getline(ss, item, ',');
        m_ulStartGpsTimeMs = std::stoull(item);
        std::getline(ss, item, ',');
        m_ulEndGpsTimeMs = std::stoull(item);
        std::getline(ss, item, ',');
        m_ulEpochCount = std::stoull(item);
        if( m_ulEpochCount > 0 )
            m_listDatFiles.push_back(DatFileInfo{ydvrFile, stCacheFile, m_ulStartGpsTimeMs, m_ulEndGpsTimeMs, m_ulEpochCount});
    } else {
        readDatFile(ydvrFile, stCacheFile, stSummaryFile);
        std::cout << "Created cached file: " << stCacheFile << std::endl;
        if( m_ulEpochCount > 0 )
            m_listDatFiles.push_back(DatFileInfo{ydvrFile, stCacheFile, m_ulStartGpsTimeMs, m_ulEndGpsTimeMs, m_ulEpochCount});
    }

}

void YdvrReader::readDatFile(const std::string &ydvrFile, const std::filesystem::path &stCacheFile,
                             const std::filesystem::path &stSummaryFile ) {

    std::ofstream cache (stCacheFile, std::ios::out);
    std::ifstream f (ydvrFile, std::ios::in | std::ios::binary);

    m_ulStartGpsTimeMs = 0;
    m_ulEndGpsTimeMs = 0;
    m_ulEpochCount = 0;

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
            const Pgn * pgn = searchForPgn(int(m.pgn));

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
            ProcessPgn(m, cache);
        }

    }

    f.close();
    cache.close();

    std::ofstream summary (stSummaryFile, std::ios::out);
    summary << "StartGpsTimeMs,EndGpsTimeMs,EpochCount" << std::endl;
    summary << m_ulStartGpsTimeMs << "," << m_ulEndGpsTimeMs << "," << m_ulEpochCount << std::endl;
    summary.close();

}

void YdvrReader::canIdToN2k(uint32_t can_id, uint8_t &prio, uint32_t &pgn, uint8_t &src, uint8_t &dst) {
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

void YdvrReader::ProcessPgn(const RawMessage &msg, std::ofstream& cache)  {

    auto data = msg.data;
    auto len = msg.len;
    const Pgn *pgn = getMatchingPgn(int(msg.pgn), data, len);
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
    } else {
        // Here we check if it's coming from approved source address
        if ( m_mapSrcForPgn[pgn->pgn] == msg.src )
        {
            switch(pgn->pgn){
                case 129029:
                    processGpsFixPgn(pgn, data, len);
                    break;
                case 129025:
                    processPosRapidUpdate(pgn, data, len);
                    ProcessEpoch(cache);
                    break;
                case 129026:
                    processCogSogPgn(pgn, data, len);
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
                case 127257:
                    processAttitude(pgn, data, len);
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
    m_epoch.cog = rad < 7 ? Direction::fromRadians(rad, m_ulLatestGpsTimeMs) : Direction::INVALID;
    extractNumberByOrder(pgn, 5, data, len, &val);
    m_epoch.sog = val < 65532 ? Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs) : Speed::INVALID;
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
        m_ulLatestGpsTimeMs = m_ulGpsFixUnixTimeMs;
        // Make it integer number of 200ms intervals

        m_ulGpsFixLocalTimeMs = m_ulUnrolledTsMs;

        m_epoch.utc = UtcTime::fromUnixTimeMs(m_ulLatestGpsTimeMs);

        int64_t val;
        extractNumberByOrder(pgn, 4, data, len, &val);
        double lat = double(val) * RES_LL_64;
        extractNumberByOrder(pgn, 5, data, len, &val);
        double lon = double(val) * RES_LL_64;
        m_epoch.loc = GeoLoc::fromDegrees(lat, lon, m_ulLatestGpsTimeMs);
    }
}

/// Position, Rapid Update
void YdvrReader::processPosRapidUpdate(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 1, data, len, &val);
    double lat = double(val) *  RES_LL_32;
    extractNumberByOrder(pgn, 2, data, len, &val);
    double lon = double(val) *  RES_LL_32;
    m_epoch.loc = GeoLoc::fromDegrees(lat, lon, m_ulLatestGpsTimeMs);

    // Infer epoch time using the last GPS fix time and the local time
    // of the last received message
    if( m_ulGpsFixUnixTimeMs > 0 && m_ulGpsFixLocalTimeMs > 0 ) {
        uint64_t ulGpsFixTimeMs = m_ulGpsFixUnixTimeMs + (m_ulUnrolledTsMs - m_ulGpsFixLocalTimeMs);
//        ulGpsFixTimeMs = uint64_t (ulGpsFixTimeMs / 100. + 0.5) * 100;
        m_epoch.utc = UtcTime::fromUnixTimeMs(ulGpsFixTimeMs);
    }
}

void YdvrReader::processVesselHeading(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    double hdg = double(val) *  RES_RADIANS;
    extractNumberByOrder(pgn, 5, data, len, &val);
    if ( val == 1 && hdg < 7) { // Magnetic
        m_epoch.mag = Direction::fromRadians(hdg, m_ulLatestGpsTimeMs);
    }
}

void YdvrReader::processBoatSpeed(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    m_epoch.sow = val < 65532 ? Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs) : Speed::INVALID;

    extractNumberByOrder(pgn, 3, data, len, &val);
    extractNumberByOrder(pgn, 4, data, len, &val);


}

void YdvrReader::processWindData(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    Speed windSpeed = val < 65532 ? Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs) : Speed::INVALID;
    extractNumberByOrder(pgn, 3, data, len, &val);
    Angle windAngle = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
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
    m_epoch.rdr = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
}

void YdvrReader::processAttitude(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    m_epoch.yaw = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
    extractNumberByOrder(pgn, 3, data, len, &val);
    m_epoch.pitch = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
    extractNumberByOrder(pgn, 4, data, len, &val);
    m_epoch.roll = val < 65532 ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;

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

    // Update mapping src to device
    m_mapDeviceBySrc[src] = modelIdAndSerialCode;

    // Update mapping src to PGNs
    for( auto & mapIt :m_mapDeviceForPgn){
        if( mapIt.second == modelIdAndSerialCode ){
            m_mapSrcForPgn[mapIt.first] = src;
        }
    }

}

void YdvrReader::ResetEpoch() {
    m_epoch = InstrumentInput(); // Reset
}

void YdvrReader::ProcessEpoch(std::ofstream &cache) {
    if( m_epoch.utc.isValid(m_ulGpsFixUnixTimeMs) ){
        if ( m_ulStartGpsTimeMs == 0 )
            m_ulStartGpsTimeMs = m_ulGpsFixUnixTimeMs;
        m_ulEndGpsTimeMs = m_ulGpsFixUnixTimeMs;
        m_ulEpochCount++;

        cache << static_cast<std::string>(m_epoch) << std::endl;
    }
}

void YdvrReader::ReadPgnSrcTable(const std::string &csvFileName) {
/*  Fill the map with the  data like this from CSV file
    m_mapDeviceForPgn = {
            {129029, "ZG100        Antenna-100022#"     },  // GNSS Position Data
            {129025, "ZG100        Antenna-100022#"     },  // Position, Rapid Update
            {129026, "ZG100        Antenna-100022#"     },  // COG & SOG, Rapid Update
            {127250, "Precision-9 Compass-120196210"    },  // Vessel Heading
            {128259, "H5000    CPU-007060#"             },  // Speed
            {130306, "H5000    CPU-007060#"             },  // Wind Data
            {127245, "RF25    _Rudder feedback-038249#" },  // Rudder
            {127257, "Precision-9 Compass-120196210"    },  // Attitude
    };
*/

    std::ifstream f (csvFileName, std::ios::in);
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string pgn;
        std::string device;
        std::getline(iss, pgn, ',');
        std::getline(iss, device, ',');
        std::string modelIdAndSerialCode = trim(device);
        int pgnInt = std::stoi(pgn);
        m_mapDeviceForPgn[pgnInt] = modelIdAndSerialCode;
    }

}
