#include <iostream>
#include <fstream>
#include <utility>
#include "GoPro.h"

#include "GPMF_parser.h"
#include "GPMF_mp4reader.h"
#include "ffmpeg/FFMpeg.h"

GoPro::GoPro(const std::string &stYGoProDir, const std::string &stCacheDir, InstrDataReader &rInstrDataReader,
             IProgressListener& rProgressListener)
: m_rInstrDataReader(rInstrDataReader), m_rProgressListener(rProgressListener){
    processGoProDir(stYGoProDir, stCacheDir);
}

void GoPro::processGoProDir(const std::string &stGoProDir, const std::string &stWorkDir) {
    std::filesystem::path stCacheDir;
    stCacheDir = std::filesystem::path(stWorkDir) / "gopro" / "cache";
    std::filesystem::create_directories(stCacheDir);
    std::filesystem::path stSummaryDir = std::filesystem::path(stWorkDir) / "gopro" / "summary" ;
    std::filesystem::create_directories(stSummaryDir);

    auto files = std::filesystem::recursive_directory_iterator(stGoProDir);
    float filesCount = 0;
    for( const auto& file : files )
        if (file.path().extension() == ".MP4")
            filesCount++;
    files = std::filesystem::recursive_directory_iterator(stGoProDir);
    float fileNo = 0;
    for( const auto& file : files ) {
        if (file.path().extension() == ".MP4") {
            processMp4File(file.path().string(), stCacheDir, stSummaryDir);
            int progress = (int)round(fileNo / filesCount * 100.f);
            m_rProgressListener.progress(file.path().filename(), progress);
            if( m_rProgressListener.stopRequested() ) {
                m_rProgressListener.progress("Terminated", 100);
                break;
            }
            fileNo++;
        }
    }

    // Sort the list by start time
    m_GoProClipInfoList.sort([](const GoProClipInfo &a, const GoProClipInfo &b) {
        return a.getClipStartUtcMs() < b.getClipStartUtcMs();
    });
}


void GoPro::processMp4File(const std::string &mp4FileName, const std::filesystem::path &cacheDir,
                           const std::filesystem::path &summaryDir) {


    std::cout << mp4FileName << std::endl;

    std::string csvFileName = std::filesystem::path(mp4FileName).filename().string() + ".csv";
    std::filesystem::path stCacheFile = std::filesystem::path(cacheDir)  / csvFileName;
    std::filesystem::path stSummaryFile = std::filesystem::path(summaryDir)  / csvFileName;

    auto haveCache = std::filesystem::exists(stCacheFile) && std::filesystem::is_regular_file(stCacheFile) ;

    auto haveSummary = std::filesystem::exists(stSummaryFile) && std::filesystem::is_regular_file(stSummaryFile) &&
                       std::filesystem::file_size(stSummaryFile) > 0;

    int width, height;
    if( haveCache && haveSummary ) {
        std::cout << "have cache and summary" << std::endl;
        auto *pInstrData = new std::list<InstrumentInput>;
        ReadCacheAndSummary(stCacheFile, stSummaryFile, *pInstrData,width, height);
        GoProClipInfo goProClipInfo(mp4FileName, m_ulClipStartUtcMs, m_ulClipEndUtcMs,
                                    pInstrData, width, height);
        m_GoProClipInfoList.push_back(goProClipInfo);
    }else{
        auto *goProInstrData = new std::list<InstrumentInput>();
        readMp4File(mp4FileName, *goProInstrData);
        std::tie(width, height) = FFMpeg::getVideoResolution(mp4FileName);

        if ( !goProInstrData->empty()){
            std::ofstream ofsCache(stCacheFile);
            std::ofstream ofsSummary(stSummaryFile);
            ofsSummary << "StartGpsTimeMs,EndGpsTimeMs,width,height" << std::endl;
            ofsSummary << m_ulClipStartUtcMs << "," << m_ulClipEndUtcMs << "," << width << "," << height << std::endl;
        }

        // Now tey to get the instrument data from instruments themselves
        auto *instrData = new std::list<InstrumentInput>();
        m_rInstrDataReader.read(m_ulClipStartUtcMs, m_ulClipEndUtcMs, *instrData);
        if ( instrData->empty()) {
            // Use the data from GOPRO
            storeCacheFile(stCacheFile, *goProInstrData);
            GoProClipInfo goProClipInfo(mp4FileName, m_ulClipStartUtcMs, m_ulClipEndUtcMs,
                                        goProInstrData, width, height);
            m_GoProClipInfoList.push_back(goProClipInfo);
        }else{
            // Use the data from instruments
            GoProClipInfo goProClipInfo(mp4FileName, m_ulClipStartUtcMs, m_ulClipEndUtcMs,
                                        instrData, width, height);
            storeCacheFile(stCacheFile, *instrData);
            m_GoProClipInfoList.push_back(goProClipInfo);
        }
    }
}


time_t GoPro::strToTime(const std::string& str) {
    struct tm tm{};
    strptime(str.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
    return  timegm(&tm);
}

void GoPro::readMp4File(const std::string &mp4FileName, std::list<InstrumentInput> &listInputs) {
    m_ulClipStartUtcMs = 0;
    m_ulClipEndUtcMs = 0;


    char *fname = const_cast<char *>(mp4FileName.c_str());
    size_t mp4handle = OpenMP4Source(fname, MOV_GPMF_TRAK_TYPE, MOV_GPMF_TRAK_SUBTYPE, 0);
    if (mp4handle == 0) {
        std::cerr << "error: " << mp4FileName << " is an invalid MP4/MOV or it has no GPMF data" << std::endl;
    }

    double metadatalength = GetDuration(mp4handle);

    if (metadatalength > 0.0) {
        uint32_t payloads = GetNumberPayloads(mp4handle);
        for (uint32_t index = 0; index < payloads; index++) {
            double in = 0.0, out = 0.0; //times
            uint32_t payloadsize = GetPayloadSize(mp4handle, index);
            size_t payloadres=0;
            payloadres = GetPayloadResource(mp4handle, payloadres, payloadsize);
            uint32_t *payload = GetPayload(mp4handle, payloadres, index);
            if (payload == nullptr)
                return;

            GPMF_ERR ret = GetPayloadTime(mp4handle, index, &in, &out);
            if (ret != GPMF_OK)
                return;

            GPMF_stream gs_stream;
            ret = GPMF_Init(&gs_stream, payload, payloadsize);
            if (ret != GPMF_OK)
                return;

            GPMF_ResetState(&gs_stream);
            GPMF_ERR nextret;

            double lat = 0;
            double lon = 0;
            double alt = 0;
            double sog_ms = 0;
            double speed3d_ms = 0;
            bool   valid = false;
            char   utc[256] = "";
            do {
                if ( m_rProgressListener.stopRequested() )
                    return;

                uint32_t key = GPMF_Key(&gs_stream);

                switch (key) {
                    case STR2FOURCC("GPS5"):{
                        auto *data = (int32_t *) GPMF_RawData(&gs_stream);
                        // Get only the first position
                        lat = (double)BYTESWAP32(data[0]) / 10000000.;
                        lon = (double)BYTESWAP32(data[1]) / 10000000.;
                        alt = (double)BYTESWAP32(data[2]) / 1000.;
                        sog_ms = (double)BYTESWAP32(data[3]) / 1000.;
                        speed3d_ms = (double)BYTESWAP32(data[4]) / 100.;
                    }
                        break;

                    case STR2FOURCC("GPSF"):{
                        auto *data = (uint32_t *) GPMF_RawData(&gs_stream);
                        valid = BYTESWAP32(data[0]) != 0;
                    }
                        break;

                    case STR2FOURCC("GPSU"): {
                        char *data = (char *) GPMF_RawData(&gs_stream);
                        sprintf(utc,"20%2.2s-%2.2s-%2.2sT%2.2s:%2.2s:%2.2s.%3.3s",
                                data, data + 2, data + 4, data + 6, data + 8, data + 10 , data+13);
                        time_t t = strToTime(utc);
                        char acMsStr[4];
                        memcpy(acMsStr, data+13, 3);
                        acMsStr[3] = '\0';
                        uint16_t ms = atoi(acMsStr);
//                        printf("t=%ld", t);
                        if ( m_ulClipStartUtcMs == 0 ) {
                            m_ulClipStartUtcMs = t * 1000 + ms;
                        }
                        m_ulClipEndUtcMs = t * 1000 + ms;
                    }
                        break;
                    default:
                        break;
                }

                nextret = GPMF_Next(&gs_stream, (GPMF_LEVELS)(GPMF_RECURSE_LEVELS | GPMF_TOLERANT));

            } while (GPMF_OK == nextret); // Scan through all GPMF data
            GPMF_ResetState(&gs_stream);


            if( valid ) {
//                printf("%d,%f,%s,", index, in,  "True" );
//                printf("%s,%.5f,%.5f,%.1f,%.1f,%.1f\n", utc, lat, lon, alt, sog_ms, speed3d_ms);
                InstrumentInput ii;
                ii.utc = UtcTime::fromUnixTimeMs(m_ulClipEndUtcMs);
                ii.loc = GeoLoc::fromDegrees(lat, lon);
                ii.sog = Speed::fromMetersPerSecond(sog_ms);
                listInputs.push_back(ii);
            }else {
                printf("%d,%f,%s,", index, in, "False");
                printf("%s,,\n", utc);
            }
        }
    }
}

void GoPro::storeCacheFile(const std::filesystem::path &cacheFile, const std::list<InstrumentInput>& instrDataList) {
    std::ofstream cache (cacheFile, std::ios::out);
    for ( const InstrumentInput &ii : instrDataList ) {
        cache << static_cast<std::string>(ii) << std::endl;
    }

}

void GoPro::ReadCacheAndSummary(const std::filesystem::path &pathCacheFile, const std::filesystem::path &pathSummaryFile,
                                std::list<InstrumentInput> &instrData, int &width, int &height) {

    // Read summary file
    std::ifstream summary(pathSummaryFile);
    std::string line;
    std::getline(summary, line); // Skip header
    std::getline(summary, line);
    std::stringstream ss(line);
    std::string item;
    std::getline(ss, item, ',');
    m_ulClipStartUtcMs = std::stoull(item);
    std::getline(ss, item, ',');
    m_ulClipEndUtcMs = std::stoull(item);
    std::getline(ss, item, ',');
    width = std::stoi(item);
    std::getline(ss, item, ',');
    height = std::stoi(item);

    // Read cache file
    std::ifstream cache(pathCacheFile);
    while (std::getline(cache, line)) {
        instrData.push_back(InstrumentInput::fromString(line));
    }
}

const std::list<GoProClipInfo> &GoPro::getGoProClipList() const {
    return m_GoProClipInfoList;
}

GoProClipInfo::GoProClipInfo(std::string mp4FileName, uint64_t ulClipStartUtcMs, uint64_t ulClipEndUtcMs, std::list<InstrumentInput> *pInstrDataList, int w, int h)
:m_mp4FileName(std::move(mp4FileName)),
m_ulClipStartUtcMs(ulClipStartUtcMs), m_ulClipEndUtcMs(ulClipEndUtcMs), m_pInstrDataList(pInstrDataList),
m_width(w), m_height(h)
{

}

const std::list<InstrumentInput> *GoProClipInfo::getInstrData() const {
    return m_pInstrDataList;
}

