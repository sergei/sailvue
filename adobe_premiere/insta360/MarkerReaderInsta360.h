#ifndef SAILVUE_MARKERREADERINSTA360_H
#define SAILVUE_MARKERREADERINSTA360_H


#include <filesystem>
#include <list>
#include "adobe_premiere/ClipMarker.h"
#include "navcomputer/Chapter.h"


class MarkerReaderInsta360 {
public:
    void read(std::filesystem::path &markersDir, std::filesystem::path &insta360Dir);
    void makeChapters(std::vector<InstrumentInput> &instrDataVector, std::list<Chapter> &chapters);
private:
    std::list<ClipMarker> m_markersList;

    static int timeCodeToSec(const std::string &item) ;
};


#endif //SAILVUE_MARKERREADERINSTA360_H
