
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "ExpeditionReader.h"
#include "magnetic/wmm.h"

ExpeditionReader::ExpeditionReader(const std::string& stDataDir, const std::string& stWorkDir, IProgressListener& rProgressListener)
:m_rProgressListener(rProgressListener){

    // Read all .DAT files in the directory and subdirectories
    std::filesystem::path stCacheDir = std::filesystem::path(stWorkDir) / "expedition" /"cache" ;
    std::filesystem::create_directories(stCacheDir);
    std::filesystem::path stSummaryDir = std::filesystem::path(stWorkDir) / "expedition"/ "summary" ;
    std::filesystem::create_directories(stSummaryDir);

    auto files = std::filesystem::recursive_directory_iterator(stDataDir);
    float filesCount = 0;
    for( const auto& file : files )
        if (boost::algorithm::to_lower_copy(std::string(file.path().extension())) == ".csv")
            filesCount++;

    files = std::filesystem::recursive_directory_iterator(stDataDir);
    float fileNo = 0;
    for( const auto& file : files ) {
        if (boost::algorithm::to_lower_copy(std::string(file.path().extension())) == ".csv") {
            processExpeditionFile(file.path().string(), stCacheDir, stSummaryDir);
            int progress = (int)round(fileNo / filesCount * 100.f);
            m_rProgressListener.progress(file.path().filename(), progress);
            if( m_rProgressListener.stopRequested() ) {
                m_rProgressListener.progress("Terminated", 100);
                break;
            }
            fileNo++;
        }
    }
}

ExpeditionReader::~ExpeditionReader() = default;

void ExpeditionReader::read(uint64_t ulStartUtcMs, uint64_t ulEndUtcMs, std::list<InstrumentInput> &listInputs) {
    for( const ExpeditionFileInfo &di : m_listExpeditionFiles){
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

void ExpeditionReader::processExpeditionFile(std::string expCsvFile, std::filesystem::path stCacheDir,
                                             std::filesystem::path stSummaryDir) {

    std::string csvFileName = std::filesystem::path(expCsvFile).filename().string() + ".csv";
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
            m_listExpeditionFiles.push_back(ExpeditionFileInfo{expCsvFile, stCacheFile, m_ulStartGpsTimeMs, m_ulEndGpsTimeMs, m_ulEpochCount});
    } else {
        readExpeditionFile(expCsvFile, stCacheFile, stSummaryFile);
        if( m_ulEpochCount > 0 )
            m_listExpeditionFiles.push_back(ExpeditionFileInfo{expCsvFile, stCacheFile, m_ulStartGpsTimeMs, m_ulEndGpsTimeMs, m_ulEpochCount});
    }


}

void ExpeditionReader::readExpeditionFile(std::string expeditionFile, std::filesystem::path stCacheFile,
                                          std::filesystem::path stSummaryFile) {
    std::ofstream cache (stCacheFile, std::ios::out);

    m_ulStartGpsTimeMs = 0;
    m_ulEndGpsTimeMs = 0;
    m_ulEpochCount = 0;

    std::cout << "Reading Expedition file: " << expeditionFile << std::endl;
    std::string line;
    std::ifstream expStream (expeditionFile, std::ios::in);

    bool gotHeader = false;
    std::vector<std::string> headerItems;
    while (std::getline(expStream, line)) {
        boost::trim(line);
        std::stringstream tokenStream(line);
        std::string item;
        if ( gotHeader){
            int i = -1;
            InstrumentInput ii;
            double lat=0;
            bool gotLat = false;
            double lon=0;
            bool gotLon = false;
            while (std::getline(tokenStream, item, ',')) {
                i++;
                if (item.empty() ){
                    continue;
                }
                if( i < headerItems.size()){
                    std::string row = headerItems[i];
                    try {
                        double val = std::stod(item);
                        if( row == "UTC"){
                            // Expedition UTC is a Microsoft DATE type
                            // https://learn.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=msvc-170&viewFallbackFrom=vs-2019
                            uint64_t ulGpsTimeMs = (val - 25569) * 86400 * 1000;
                            if( m_ulStartGpsTimeMs == 0 ){
                                m_ulStartGpsTimeMs = ulGpsTimeMs;
                            }
                            m_ulEndGpsTimeMs = ulGpsTimeMs;
                            ii.utc = UtcTime::fromUnixTimeMs(ulGpsTimeMs);
                        } else if ( row == "Lat") {
                            lat = val;
                            gotLat = true;
                        } else if ( row == "Lon") {
                            lon = val;
                            gotLon = true;
                        } else if ( row == "COG" ) {
                            ii.cog = Direction::fromDegrees(val, m_ulEndGpsTimeMs);
                        } else if ( row == "SOG" ) {
                            ii.sog = Speed::fromKnots(val, m_ulEndGpsTimeMs);
                        } else if ( row == "AWS" ) {
                            ii.aws = Speed::fromKnots(val, m_ulEndGpsTimeMs);
                        } else if ( row == "AWA" ) {
                            ii.awa = Angle::fromDegrees(val, m_ulEndGpsTimeMs);
                        } else if ( row == "TWS" ) {
                            ii.tws = Speed::fromKnots(val, m_ulEndGpsTimeMs);
                        } else if ( row == "TWA" ) {
                            ii.twa = Angle::fromDegrees(val, m_ulEndGpsTimeMs);
                        } else if ( row == "HDG" ) {
                            ii.mag = Direction::fromDegrees(val, m_ulEndGpsTimeMs);
                        } else if ( row == "BSP" ) {
                            ii.sow = Speed::fromKnots(val, m_ulEndGpsTimeMs);
                        }else if ( row == "Rudder" ) {
                            ii.rdr = Angle::fromDegrees(val, m_ulEndGpsTimeMs);
                        }else if ( row == "Depth" ) {
                            ii.depth = Distance::fromMillimeters(val * 1000, m_ulEndGpsTimeMs);
                        }
                    }catch(...) {
                        continue;
                    }
                }
            }

            if ( gotLat && gotLon ){
                double year = 1970 + m_ulEndGpsTimeMs / 1000. / 3600. / 24 / 365.25;
                double magVar = computeMagDecl(lat, lon, year);
                ii.magVar = Angle::fromDegrees(magVar, m_ulEndGpsTimeMs);

                // Make COG magnetic
                if ( ii.cog.isValid(m_ulEndGpsTimeMs) ){
                    ii.cog = Direction::fromDegrees(ii.cog.getDegrees() - magVar, m_ulEndGpsTimeMs);
                }

                ii.loc = GeoLoc::fromDegrees(lat, lon, m_ulEndGpsTimeMs);
                cache << static_cast<std::string>(ii) << std::endl;
                m_ulEpochCount++;
            }

        }else{
            while (std::getline(tokenStream, item, ',')) {
                headerItems.push_back(item);
            }
            gotHeader = true;
        }
    }
    cache.close();

    std::ofstream summary (stSummaryFile, std::ios::out);
    summary << "StartGpsTimeMs,EndGpsTimeMs,EpochCount" << std::endl;
    summary << m_ulStartGpsTimeMs << "," << m_ulEndGpsTimeMs << "," << m_ulEpochCount << std::endl;
    summary.close();

}
