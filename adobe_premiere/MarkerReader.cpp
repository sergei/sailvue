#include <iostream>
#include <ctime>
#include <fstream>
#include <codecvt>
#include <QJsonDocument>

#include "MarkerReader.h"

void MarkerReader::read(const std::filesystem::path &markerFile, const std::list<CameraClipInfo *> &cameraClips) {

    std::cout << "Reading marker file " << markerFile << std::endl;

    // Read marker file line by line
    // opens as text file
    std::ifstream is(markerFile);

    std::string line;
    // The file can be either \n or \r separated, so the first line is read to determine the separator
    std::getline(is, line);
    std::istream *pStream;
    if( line.find('\r') != std::string::npos) {
        // Replace \r with \n
        std::replace(line.begin(), line.end(), '\r', '\n');
        pStream = new std::istringstream(line);
        // Skip header
        std::getline(*pStream, line);
    }else{
        pStream = &is;
    }

    while (std::getline(*pStream, line)) {
        std::stringstream ss(line);
        std::string item;

        auto * marker = new ClipMarker();

        // Clip name
        std::getline(ss, item, ',');
        std::filesystem::path clipName = item;
        std::filesystem::path clipBaseName = clipName.filename().stem().string();
        // Find corresponding clip
        u_int64_t createTimeUtcMs = 0;
        CameraClipInfo *pClipInfo;

        for( auto clip : cameraClips){
            if( clip->getFileName().find(clipBaseName) != std::string::npos){
                std::cout << "  Marker for clip " << clip->getFileName() << std::endl;
                createTimeUtcMs = clip->getClipStartUtcMs();
                pClipInfo = clip;
                break;
            }
        }

        marker->setClipInfo(pClipInfo);

        // In seconds
        std::getline(ss, item, ',');
        marker->setInTimeMilliSecond((u_int64_t)std::round(std::stof(item) * 1000));

        // Out seconds
        std::getline(ss, item, ',');
        marker->setOutTimeMilliSecond((u_int64_t)std::round(std::stof(item) * 1000));

        // Name
        std::getline(ss, item, ',');
        // Trim whitespace
        item.erase(std::remove(item.begin(), item.end(), ' '), item.end());
        marker->setName(item);

        // Type
        std::getline(ss, item, ',');
        marker->setType(std::stoi(item));

        // Uuid
        std::getline(ss, item, ',');
        // Trim whitespace
        item.erase(std::remove(item.begin(), item.end(), ' '), item.end());
        marker->setUuid(item);

        // Overlay name if any
        std::getline(ss, item, ',');
        // Trim whitespace
        item.erase(std::remove(item.begin(), item.end(), ' '), item.end());
        marker->setOverlayName(item);

        marker->setClipStartUtc(createTimeUtcMs + m_timeAdjustmentMs);

        m_markersList.push_back(*marker);
        std::cout << "  Added marker  " << marker->getName() << std::endl;
    }

    // Sort Markers by start UTC time
    m_markersList.sort( [] (const ClipMarker &c1, const ClipMarker &c2) {return c1.getUtcMsIn() < c2.getUtcMsIn();});

}

void MarkerReader::makeChapters(std::list<Chapter *> &chapters, std::vector<InstrumentInput> &instrDataVector) {

    for( const auto& marker: m_markersList){
        // Find index of the clip in

        // Returns the first iterator iter in [first, last) where bool(comp(*iter, value)) is false, or last if no such iter exists.
        auto inIter = std::lower_bound(instrDataVector.begin(), instrDataVector.end(), marker.getUtcMsIn(),
                                        [](const InstrumentInput& ii, u_int64_t utcMs)
                                        { return ii.utc.getUnixTimeMs() <= utcMs;}
                                        );

        auto outIter = std::lower_bound(instrDataVector.begin(), instrDataVector.end(), marker.getUtcMsOut(),
                                        [](const InstrumentInput& ii, u_int64_t utcMs)
                                        { return ii.utc.getUnixTimeMs() < utcMs;}
                                        );

        if ( inIter != instrDataVector.end() && outIter != instrDataVector.end()){
            u_int64_t startIdx = std::distance(instrDataVector.begin(), inIter);
            u_int64_t endIdx = std::distance(instrDataVector.begin(), outIter);

            if( startIdx >= endIdx) {
                std::cerr << "  Invalid timestamp marker " << marker.getName() << " at " << startIdx << " to " << endIdx
                          << std::endl;
                continue;
            }

            Chapter *chapter;
            chapter = new Chapter(QUuid(marker.getUuid()), startIdx, endIdx);
            chapter->setChapterClipFileName(marker.getOverlayName());
            chapter->SetName(marker.getName());
            chapter->setChapterType(ChapterTypes::ChapterType(marker.getType()));
            chapters.push_back(chapter);
        }else{
            std::cerr << "Could not timestamp marker " << marker.getName() << std::endl;
        }
    }

    // Sort chapters by index
    chapters.sort( [] (const Chapter *c1, const Chapter *c2) {return c1->getStartIdx() < c2->getStartIdx();});

}

void MarkerReader::makeMarkers(const std::list<Chapter *>& chapters, std::vector<InstrumentInput> &instrDataVector,
                               std::list<CameraClipInfo *> &clips, const std::filesystem::path &markerFile) const {

    std::filesystem::path markersDir = markerFile.parent_path();
    std::filesystem::create_directories(markersDir);
    std::ofstream outStream (markerFile, std::ios::out);

    outStream << "Clip filename, In, Out, Description, Type, Uuid, Overlay filename" << std::endl;
    for(auto chapter: chapters) {
        uint64_t startUtcMs = instrDataVector[chapter -> getStartIdx()].utc.getUnixTimeMs() - m_timeAdjustmentMs;
        uint64_t endUtcMs = instrDataVector[chapter -> getEndIdx()].utc.getUnixTimeMs()- m_timeAdjustmentMs;

        // Find clip corresponding to the start UTC
        outStream << makeCsvEntry(chapter, startUtcMs, endUtcMs, clips) << std::endl;
    }
}

std::string MarkerReader::makeCsvEntry(const Chapter *pChapter, uint64_t inUtcMs, uint64_t outUtcMs, std::list<CameraClipInfo *> &clips) {
    std::string entry;

    for(const auto &clip : clips){
        if (clip->getClipStartUtcMs() <= inUtcMs && clip->getClipEndUtcMs() >= inUtcMs  ){
            uint64_t clipIn = (inUtcMs - clip->getClipStartUtcMs()) / 1000;
            uint64_t clipOut = (outUtcMs - clip->getClipStartUtcMs()) / 1000;
            std::ostringstream oss;
            oss << clip->getFileName()
            << ", " << clipIn
            << ", " << clipOut
            << ", " << pChapter->getName()
            << ", " << pChapter->getChapterType()
            << ", " << pChapter->getUuid().toStdString()
            << ", " << pChapter->getChapterClipFileName()
            ;
            entry = oss.str();
            break;
        }
    }

    return entry;
}
