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
    Direction cog = Direction::INVALID;
    Speed sog = Speed::INVALID;
    Speed aws = Speed::INVALID;
    Angle awa = Angle::INVALID;
    Speed tws = Speed::INVALID;
    Angle twa = Angle::INVALID;
    Direction mag = Direction::INVALID;
    Speed sow = Speed::INVALID;
    Angle rdr = Angle::INVALID;
    Angle yaw = Angle::INVALID;
    Angle pitch = Angle::INVALID;
    Angle roll = Angle::INVALID;

    explicit operator std::string() const {
        std::stringstream ss;
        ss << static_cast<std::string>(utc)
              << ",loc," << static_cast<std::string>(loc)
              << ",cog," << static_cast<std::string>(cog)
              << ",sog," << static_cast<std::string>(sog)
              << ",aws," << static_cast<std::string>(aws)
              << ",awa," << static_cast<std::string>(awa)
              << ",tws," << static_cast<std::string>(tws)
              << ",twa," << static_cast<std::string>(twa)
              << ",mag," << static_cast<std::string>(mag)
              << ",sow," << static_cast<std::string>(sow)
              << ",rdr," << static_cast<std::string>(rdr)
              << ",yaw," << static_cast<std::string>(yaw)
              << ",pitch," << static_cast<std::string>(pitch)
              << ",roll," << static_cast<std::string>(roll)
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
                ii.loc = GeoLoc::fromDegrees(std::stod(lat), std::stod(lon));
            } else if( item == "cog")
                ii.cog = Direction::fromDegrees(std::stod(value));
            else if( item == "sog")
                ii.sog = Speed::fromKnots(std::stod(value));
            else if( item == "aws")
                ii.aws = Speed::fromKnots(std::stod(value));
            else if( item == "awa")
                ii.awa = Angle::fromDegrees(std::stod(value));
            else if( item == "tws")
                ii.tws = Speed::fromKnots(std::stod(value));
            else if( item == "twa")
                ii.twa = Angle::fromDegrees(std::stod(value));
            else if( item == "mag")
                ii.mag = Direction::fromDegrees(std::stod(value));
            else if( item == "sow")
                ii.sow = Speed::fromKnots(std::stod(value));
            else if( item == "rdr")
                ii.rdr = Angle::fromDegrees(std::stod(value));
            else if( item == "yaw")
                ii.yaw = Angle::fromDegrees(std::stod(value));
            else if( item == "pitch")
                ii.pitch = Angle::fromDegrees(std::stod(value));
            else if( item == "roll")
                ii.roll = Angle::fromDegrees(std::stod(value));
        }

        return ii;
    }

};


#endif //SAILVUE_INSTRUMENTINPUT_H
