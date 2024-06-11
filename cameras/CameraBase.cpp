#include "CameraBase.h"

#include <utility>

CameraBase::CameraBase(const std::string& cameraName, const std::string& cameraClipExtension,
                       InstrDataReader &rInstrDataReader,
                       IProgressListener &rProgressListener)
:m_cameraName(std::move(cameraName)), m_cameraClipExtension(std::move(cameraClipExtension)),
 m_rInstrDataReader(rInstrDataReader), m_rProgressListener(rProgressListener)
{
}

void CameraBase::processClipsDir(const std::string &stClipsDir, const std::string &stWorkDir) {
    std::filesystem::path stCacheDir;
    stCacheDir = std::filesystem::path(stWorkDir) / m_cameraName / "cache";
    std::filesystem::create_directories(stCacheDir);
    std::filesystem::path stSummaryDir = std::filesystem::path(stWorkDir) / m_cameraName / "summary" ;
    std::filesystem::create_directories(stSummaryDir);

    auto files = std::filesystem::recursive_directory_iterator(stClipsDir);
    float filesCount = 0;
    for( const auto& file : files )
        if (file.path().extension() == m_cameraClipExtension)
            filesCount++;
    files = std::filesystem::recursive_directory_iterator(stClipsDir);
    float fileNo = 0;
    for( const auto& file : files ) {
        if (file.path().extension() == m_cameraClipExtension) {
            processClipFile(file.path().string(), stCacheDir, stSummaryDir);
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
    m_clipInfoList.sort([](const CameraClipInfo *a, const CameraClipInfo *b) {
        return a->getClipStartUtcMs() < b->getClipStartUtcMs();
    });
}

void CameraBase::processClipFile(const std::string &mp4FileName, const std::filesystem::path &cacheDir,
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
        auto goProClipInfo = new CameraClipInfo (mp4FileName, m_ulClipStartUtcMs, m_ulClipEndUtcMs,
                                    pInstrData, width, height);
        m_clipInfoList.push_back(goProClipInfo);
    }else{
        auto *cameraInstrData = new std::list<InstrumentInput>();
        std::tie(width, height) = readClipFile(mp4FileName, *cameraInstrData);

        if ( width != -1 ){
            std::ofstream ofsCache(stCacheFile);
            std::ofstream ofsSummary(stSummaryFile);
            ofsSummary << "StartGpsTimeMs,EndGpsTimeMs,width,height" << std::endl;
            ofsSummary << m_ulClipStartUtcMs << "," << m_ulClipEndUtcMs << "," << width << "," << height << std::endl;
        }

        // Now try to get the instrument data from instruments themselves
        auto *instrData = new std::list<InstrumentInput>();
        m_rInstrDataReader.read(m_ulClipStartUtcMs, m_ulClipEndUtcMs, *instrData);
        if ( instrData->empty()) {
            // Use the data from camera
            storeCacheFile(stCacheFile, *cameraInstrData);
            auto clipInfo = new CameraClipInfo (mp4FileName, m_ulClipStartUtcMs, m_ulClipEndUtcMs,
                                    cameraInstrData, width, height);
            m_clipInfoList.push_back(clipInfo);
        }else{
            // Use the data from instruments
            auto clipInfo = new CameraClipInfo(mp4FileName, m_ulClipStartUtcMs, m_ulClipEndUtcMs,
                                    instrData, width, height);
            storeCacheFile(stCacheFile, *instrData);
            m_clipInfoList.push_back(clipInfo);
        }
    }
}

void CameraBase::storeCacheFile(const std::filesystem::path &cacheFile, const std::list<InstrumentInput>& instrDataList) {
    std::ofstream cache (cacheFile, std::ios::out);
    for ( const InstrumentInput &ii : instrDataList ) {
        cache << static_cast<std::string>(ii) << std::endl;
    }

}

void CameraBase::ReadCacheAndSummary(const std::filesystem::path &pathCacheFile, const std::filesystem::path &pathSummaryFile,
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
