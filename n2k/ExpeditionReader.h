#ifndef SAILVUE_EXPEDITIONREADER_H
#define SAILVUE_EXPEDITIONREADER_H


#include "navcomputer/InstrumentInput.h"
#include "navcomputer/IProgressListener.h"
#include "../InstrDataReader.h"
#include <vector>

class ExpeditionFileInfo {
public:
    std::string stExpeditionFile;
    std::string stCacheFile;
    uint64_t m_ulStartGpsTimeMs = 0;
    uint64_t m_ulEndGpsTimeMs = 0;
    uint32_t m_ulEpochCount = 0;
};

class ExpeditionReader  : public InstrDataReader {
public:
    ExpeditionReader(const std::string& stDataDir, const std::string& stCacheDir, IProgressListener& rProgressListener);
    ~ExpeditionReader();
    void read(uint64_t ulStartUtcMs, uint64_t ulEndUtcMs, std::list<InstrumentInput> &listInputs) override;
private:
    IProgressListener& m_rProgressListener;
    uint64_t m_ulStartGpsTimeMs = 0;
    uint64_t m_ulEndGpsTimeMs = 0;
    uint32_t m_ulEpochCount = 0;
    std::list<ExpeditionFileInfo> m_listExpeditionFiles;

    void processExpeditionFile(std::string xpCsvFile, std::filesystem::path cacheDir, std::filesystem::path summaryDir);

    void
    readExpeditionFile(std::string expeditionFile, std::filesystem::path stCacheFile,
                       std::filesystem::path stSummaryFile);
};


#endif //SAILVUE_EXPEDITIONREADER_H
