#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <codecvt>
#include "MarkerReaderInsta360.h"

void MarkerReaderInsta360::read(std::filesystem::path &markersDir, std::filesystem::path &insta360Dir) {

    auto markerFiles = std::filesystem::recursive_directory_iterator(markersDir);

    for( const auto& markerFile : markerFiles ) {
        if (markerFile.path().extension() == ".csv") {
            // Find corresponding .insv file
            std::filesystem::path insvBaseName = markerFile.path().filename().stem();
            std::filesystem::path insvPath = insta360Dir / insvBaseName;

            std::cout << insvPath << std::endl;

            const char *fname = insvPath.c_str();

            // Get file creation time
            struct stat t_stat{};
            stat(fname, &t_stat);
            u_int64_t createTimeUtcMs = t_stat.st_birthtimespec.tv_sec * 1000 +
                    t_stat.st_birthtimespec.tv_nsec / 1000 / 1000 ;

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

                marker->setClipStartUtc(createTimeUtcMs);
                m_markersList.push_back(*marker);
          }
        }
    }
}

int MarkerReaderInsta360::timeCodeToSec(const std::string &item) {
    return stoi(item.substr(0, 2)) * 3600
               + stoi(item.substr(3, 2)) * 60
               + stoi(item.substr(6, 2));
}

void MarkerReaderInsta360::makeChapters(std::vector<InstrumentInput> &instrDataVector, std::list<Chapter> &chapters) {

    for( auto marker: m_markersList){
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
            u_int64_t startIdx = inIter - instrDataVector.begin();
            u_int64_t endIdx = outIter - instrDataVector.begin();

            auto chapter = new Chapter(startIdx, endIdx);
            chapter->SetName(marker.getName());
            chapters.push_back(*chapter);
        }else{
            std::cerr << "Could not timestamp marker " << marker.getName() << std::endl;
        }
    }

    // Sort chapters by index
    chapters.sort( [] (const Chapter &c1, const Chapter &c2) {return c1.getStartIdx() < c2.getStartIdx();});

}
