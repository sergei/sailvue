#ifndef SAILVUE_INSTRUMENTINPUT_H
#define SAILVUE_INSTRUMENTINPUT_H


#include "geo/Angle.h"
#include "geo/Direction.h"
#include "geo/Speed.h"
#include "geo/UtcTime.h"
#include "geo/GeoLoc.h"

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
};


#endif //SAILVUE_INSTRUMENTINPUT_H
