#ifndef SAILVUE_CAMERABASE_H
#define SAILVUE_CAMERABASE_H

#include <string>
#include "../InstrDataReader.h"
#include "navcomputer/IProgressListener.h"

class CameraClipInfo {
public:
    CameraClipInfo(std::string clipFileName, uint64_t ulClipStartUtcMs, uint64_t ulClipEndUtcMs,
                     std::list<InstrumentInput> *pInstrDataList, int w, int h)
                     : m_clipFileName(std::move(clipFileName)),
                       m_ulClipStartUtcMs(ulClipStartUtcMs), m_ulClipEndUtcMs(ulClipEndUtcMs), m_pInstrDataList(pInstrDataList),
                       m_width(w), m_height(h){

    }
    [[nodiscard]] uint64_t getClipStartUtcMs() const { return m_ulClipStartUtcMs; }
    [[nodiscard]] uint64_t getClipEndUtcMs() const { return m_ulClipEndUtcMs; }
    [[nodiscard]] const std::string &getFileName() const {return m_clipFileName;}
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    std::list<InstrumentInput> *getInstrData() {return m_pInstrDataList;};

protected:
    const std::string m_clipFileName;
    const int m_width;
    const int m_height;
    uint64_t m_ulClipStartUtcMs = 0;
    uint64_t m_ulClipEndUtcMs = 0;
    std::list<InstrumentInput> *m_pInstrDataList = nullptr;
};

class CameraBase {
public:
    CameraBase(const std::string& cameraName, const std::string& cameraClipExtension,
               InstrDataReader& rInstrDataReader, IProgressListener& rProgressListener);

    void processClipsDir(const std::string &stClipsDir, const std::string &stWorkDir);
    std::list<CameraClipInfo *> &getClipList() {return m_clipInfoList; };

protected:
    static void storeCacheFile(const std::filesystem::path &cacheFile, const std::list<InstrumentInput>& instrDataList);
    void ReadCacheAndSummary(const std::filesystem::path &pathCacheFile, const std::filesystem::path &pathSummaryFile,
                             std::list<InstrumentInput> &instrData, int &width, int &height);
    void processClipFile(const std::string &mp4FileName, const std::filesystem::path &cacheDir,
                                     const std::filesystem::path &summaryDir);
    // Returns width, height
    virtual std::tuple<int , int >  readClipFile(const std::string &clipFileName, std::list<InstrumentInput> &listInputs) = 0;


protected:
    InstrDataReader& m_rInstrDataReader;
    IProgressListener& m_rProgressListener;
    std::list<CameraClipInfo *> m_clipInfoList;
    uint64_t m_ulClipStartUtcMs = 0;
    uint64_t m_ulClipEndUtcMs = 0;
    std::string m_cameraName;
    std::string m_cameraClipExtension;
};


#endif //SAILVUE_CAMERABASE_H
