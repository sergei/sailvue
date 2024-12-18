#include "YdvrReader.h"
#include "InitCanBoat.h"
#include "geo/Angle.h"
#include "geo/Speed.h"

static void deCommas(std::string &s) {
    std::replace( s.begin(), s.end(), ',', ';'); // replace all ',' to ';'
}

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
                       bool bSummaryOnly, bool bMappingOnly, IProgressListener& rProgressListener, bool ignoreSourcesMap)
: m_rProgressListener(rProgressListener), m_ignoreSourcesMap(ignoreSourcesMap)
{
    if ( ! m_sCanBoatInitialized ){
        initCanBoat();  // Initialize canboat library
        m_sCanBoatInitialized = true;
    }
    if ( ! bMappingOnly ){ // We don't have this table
        ReadPgnSrcTable(stPgnSrcCsv); // Decide on what devices to use to get each PGN
    }
    processYdvrDir(stYdvrDir, stCacheDir, bSummaryOnly, bMappingOnly);
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

void YdvrReader::processYdvrDir(const std::string& stYdvrDir, const std::string& stWorkDir, bool bSummaryOnly, bool bMappingOnly) {

    // Read all .DAT files in the directory and subdirectories
    std::filesystem::path stCacheDir = std::filesystem::path(stWorkDir) / "ydvr" /"cache" ;
    std::filesystem::create_directories(stCacheDir);
    std::filesystem::path stSummaryDir = std::filesystem::path(stWorkDir) / "ydvr"/ "summary" ;
    std::filesystem::create_directories(stSummaryDir);

    auto files = std::filesystem::recursive_directory_iterator(stYdvrDir);

    // Sort files by name
    std::list<std::filesystem::directory_entry> filesSorted;
    for( const auto& file : files ) {
        if (file.path().extension() == ".DAT") {
            filesSorted.push_back(file);
        }
    }

    filesSorted.sort([](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    });

    auto filesCount = (float)filesSorted.size();
    float fileNo = 0;

    for( const auto& file : filesSorted ) {
        processDatFile(file.path().string(), stCacheDir, stSummaryDir, bSummaryOnly, bMappingOnly);
        int progress = (int)round(fileNo / filesCount * 100.f);
        m_rProgressListener.progress(file.path().filename(), progress);
        if( m_rProgressListener.stopRequested() ) {
            m_rProgressListener.progress("Terminated", 100);
            break;
        }
        fileNo++;
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

void YdvrReader::processDatFile(const std::string &ydvrFile, const std::string& stCacheDir, const std::string& stSummaryDir, bool bSummaryOnly, bool bMappingOnly){
    std::cout << ydvrFile << std::endl;

    std::string csvFileName = std::filesystem::path(ydvrFile).filename().string() + ".csv";

    std::filesystem::path fileCacheDir = std::filesystem::path(stCacheDir) / std::filesystem::path(ydvrFile).parent_path().filename();
    std::filesystem::path fileSummaryDir = std::filesystem::path(stSummaryDir) / std::filesystem::path(ydvrFile).parent_path().filename();

    std::filesystem::path stCacheFile = bMappingOnly ? std::filesystem::path("/dev/null") : fileCacheDir  / csvFileName;
    std::filesystem::path stSummaryFile = fileSummaryDir  / csvFileName;

    auto haveCache = std::filesystem::exists(stCacheFile) && std::filesystem::is_regular_file(stCacheFile) ;

    auto haveSummary = std::filesystem::exists(stSummaryFile) && std::filesystem::is_regular_file(stSummaryFile) &&
                     std::filesystem::file_size(stSummaryFile) > 0;
    if (haveCache && haveSummary  && !bMappingOnly) {
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
        std::filesystem::create_directories(fileCacheDir);
        std::filesystem::create_directories(fileSummaryDir);

        readDatFile(ydvrFile, stCacheFile, stSummaryFile);
        if( m_ulEpochCount > 0 )
            m_listDatFiles.push_back(DatFileInfo{ydvrFile, stCacheFile, m_ulStartGpsTimeMs, m_ulEndGpsTimeMs, m_ulEpochCount});
    }

}

void YdvrReader::readDatFile(const std::string &ydvrFile, const std::filesystem::path &stCacheFile,
                             const std::filesystem::path &stSummaryFile) {

    std::ofstream cache (stCacheFile, std::ios::out);
    std::ifstream fs (ydvrFile, std::ios::in | std::ios::binary | std::ios::ate);
    std::streamsize size = fs.tellg();
    fs.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    auto buff = buffer.data();
    if (!fs.read(buff, size))
    {
        std::cerr << "Failed to read file: " << ydvrFile << std::endl;
        return;
    }
    fs.close();

    m_ulStartGpsTimeMs = 0;
    m_ulEndGpsTimeMs = 0;
    m_ulEpochCount = 0;

    size_t offset = 0;
    while (offset < size) {
        uint16_t ts;

        if ( offset + sizeof(ts) > size )
            break;
        memcpy(&ts, buff + offset, sizeof(ts));
        offset += sizeof(ts);

        uint32_t msg_id;
        if ( offset + sizeof(msg_id) > size )
            break;
        memcpy(&msg_id, buff + offset, sizeof(msg_id));
        offset += sizeof(msg_id);

        if( msg_id == 0xFFFFFFFF ) {  // YDVR service record
            uint8_t data[8];
            if ( offset + sizeof(data) > size )
                break;
            memcpy(data, buff + offset, sizeof(data));
            offset += sizeof(data);

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
            YdvrMessage m;
            canIdToN2k(msg_id, m.prio, m.pgn, m.src, m.dst);

            if ( m.pgn < 59392 ){
                std::cerr << "Invalid PGN: " << m.pgn << " file " << ydvrFile << " offset " << offset << std::endl;
            }
            const Pgn * pgn = searchForPgn(int(m.pgn));

            if ( pgn == nullptr ) { // Unknown to us PGN, just skip this message
                m.len = 8;
                if ( offset + m.len > size )
                    break;
                offset += m.len;
            }else if ( m.pgn == 59904 ) {
                m.len = 3;
                if ( offset + m.len > size )
                    break;
                m.data =  (uint8_t  *)(buff + offset);
                offset += m.len;

            } else if ( pgn->type == PACKET_FAST && pgn->complete == PACKET_COMPLETE ) {
                uint8_t seqNo;
                if ( offset + 1 > size )
                    break;
                seqNo = buff[offset];
                offset += 1;

                if ( offset + 1 > size )
                    break;
                m.len = buff[offset];
                offset += 1;

                if( m.len > FASTPACKET_MAX_SIZE ) {
                    std::cerr << "Invalid length: " << m.len <<  " offset " << offset << std::endl;
                    continue;
                }

                if ( offset + m.len > size )
                    break;
                m.data =  (uint8_t *)(buff + offset);
                offset += m.len;

//                std::cout << "Fast complete PGN: " << m.pgn << " src: " << int(m.src) << " len: " << int(m.len) <<  " offset " << offset << std::endl;
                ProcessPgn(m, cache);
            }else {
                // Might be a single frame message of fast message not known to YDVR
                m.len = 8;
                if ( offset + m.len > size )
                    break;
                m.data =  (uint8_t  *)(buff + offset);
                offset += m.len;

                if ( pgn->type == PACKET_FAST ) { // Some assembly is required

                    Packet *p = assembleFastPacket(&m);

                    if (p != nullptr) {
                        m.pgn = p->pgn;
                        m.src = p->src;
                        m.len = p->size;
                        m.data = p->data;
//                        std::cout << "Fast assembled PGN: " << m.pgn << " src: " << int(m.src) << " len: " << int(m.len) << " offset " << offset
//                                  << std::endl;
                        ProcessPgn(m, cache);
                    }
                }else if ( pgn->type == PACKET_SINGLE ){
//                    std::cout << "Single PGN: " << m.pgn << " src: " << int(m.src) << " len: " << int(m.len) << " offset " << offset
//                              << std::endl;
                    ProcessPgn(m, cache);
                }
            }
        }
    }

    cache.close();

    std::ofstream summary (stSummaryFile, std::ios::out);
    summary << "StartGpsTimeMs,EndGpsTimeMs,EpochCount" << std::endl;
    summary << m_ulStartGpsTimeMs << "," << m_ulEndGpsTimeMs << "," << m_ulEpochCount << std::endl;
    summary.close();
}

static void resetPacketBuffer(Packet *p){
    p->frames = 0;
    p->used   = false;
    p->allFrames = 0;
}


Packet *YdvrReader::assembleFastPacket(YdvrMessage *msg) {
    size_t     buffer;
    Packet    *p;

    // Find slot containing the sequence for given PGN from given SRC
    uint32_t seq      = msg->data[0] & 0xe0;
    for (buffer = 0; buffer < REASSEMBLY_BUFFER_SIZE; buffer++)
    {
        p = &reassemblyBuffer[buffer];

        if (p->used && p->pgn == msg->pgn && p->src == msg->src && p->seq == seq)
        {
            // Found existing slot
            break;
        }
    }

    // Not found, put message to the new slot
    if (buffer == REASSEMBLY_BUFFER_SIZE)
    {
        // Find a free slot
        for (buffer = 0; buffer < REASSEMBLY_BUFFER_SIZE; buffer++)
        {
            p = &reassemblyBuffer[buffer];
            if (!p->used)
            {
                break;
            }
        }
        if (buffer == REASSEMBLY_BUFFER_SIZE)
        {
            logError("Out of reassembly buffers; ignoring PGN %u\n", msg->pgn);
            return nullptr;
        }
        p->used   = true;
        p->src    = msg->src;
        p->pgn    = msg->pgn;
        p->frames = 0;
        p->seq    = seq;
    }


    // YDWG can receive frames out of order, so handle this.
    uint32_t frame    = msg->data[0] & 0x1f;
    size_t   idx      = (frame == 0) ? 0 : FASTPACKET_BUCKET_0_SIZE + (frame - 1) * FASTPACKET_BUCKET_N_SIZE;
    size_t   frameLen = (frame == 0) ? FASTPACKET_BUCKET_0_SIZE : FASTPACKET_BUCKET_N_SIZE;
    size_t   msgIdx   = (frame == 0) ? FASTPACKET_BUCKET_0_OFFSET : FASTPACKET_BUCKET_N_OFFSET;

    if ((p->frames & (1 << frame)) != 0)
    {
        logError("Received incomplete fast packet PGN %u from source %u\n", msg->pgn, msg->src);
        resetPacketBuffer(p);
        return nullptr;
    }

    if (frame == 0 && p->frames == 0)
    {
        p->size      = msg->data[1];
        p->allFrames = (1ULL << (1 + (p->size / 7))) - 1;
    }

    memcpy(&p->data[idx], &msg->data[msgIdx], frameLen);
    p->frames |= 1 << frame;

    logDebug("Using buffer %u for reassembly of PGN %u src %u: size %zu frame %u sequence %u idx=%zu frames=%x mask=%x\n",
             buffer,
             msg->pgn,
             msg->src,
             p->size,
             frame,
             seq,
             idx,
             p->frames,
             p->allFrames);

    if (p->frames == p->allFrames)
    {
        // Received all data
        resetPacketBuffer(p);
        return p;
    }

    return nullptr;
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

void YdvrReader::ProcessPgn(const YdvrMessage &msg, std::ofstream& cache)  {

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
        bool bCorrectSource = m_mapSrcForPgn[pgn->pgn] == msg.src;
        // Accept B&G key value from any source
        if (m_ignoreSourcesMap || bCorrectSource || PGN_BANG_KEY_VALUE == pgn->pgn)
        {
            switch(pgn->pgn){
                case 129029:
                    if ( processGpsFixPgn(pgn, data, len) ) {
                        ProcessEpochBatch(cache);
                    }else{
                        // Start accumulating over
                        m_epochBatch.clear();
                    }
                    break;
                case 129025:
                    processPosRapidUpdate(pgn, data, len);
                    ProcessEpoch();
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
                case 127258:
                    processMagneticVariation(pgn, data, len);
                    break;
                case 127237:
                    processHeadingControl(pgn, data, len);
                    break;
                case 128275:
                    processDistanceLog(pgn, data, len);
                    break;
                case 128267:
                    processWaterDepth(pgn, data, len);
                    break;
                case PGN_BANG_KEY_VALUE:
                    processBangProprietary(pgn, data, len);
                    break;
            }
        }
    }
}
extern "C" size_t getFieldOffsetByOrder(const Pgn *pgn, size_t order);
void YdvrReader::processBangProprietary(const Pgn *pgn, uint8_t *data, uint8_t dataLen) {
    bool haveMore = false;
    size_t startBit = getFieldOffsetByOrder(pgn, pgn->repeatingStart1);
    int64_t maxValue;
    int fieldNo = pgn->repeatingStart1 - 1;
    const Field *field;

    BangStartLineData startLineData;
    while(true) {
        int64_t val;
        int64_t valueLen;
        int64_t key;

        field = &pgn->fieldList[fieldNo];
        haveMore = extractNumber(field, data, dataLen, startBit, field->size, &key, &maxValue);
        if ( !haveMore )
            break;
        startBit += field->size;

        field = &pgn->fieldList[fieldNo + 1];
        extractNumber(field, data, dataLen, startBit, field->size, &valueLen, &maxValue);
        startBit += field->size;

        valueLen *= 8; // Convert to bits

        field = &pgn->fieldList[fieldNo + 2];
        extractNumber(field, data, dataLen, startBit, valueLen, &val, &maxValue);
        startBit += valueLen;

        lookupBangField(val, key, startLineData);
        // Check if we got valid start line data
        if( startLineData.portEndLat != DBL_MAX && startLineData.portEndLon != DBL_MAX &&
            startLineData.stbdEndLat != DBL_MAX && startLineData.stbdEndLon != DBL_MAX ) {
            m_epoch.startLinePortEnd = GeoLoc::fromDegrees(startLineData.portEndLat, startLineData.portEndLon, m_ulLatestGpsTimeMs);
            m_epoch.startLineStbdEnd = GeoLoc::fromDegrees(startLineData.stbdEndLat, startLineData.stbdEndLon, m_ulLatestGpsTimeMs);
        }
    }
}

void YdvrReader::lookupBangField(int64_t val, int64_t key, BangStartLineData &startLineData) {
    switch(key){
        case BANG_KEY_RACE_TIMER:  // "Race Timer", "TIME_UFIX32_MS"
            m_epoch.raceTimer = (int32_t)val;
            break;
        case BANG_KEY_TARGET_SPEED: // "Target Boat Speed", "SPEED_FIX16_CM"
            if ( isInt16Valid(val) ) {
                m_epoch.targetSpeed = Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.targetSpeed = Speed::INVALID;
            }
            break;
        case BANG_KEY_POLAR_SPEED: // "Polar Speed", "SPEED_UFIX16_CM"
            if ( isInt16Valid(val) ) {
                m_epoch.polarSpeed = Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.polarSpeed = Speed::INVALID;
            }
            break;
        case BANG_KEY_VMG: // "VMG", "SPEED_FIX16_CM"
            if ( isInt16Valid(val) ) {
                m_epoch.vmg = Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.vmg = Speed::INVALID;
            }
            break;
        case BANG_KEY_LEEWAY: // "Leeway", "ANGLE_FIX16"
            if ( isInt16Valid(val) ) {
                m_epoch.leeway = Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.leeway = Angle::INVALID;
            }
            break;
        case BANG_KEY_CURRENT_DRIFT: // "Current Drift", "SPEED_FIX16_CM"
            if ( isInt16Valid(val) ) {
                m_epoch.currentDrift = Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.currentDrift = Speed::INVALID;
            }
            break;
        case BANG_KEY_CURRENT_SET: // "Current Set", "ANGLE_FIX16"
            if ( isInt16Valid(val) ) {
                m_epoch.currentSet = Direction::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.currentSet = Direction::INVALID;
            }
            break;
        case BANG_KEY_DIST_BEHIND_START: // "Distance behind Start line", "DISTANCE_FIX32_CM"
            if ( isInt32Valid(val) ) {
                m_epoch.distToStart = Distance::fromMeters(double((int32_t)val) * RES_MPS, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.distToStart = Distance::INVALID;
            }
            break;
        case BANG_KEY_PILOT_TARGET_TWA: // "Pilot Target Wind Angle", "ANGLE_FIX16" Value
            if ( isInt16Valid(val) ) {
                double twaRads = double(val) * RES_RADIANS;
                // FIXME: For some reasons the negative value always more by fifteen degrees, so we correct it
                double correctedTwa = twaRads;
                if( twaRads < 0 || twaRads > M_PI )
                    correctedTwa -= 15.4 * M_PI / 180;
                m_epoch.pilotTwa = Angle::fromRadians(correctedTwa, m_ulLatestGpsTimeMs);
            } else {
                m_epoch.pilotTwa = Angle::INVALID;
            }
            break;
            case BANG_KEY_START_LINE_PORT_END_LAT:
            if ( isInt32Valid(val) ) {
                startLineData.portEndLat = double((int32_t)val) * RES_LL_32;
            }
            break;
        case BANG_KEY_START_LINE_PORT_END_LON:
            if ( isInt32Valid(val) ) {
                startLineData.portEndLon = double((int32_t)val) * RES_LL_32;
            }
            break;
        case BANG_KEY_START_LINE_STBD_END_LAT:
            if ( isInt32Valid(val) ) {
                startLineData.stbdEndLat = double((int32_t)val) * RES_LL_32;
            }
            break;
        case BANG_KEY_START_LINE_STBD_END_LON:
            if ( isInt32Valid(val) ) {
                startLineData.stbdEndLon = double((int32_t)val) * RES_LL_32;
            }
            break;
        default:
//            printf("Unknown key %d with value %08X,%d, %.5f\n", key, val, val, double(val) *  RES_LL_32);
            break;
    }
}

/// COG & SOG, Rapid Update
void YdvrReader::processCogSogPgn(const Pgn *pgn, const uint8_t *data, size_t len) {
    int64_t val;
    int64_t type;
    extractNumberByOrder(pgn, 2, data, len, &type);
    extractNumberByOrder(pgn, 4, data, len, &val);
    double rad = double(val) *  RES_RADIANS;
    bool isCogValid = isUint16Valid(val);
    if (isCogValid){
        if ( type == 0) {  // True ( convert to magnetic
            m_epoch.cog =  m_magVar.isValid(m_ulLatestGpsTimeMs) ? Direction::fromRadians(rad - m_magVar.getRadians(), m_ulLatestGpsTimeMs) : Direction::INVALID;
        }else if ( type == 1) { // Magnetic ( no need to convert)
            m_epoch.cog =  Direction::fromRadians(rad, m_ulLatestGpsTimeMs);
        }else { // Invalid
            m_epoch.cog =  Direction::INVALID;
        }
    }else{
        m_epoch.cog =  Direction::INVALID;
    }

    extractNumberByOrder(pgn, 5, data, len, &val);
    m_epoch.sog = isUint16Valid(val) ? Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs) : Speed::INVALID;
}

void YdvrReader::processVesselHeading(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    bool isHeadingValid = isUint16Valid(val);
    double rad = double(val) * RES_RADIANS;

    extractNumberByOrder(pgn, 5, data, len, &val);
    if ( val == 1 && isHeadingValid) { // Magnetic
        m_epoch.mag = Direction::fromRadians(rad, m_ulLatestGpsTimeMs);
    }else if( val == 0 && isHeadingValid ) { // true
        m_epoch.mag = m_magVar.isValid(m_ulLatestGpsTimeMs) ? Direction::fromRadians(rad - m_magVar.getRadians(), m_ulLatestGpsTimeMs) : Direction::INVALID;
    }
}

/// GNSS Position Data
bool YdvrReader::processGpsFixPgn(const Pgn *pgn, const uint8_t *data, size_t len) {
    int64_t fixQuality;
    extractNumberByOrder(pgn, 8, data, len, &fixQuality);
    if( fixQuality > 0 ){
        int64_t date;
        extractNumberByOrder(pgn, 2, data, len, &date);
        if( date > 40000){
            std::cerr << "Invalid date: " << date << std::endl;
            return false;
        }
        int64_t time;
        extractNumberByOrder(pgn, 3, data, len, &time);

        auto ulGpsTime = date * 86400 * 1000 + time / 10;
        if ( ulGpsTime <= m_ulLatestGpsTimeMs && !m_ignoreSourcesMap){  // If we ignore sources the GPS from other source can be out of order
            std::cout << "GPS time went backward" << std::endl;
            return false;
        }
        m_ulLatestGpsTimeMs = ulGpsTime;
        m_epoch.utc = UtcTime::fromUnixTimeMs(m_ulLatestGpsTimeMs);

        int64_t val;
        extractNumberByOrder(pgn, 4, data, len, &val);
        double lat = double(val) * RES_LL_64;
        extractNumberByOrder(pgn, 5, data, len, &val);
        double lon = double(val) * RES_LL_64;
        m_epoch.loc = GeoLoc::fromDegrees(lat, lon, m_ulLatestGpsTimeMs);
        return true;
    }
    return false;
}

/// Position, Rapid Update
void YdvrReader::processPosRapidUpdate(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 1, data, len, &val);
    double lat = double(val) *  RES_LL_32;
    extractNumberByOrder(pgn, 2, data, len, &val);
    double lon = double(val) *  RES_LL_32;
    m_epoch.loc = GeoLoc::fromDegrees(lat, lon, m_ulLatestGpsTimeMs);
    m_epoch.utc = UtcTime::INVALID;
}


void YdvrReader::processBoatSpeed(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    m_epoch.sow = isUint16Valid(val) ? Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs) : Speed::INVALID;

    extractNumberByOrder(pgn, 3, data, len, &val);
    extractNumberByOrder(pgn, 4, data, len, &val);
}

void YdvrReader::processWindData(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    Speed windSpeed = isUint16Valid(val) ? Speed::fromMetersPerSecond(double(val) * RES_MPS, m_ulLatestGpsTimeMs) : Speed::INVALID;
    extractNumberByOrder(pgn, 3, data, len, &val);
    Angle windAngle = isUint16Valid(val) ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
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
    m_epoch.rdr = isInt16Valid(val) ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
}

void YdvrReader::processHeadingControl(const Pgn *pgn, uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 9, data, len, &val);  // "Commanded Rudder Direction"
    if ( val < 7 ){
        extractNumberByOrder(pgn, 10, data, len, &val);  // "Commanded Rudder Angle"
        m_epoch.cmdRdr = isInt16Valid(val) ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
    }else{
        m_epoch.cmdRdr = Angle::INVALID;
    }

    extractNumberByOrder(pgn, 11, data, len, &val);  // "Heading-To-Steer"
    if ( isUint16Valid(val) ){
        double rad = double(val) *  RES_RADIANS;
        // Convert to magnetic
        m_epoch.hdgToSteer = m_magVar.isValid(m_ulLatestGpsTimeMs) ? Direction::fromRadians(rad - m_magVar.getRadians(), m_ulLatestGpsTimeMs)
                                        : Direction::INVALID;
    }else{
        m_epoch.hdgToSteer = Direction::INVALID;
    }
}

void YdvrReader::processAttitude(const Pgn *pgn, const uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 2, data, len, &val);
    m_epoch.yaw = isUint16Valid(val) ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
    extractNumberByOrder(pgn, 3, data, len, &val);
    m_epoch.pitch = isUint16Valid(val) ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
    extractNumberByOrder(pgn, 4, data, len, &val);
    m_epoch.roll = isUint16Valid(val) ? Angle::fromRadians(double(val) * RES_RADIANS, m_ulLatestGpsTimeMs) : Angle::INVALID;
}

void YdvrReader::processMagneticVariation(const Pgn *pgn, uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 5, data, len, &val);
    if (isInt16Valid(val)){
        // Positive value is easterly
        double rad = double(val) * RES_RADIANS;
        m_magVar = Angle::fromRadians(rad, m_ulLatestGpsTimeMs);
        m_epoch.magVar = m_magVar;
    }
}


void YdvrReader::processDistanceLog(const Pgn *pgn, uint8_t *data, uint8_t len) {
    int64_t val;
    extractNumberByOrder(pgn, 3, data, len, &val);
    m_epoch.log = Distance::fromMeters(val, m_ulLatestGpsTimeMs);
}

void YdvrReader::processWaterDepth(const Pgn *pgn, uint8_t *data, uint8_t len) {
    int64_t val;

    // Offset
    // Positive values - distance form transducer to waterline
    // Negative values - distance from the transducer to the keel
    extractNumberByOrder(pgn, 3, data, len, &val);
    int16_t offsetMm = 0;
    if ( isInt16Valid(val) ){
        offsetMm = val;
    }

    // Water Depth, Transducer
    extractNumberByOrder(pgn, 2, data, len, &val);
    uint32_t depthMm = val * 10;
    m_epoch.depth = isUint32Valid(val) ? Distance::fromMillimeters(depthMm + offsetMm, m_ulLatestGpsTimeMs) : Distance::INVALID;
}


void YdvrReader::processProductInformationPgn(uint8_t  src, const Pgn *pgn, const uint8_t *data, uint8_t len)  {

    char acModelId[32];
    extractStringByOrder(pgn, 3, data, len, acModelId, sizeof(acModelId));

    char asSerialCode[32];
    extractStringByOrder(pgn, 6, data, len, asSerialCode, sizeof(asSerialCode));

    std::string serialCode(asSerialCode);
    std::string modelId(acModelId);

    std::string modelIdAndSerialCode = trim(modelId) + "-" + trim(serialCode);
    deCommas(modelIdAndSerialCode);
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

void YdvrReader::ProcessEpoch() {
    m_epochBatch.push_back(m_epoch);
}

void YdvrReader::ProcessEpochBatch(std::ofstream &cache) {
    auto thisEpochUtcMs = m_epoch.utc.getUnixTimeMs();

    if( m_ulStartGpsTimeMs == 0 ){
        m_ulStartGpsTimeMs = thisEpochUtcMs;
    }
    m_ulEndGpsTimeMs = thisEpochUtcMs;

    if( thisEpochUtcMs < m_ulPrevEpochUtcMs) {
        std::cerr << "Out of order epoch " << std::endl;
        m_ulPrevEpochUtcMs = 0;
    }
    if( (thisEpochUtcMs - m_ulPrevEpochUtcMs) > 10000 ){
        std::cerr << "Too large gaps between epochs" << std::endl;
        m_ulPrevEpochUtcMs = 0;
    }

    if( m_ulPrevEpochUtcMs != 0) {
        if( m_epochBatch.size() == 10 ){
            m_epochBatch.pop_back();  // Let's assume last rapid position report is the same as first one on 10 Hz B&G system
        }

        // Go through all epochs obtained from rapid GPS  update and update their timestamps by spreading them uniformly
        uint64_t updateRate = (thisEpochUtcMs - m_ulPrevEpochUtcMs) / (m_epochBatch.size() + 1);
        uint64_t epochUtcMs = m_ulPrevEpochUtcMs + updateRate;
        for( auto epoch: m_epochBatch ){
            epoch.utc = UtcTime::fromUnixTimeMs(epochUtcMs);
            m_ulEpochCount++;
            cache << static_cast<std::string>(epoch) << std::endl;
            epochUtcMs += updateRate;
        }
    }

    // Now write the epoch from full GPS update (one with valid utc time)
    m_ulEpochCount++;
    cache << static_cast<std::string>(m_epoch) << std::endl;
    m_ulPrevEpochUtcMs = thisEpochUtcMs;
    m_epochBatch.clear();
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

    m_mapDeviceForPgn.clear();
    std::cout  << "Reading PGN sources from " << csvFileName << std::endl;
    while (std::getline(f, line)) {
        std::cout  << line << std::endl;
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        if ( tokens.size() > 3 ){
            uint32_t pgn = std::stoul(tokens[0]);
            std::string desc = tokens[1];
            std::vector<std::string> srcs;
            uint32_t srcIdx = std::stoul(tokens[2]);
            for (int i = 3; i < tokens.size(); i++){
                srcs.push_back(tokens[i]);
            }
            m_mapDeviceForPgn[pgn] = srcs[srcIdx];
            std::cout  << pgn << " " << desc << " " << srcs[srcIdx] << std::endl;
        }
    }

}

void YdvrReader::getPgnData(std::map<uint32_t, std::vector<std::string>> &mapPgnDevices,
                            std::map<uint32_t, std::string> &mapPgnDescription) {

    std::cout << "---------------------------------------" << std::endl;
    std::cout << "DeviceName SerialNo, PGN, Description" << std::endl;
    for (auto & mapIt : m_mapPgnsByDeviceModelAndSerialNo) {
        for (auto setIt = mapIt.second.begin(); setIt != mapIt.second.end(); setIt++) {
            int pgn = int(*setIt);

            // Check if we need this PGN
            if (REQUIRED_PGNS.find(pgn) == REQUIRED_PGNS.end() )
                continue;

            auto deviceName = mapIt.first;
            std::cout << deviceName << "," << pgn ;

            if (mapPgnDevices.find(pgn) == mapPgnDevices.end() ) {
                auto *l = new  std::vector<std::string>();
                mapPgnDevices[pgn] = *l;
            }
            mapPgnDevices[pgn].push_back(deviceName);

            const Pgn *pPgn = searchForPgn(pgn);
            if (pPgn != nullptr && pPgn->description != nullptr) {
                std::string description(pPgn->description);
                deCommas(description);
                mapPgnDescription[pgn] = description;
                std::cout << ",\"" << description << "\"";
            }else{
                mapPgnDescription[pgn] = "unknown";
            }
            std::cout << std::endl;
        }
    }
    std::cout << "---------------------------------------" << std::endl;

}
