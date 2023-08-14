#ifndef SAILVUE_GOPRO_H
#define SAILVUE_GOPRO_H


#include <string>
#include <filesystem>
#include "../InstrDataReader.h"
#include "navcomputer/IProgressListener.h"

class GoProClipInfo {
public:
    GoProClipInfo(std::string mp4FileName, uint64_t ulClipStartUtcMs, uint64_t ulClipEndUtcMs,
                  std::list<InstrumentInput> *pInstrDataList, int w, int h);
    [[nodiscard]] uint64_t getClipStartUtcMs() const { return m_ulClipStartUtcMs; }
    [[nodiscard]] uint64_t getClipEndUtcMs() const { return m_ulClipEndUtcMs; }
    [[nodiscard]] const std::string &getFileName() const {return m_mp4FileName;}
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }

private:
    const std::string m_mp4FileName;

private:
    uint64_t m_ulClipStartUtcMs = 0;
    uint64_t m_ulClipEndUtcMs = 0;
    const std::list<InstrumentInput> *m_pInstrDataList;
    const int m_width;
public:
private:
    const int m_height;
public:
    const std::list<InstrumentInput> *getInstrData() const;
};

class GoPro {
public:
    GoPro(const std::string& stYGoProDir, const std::string& stCacheDir,
          InstrDataReader& rInstrDataReader, IProgressListener& rProgressListener);
private:
    void processGoProDir(const std::string &stGoProDir, const std::string &stWorkDir);
    void processMp4File(const std::string &mp4FileName, const std::filesystem::path &cacheDir,
                        const std::filesystem::path &summaryDir);
private:
    static time_t strToTime(const std::string& str);
    void readMp4File(const std::string &mp4FileName, std::list<InstrumentInput> &listInputs);
    void storeCacheFile(const std::filesystem::path &cacheFile, const std::list<InstrumentInput>& instrDataList);
    void ReadCacheAndSummary(const std::filesystem::path &pathCacheFile, const std::filesystem::path &pathSummaryFile,
                             std::list<InstrumentInput> &instrData, int &width, int &height);
private:
    InstrDataReader& m_rInstrDataReader;
    IProgressListener& m_rProgressListener;
    std::list<GoProClipInfo> m_GoProClipInfoList;
public:
    const std::list<GoProClipInfo> &getGoProClipList() const;

private:
    uint64_t m_ulClipStartUtcMs = 0;
    uint64_t m_ulClipEndUtcMs = 0;

};


#endif //SAILVUE_GOPRO_H
