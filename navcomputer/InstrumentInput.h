#ifndef SAILVUE_INSTRUMENTINPUT_H
#define SAILVUE_INSTRUMENTINPUT_H


#include "n2k/geo/Angle.h"
#include "n2k/geo/Direction.h"
#include "n2k/geo/Speed.h"
#include "n2k/geo/UtcTime.h"
#include "n2k/geo/GeoLoc.h"

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
    Angle yaw = Angle::INVALID;
    Angle pitch = Angle::INVALID;
    Angle roll = Angle::INVALID;

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
              << ",yaw," << yaw.toString(utc.getUnixTimeMs())
              << ",pitch," << pitch.toString(utc.getUnixTimeMs())
              << ",roll," << roll.toString(utc.getUnixTimeMs())
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
            else if( item == "yaw")
                ii.yaw = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "pitch")
                ii.pitch = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if( item == "roll")
                ii.roll = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
        }

        return ii;
    }

};


#endif //SAILVUE_INSTRUMENTINPUT_H
