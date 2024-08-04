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

static const char *const ITEM_UTC = "utc_unix_ms";
static const char *const ITEM_LOC = "loc";
static const char *const ITEMS_LAT_LON = "lat_deg,lon_deg";
static const char *const ITEM_COG = "cog_deg_mag";
static const char *const ITEM_SOG = "sog_kts";
static const char *const ITEM_AWS = "aws_kts";
static const char *const ITEM_AWA = "awa_deg";
static const char *const ITEM_TWS = "tws_kts";
static const char *const ITEM_TWA = "twa_deg";
static const char *const ITEM_MAG = "hdg_deg_mag";
static const char *const ITEM_SOW = "sow_kts";
static const char *const ITEM_RDR = "rdr_deg";
static const char *const ITEM_CMD_RDR = "cmd_rdr_deg";
static const char *const ITEM_HDG_TO_STEER = "hdg_to_steer_deg";
static const char *const ITEM_YAW = "yaw_deg";
static const char *const ITEM_PITCH = "pitch_deg";
static const char *const ITEM_ROLL = "roll_deg";
static const char *const ITEM_LOG = "log_m";
static const char *const ITEM_MAG_VAR = "mag_var_deg";
static const char *const ITEM_DEPTH = "depth_m";
static const char *const ITEM_RACE_TIMER = "race_timer_ms";
static const char *const ITEM_TARGET_SPEED = "target_speed_kts";
static const char *const ITEM_POLAR_SPEED = "polar_speed_kts";
static const char *const ITEM_VMG = "vmg_kts";
static const char *const ITEM_LEEWAY = "leeway_deg";
static const char *const ITEM_CURRENT_DRIFT = "current_drift_kts";
static const char *const ITEM_CURRENT_SET = "current_set_deg";
static const char *const ITEM_DIST_TO_START = "dist_to_start_m";
static const char *const ITEM_PILOT_TWA = "pilot_twa_deg";

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
    Angle magVar = Angle::INVALID;
    Distance depth = Distance::INVALID;

    /// The fields below coming form B&G proprietary PGN
    // Race timer
    int64_t raceTimer = INT64_MIN;
    // Target boat speed
    Speed targetSpeed = Speed::INVALID;
    // Polar speed
    Speed polarSpeed = Speed::INVALID;
    // VMG to wind
    Speed vmg = Speed::INVALID;
    // Leeway angle
    Angle leeway = Angle::INVALID;
    // Current drift
    Speed currentDrift = Speed::INVALID;
    // Current set
    Direction currentSet = Direction::INVALID;
    // Distance to start line
    Distance distToStart = Distance::INVALID;
    // Pilot Target Wind Angle
    Angle pilotTwa = Angle::INVALID;

    [[nodiscard]] std::string toCsv(bool isHeader) const {
        std::stringstream ss;
        std::istringstream tokenStream(std::string(*this));

        std::string item;
        int count = 0;

        if ( isHeader ) {
            ss << ITEM_UTC << ",";
        }

        bool isLocValue = false;
        while (std::getline(tokenStream, item, ',')) {
            bool isName  = (count % 2) != 0;
            bool isValue = (count % 2) == 0;
            bool doPrint = (isName && isHeader) || (isValue && !isHeader);
            if (item == ITEM_LOC)
                isLocValue = true;
            if ( doPrint ){
                // Special treatment of location object to make uniform CSV
                // Replace ; with ,
                if (  isLocValue ){
                    isLocValue = false;
                    // Replace ; with '
                    std::replace( item.begin(), item.end(), ';', ',');
                }
                // Replace loc with lat,lon
                if (item == ITEM_LOC){
                    ss << ITEMS_LAT_LON;
                }else{
                    ss << item;
                }

                ss << ",";
            }
           count ++;
        }

        return ss.str();
    }

    explicit operator std::string() const {
        std::stringstream ss;
        ss << static_cast<std::string>(utc)
              <<  "," << ITEM_LOC << "," << loc.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_COG << "," << cog.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_SOG << "," << sog.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_AWS << "," << aws.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_AWA << "," << awa.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_TWS << "," << tws.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_TWA << "," << twa.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_MAG << "," << mag.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_SOW << "," << sow.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_RDR << "," << rdr.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_CMD_RDR << "," << cmdRdr.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_HDG_TO_STEER << "," << hdgToSteer.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_YAW << "," << yaw.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_PITCH << "," << pitch.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_ROLL << "," << roll.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_LOG << "," << log.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_MAG_VAR << "," << magVar.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_DEPTH << "," << depth.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_RACE_TIMER << "," << std::to_string(raceTimer)
              <<  "," << ITEM_TARGET_SPEED << "," << targetSpeed.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_POLAR_SPEED << "," << polarSpeed.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_VMG << "," << vmg.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_LEEWAY << "," << leeway.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_CURRENT_DRIFT << "," << currentDrift.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_CURRENT_SET << "," << currentSet.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_DIST_TO_START << "," << distToStart.toString(utc.getUnixTimeMs())
              <<  "," << ITEM_PILOT_TWA << "," << pilotTwa.toString(utc.getUnixTimeMs())
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

            if(item == ITEM_LOC) {
                std::stringstream ss(value);
                std::string lat, lon;
                std::getline(ss, lat, ';');
                std::getline(ss, lon );
                ii.loc = GeoLoc::fromDegrees(std::stod(lat), std::stod(lon), ulGpsTimeMs);
            } else if(item == ITEM_COG)
                ii.cog = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_SOG)
                ii.sog = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_AWS)
                ii.aws = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_AWA)
                ii.awa = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_TWS)
                ii.tws = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_TWA)
                ii.twa = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_MAG)
                ii.mag = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_SOW)
                ii.sow = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_RDR)
                ii.rdr = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_CMD_RDR)
                ii.cmdRdr = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_HDG_TO_STEER)
                ii.hdgToSteer = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_YAW)
                ii.yaw = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_PITCH)
                ii.pitch = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_ROLL)
                ii.roll = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_LOG)
                ii.log = Distance::fromMeters(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_MAG_VAR)
                ii.magVar = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_DEPTH)
                ii.depth = Distance::fromMillimeters(std::stod(value) * 1000, ulGpsTimeMs);
            else if(item == ITEM_RACE_TIMER)
                ii.raceTimer = std::stoll(value);
            else if(item == ITEM_TARGET_SPEED)
                ii.targetSpeed = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_POLAR_SPEED)
                ii.polarSpeed = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_VMG)
                ii.vmg = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_LEEWAY)
                ii.leeway = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_CURRENT_DRIFT)
                ii.currentDrift = Speed::fromKnots(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_CURRENT_SET)
                ii.currentSet = Direction::fromDegrees(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_DIST_TO_START)
                ii.distToStart = Distance::fromMeters(std::stod(value), ulGpsTimeMs);
            else if(item == ITEM_PILOT_TWA)
                ii.pilotTwa = Angle::fromDegrees(std::stod(value), ulGpsTimeMs);
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
        std::list<Speed> targetSpeeds;
        std::list<Speed> polarSpeeds;
        std::list<Speed> vmgs;
        std::list<Angle> leeways;
        std::list<Speed> currentDrifts;
        std::list<Direction> currentSets;
        std::list<Angle> pilotTwas;

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
            targetSpeeds.push_back(ii.targetSpeed);
            polarSpeeds.push_back(ii.polarSpeed);
            vmgs.push_back(ii.vmg);
            leeways.push_back(ii.leeway);
            currentDrifts.push_back(ii.currentDrift);
            currentSets.push_back(ii.currentSet);
            pilotTwas.push_back(ii.pilotTwa);
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
        median.magVar = from->magVar;
        median.depth = from->depth;
        median.raceTimer = from->raceTimer;
        median.targetSpeed = Speed::median(targetSpeeds, from->utc.getUnixTimeMs());
        median.polarSpeed = Speed::median(polarSpeeds, from->utc.getUnixTimeMs());
        median.vmg = Speed::median(vmgs, from->utc.getUnixTimeMs());
        median.leeway = Angle::median(leeways, from->utc.getUnixTimeMs());
        median.currentDrift = Speed::median(currentDrifts, from->utc.getUnixTimeMs());
        median.currentSet = Direction::median(currentSets, from->utc.getUnixTimeMs());
        median.distToStart = from->distToStart;
        median.pilotTwa = Angle::median(pilotTwas, from->utc.getUnixTimeMs());

        return median;
    }

};


#endif //SAILVUE_INSTRUMENTINPUT_H
