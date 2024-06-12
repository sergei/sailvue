#include <iostream>
#include <ctime>
#include <fstream>
#include <codecvt>
#include <QJsonDocument>

#include "MarkerReader.h"

void MarkerReader::read(const std::filesystem::path &markersDir, const std::list<CameraClipInfo *> &cameraClips) {

    auto markerFiles = std::filesystem::recursive_directory_iterator(markersDir);

    for( const auto& markerFile : markerFiles ) {
        std::cout << "Reading marker file " << markerFile.path() << std::endl;

        if (markerFile.path().extension() == ".csv") {
            std::filesystem::path clipBaseName = markerFile.path().filename().stem().string();

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

            if ( createTimeUtcMs == 0 ){
                std::cerr << "Failed to find clip for marker file " << markerFile.path() << std::endl;
                continue;
            }

            // Read marker file line by line
            // Marker file is encoded as UTF-16LE
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
            std::wifstream is16(markerFile);
            is16.imbue(std::locale(is16.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>()));

            std::wstring wline;
            // Skip header
            std::getline(is16, wline);

            while (std::getline(is16, wline)) {
                std::string line = converter.to_bytes(wline);
                std::stringstream ss(line);
                std::string item;

                auto * marker = new ClipMarker();

                // Marker Name
                std::getline(ss, item, '\t');
                marker->setName(item);

                // Description
                std::getline(ss, item, '\t');

                // In
                std::getline(ss, item, '\t');
                int inSec = timeCodeToSec(item);
                marker->setInTimeSec(inSec);

                // Out
                std::getline(ss, item, '\t');
                int outSec = timeCodeToSec(item);
                marker->setOutTimeSec(outSec);

                marker->setClipStartUtc(createTimeUtcMs + m_timeAdjustmentMs);

                marker->setClipInfo(pClipInfo);

                m_markersList.push_back(*marker);
                std::cout << "  Added marker  " << marker->getName() << std::endl;
            }
        }
    }

    // Sort Markers by start UTC time
    m_markersList.sort( [] (const ClipMarker &c1, const ClipMarker &c2) {return c1.getUtcMsIn() < c2.getUtcMsIn();});

}


int MarkerReader::timeCodeToSec(const std::string &item) {
    return stoi(item.substr(0, 2)) * 3600
               + stoi(item.substr(3, 2)) * 60
               + stoi(item.substr(6, 2));
}

void MarkerReader::makeChapters(std::list<Chapter *> &chapters, std::vector<InstrumentInput> &instrDataVector) {

    for( const auto& marker: m_markersList){
        // Find index of the clip in
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

            auto chapter = new Chapter(startIdx, endIdx);
            chapter->SetName(marker.getName());
            chapters.push_back(chapter);
        }else{
            std::cerr << "Could not timestamp marker " << marker.getName() << std::endl;
        }
    }

    // Sort chapters by index
    chapters.sort( [] (const Chapter *c1, const Chapter *c2) {return c1->getStartIdx() < c2->getStartIdx();});

}

void MarkerReader::makeMarkers(std::list<Chapter *> chapters, std::vector<InstrumentInput> &instrDataVector,
                               std::list<CameraClipInfo *> &clips, const std::filesystem::path &markerFile) {

    std::filesystem::path markersDir = markerFile.parent_path();
    std::filesystem::create_directories(markersDir);
    std::ofstream outStream (markerFile, std::ios::out);

    outStream << "Clip file, In, Name" << std::endl;
    for(auto chapter: chapters) {
        uint64_t startUtcMs = instrDataVector[chapter -> getStartIdx()].utc.getUnixTimeMs() - m_timeAdjustmentMs;
        uint64_t endUtcMs = instrDataVector[chapter -> getEndIdx()].utc.getUnixTimeMs();

        // Find clip corresponding to the start UTC
        outStream << makeCsvEntry(chapter->getName(), startUtcMs, clips) << std::endl;
    }
}

std::string MarkerReader::makeCsvEntry(std::string chapterName, uint64_t utcMs, std::list<CameraClipInfo *> &clips) {
    std::string entry;

    for(const auto &clip : clips){
        if ( clip->getClipStartUtcMs() <= utcMs && clip->getClipEndUtcMs() >= utcMs  ){
            int clipIn = (utcMs - clip->getClipStartUtcMs()) / 1000;
            int hours = clipIn / 3600;
            int minutes = (clipIn / 60) % 60;
            int seconds = clipIn  % 60;
            std::ostringstream oss;
            oss << std::filesystem::path(clip->getFileName()).filename()
            << ", "<< std::setw(2) << std::setfill('0') << hours << ":" << minutes << ":" << seconds << ":00"
            << ", " << chapterName
            ;
            entry = oss.str();
            break;
        }
    }

    return entry;
}
