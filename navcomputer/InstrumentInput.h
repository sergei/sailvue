#ifndef SAILVUE_INSTRUMENTINPUT_H
#define SAILVUE_INSTRUMENTINPUT_H

#include <vector>
#include <list>
#include "n2k/geo/Angle.h"
#include "n2k/geo/Direction.h"
#include "n2k/geo/Speed.h"
#include "n2k/geo/UtcTime.h"
#include "n2k/geo/GeoLoc.h"
#include "n2k/geo/Distance.h"

class InstrumentInput {
public:
    UtcTime utc = UtcTime::INVALID;
    GeoLoc  loc = GeoLoc::INVALID;
    Direction cog = Direction::INVALID;  // Magnetic, not true
    Speed sog = Speed::INVALID;
    Speed aws = Speed::INVALID;
    Angle awa = Angle::INVALID;
    Speed tws = Speed::INVALID;
    Angle twa = Angle::INVALID;
    Direction mag = Direction::INVALID;
    Speed sow = Speed::INVALID;
    Angle rdr = Angle::INVALID;
    Angle cmdRdr = Angle::INVALID;
    Direction hdgToSteer = Direction::INVALID;
    Angle yaw = Angle::INVALID;
    Angle pitch = Angle::INVALID;
    Angle roll = Angle::INVALID;
    Distance log = Distance::INVALID;

    explicit operator std::string() const {
        std::stringstream ss;
        ss << static_cast<std::string>(utc)
              << ",loc," << loc.toString(utc.getUnixTimeMs())
              << ",cog," << cog.toString(utc.getUnixTimeMs())
              << ",sog," << sog.toString(utc.getUnixTimeMs())
              << ",aws," << aws.toString(utc.getUnixTimeMs())
              << ",awa," << awa.toString(utc.getUnixTimeMs())
              << ",tws," << tws.toString(utc.getUnixTimeMs())
              << ",twa," << twa.toString(utc.getUnixTimeMs())
              << ",mag," << mag.toString(utc.getUnixTimeMs())
              << ",sow," << sow.toString(utc.getUnixTimeMs())
              << ",rdr," << rdr.toString(utc.getUnixTimeMs())
              << ",cmdRdr," << cmdRdr.toString(utc.getUnixTimeMs())
              << ",hdgToSteer," << hdgToSteer.toString(utc.getUnixTimeMs())
              << ",yaw," << yaw.toString(utc.getUnixTimeMs())
              << ",pitch," << pitch.toString(utc.getUnixTimeMs())
              << ",roll," << roll.toString(utc.getUnixTimeMs())
              << ",log," << log.toString(utc.getUnixTimeMs())
              ;
        return ss.str();
    }

    static InstrumentInput fromString(const std::string &str) {
        InstrumentInput ii;
        std::string item;
        std::istringstream tokenStream(str);

        std::getline(tokenStream, item, ',');
        uint64_t ulGpsTimeMs = std::stoull(item);
        ii.utc = UtcTime::fromUnixTimeMs(ulGpsTimeMs);

        while (std::getline(tokenStream, item, ',')) {
            std::string value;
            std::getline(tokenStream, value, ',');
            if ( value.empty() )
                continue;

            if( item == "loc") {
                std::stringstream ss(value);
                std::string lat, lon;
                std::getline(ss, lat, ';');
                std::getline(ss, lon );
                ii.loc = GeoLoc::fromDegrees(std::stod(lat), std::stod(lon), ulGpsTimeMs);
            } else if( item == "cog")
                ii.cog = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "sog")
                ii.sog = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if( item == "aws")
                ii.aws = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if( item == "awa")
                ii.awa = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "tws")
                ii.tws = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if( item == "twa")
                ii.twa = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "mag")
                ii.mag = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "sow")
                ii.sow = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if( item == "rdr")
                ii.rdr = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "cmdRdr")
                ii.cmdRdr = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "hdgToSteer")
                ii.hdgToSteer = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "yaw")
                ii.yaw = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "pitch")
                ii.pitch = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "roll")
                ii.roll = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "log")
                ii.log = Distance::fromMeters(std::stod(value), ulGpsTimeMs);
        }

        return ii;
    }

    static InstrumentInput median(const std::vector<InstrumentInput>::iterator &from, const std::vector<InstrumentInput>::iterator &to){
        std::list<GeoLoc> locs;
        std::list<Direction> cogs;
        std::list<Speed> sogs;
        std::list<Speed> awss;
        std::list<Angle> awas;
        std::list<Speed> twss;
        std::list<Angle> twas;
        std::list<Direction> mags;
        std::list<Speed> sows;
        std::list<Angle> rdrs;
        std::list<Angle> cmdRdrs;
        std::list<Direction> hdgsToSteer;
        std::list<Angle> yaws;
        std::list<Angle> pitches;
        std::list<Angle> rolls;

        for( auto it = from; it != to; it++){
            InstrumentInput ii = *it;
            locs.push_back(ii.loc);
            cogs.push_back(ii.cog);
            sogs.push_back(ii.sog);
            awss.push_back(ii.aws);
            awas.push_back(ii.awa);
            twss.push_back(ii.tws);
            twas.push_back(ii.twa);
            mags.push_back(ii.mag);
            sows.push_back(ii.sow);
            rdrs.push_back(ii.rdr);
            cmdRdrs.push_back(ii.cmdRdr);
            hdgsToSteer.push_back(ii.hdgToSteer);
            yaws.push_back(ii.yaw);
            pitches.push_back(ii.pitch);
            rolls.push_back(ii.roll);
        }

        InstrumentInput median;
        median.utc = from->utc;
        median.loc = GeoLoc::median(locs, from->utc.getUnixTimeMs());
        median.cog = Direction::median(cogs, from->utc.getUnixTimeMs());
        median.sog = Speed::median(sogs, from->utc.getUnixTimeMs());
        median.aws = Speed::median(awss, from->utc.getUnixTimeMs());
        median.tws = Speed::median(twss, from->utc.getUnixTimeMs());
        median.awa = Angle::median(awas, from->utc.getUnixTimeMs());
        median.twa = Angle::median(twas, from->utc.getUnixTimeMs());
        median.mag = Direction::median(mags, from->utc.getUnixTimeMs());
        median.sow = Speed::median(sows, from->utc.getUnixTimeMs());
        median.rdr = Angle::median(rdrs, from->utc.getUnixTimeMs());
        median.cmdRdr = Angle::median(cmdRdrs, from->utc.getUnixTimeMs());
        median.hdgToSteer = Direction::median(hdgsToSteer, from->utc.getUnixTimeMs());
        median.yaw = Angle::median(yaws, from->utc.getUnixTimeMs());
        median.pitch = Angle::median(pitches, from->utc.getUnixTimeMs());
        median.roll = Angle::median(rolls, from->utc.getUnixTimeMs());
        median.log = from->log;

        return median;
    }

};


#endif //SAILVUE_INSTRUMENTINPUT_H
