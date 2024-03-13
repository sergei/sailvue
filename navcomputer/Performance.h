#ifndef SAILVUE_PERFORMANCE_H
#define SAILVUE_PERFORMANCE_H


#include "n2k/geo/Speed.h"
#include "n2k/geo/Angle.h"
#include "n2k/geo/UtcTime.h"

class Performance {
public:
    int raceIdx= -1;
    int legIdx= -1;
    bool isValid = false;
    Speed targetSpeed;
    Angle targetTwa;
    Speed targetVmg;
    Speed ourVmg;
    bool isFetching = false;
    double legDistLostToTargetMeters=0;
    double legTimeLostToTargetSec=0;
    double raceDistLostToTargetMeters=0;
    double raceTimeLostToTargetSec=0;

    std::string toString(UtcTime &utc) const {
        std::stringstream ss;
        ss << "r_idx," << raceIdx
           << ",l_idx," << legIdx
           << ",t_sow_kts," << targetSpeed.toString(utc.getUnixTimeMs())
           << ",t_twa_deg," << targetTwa.toString(utc.getUnixTimeMs())
           << ",t_vmg_kts," << targetVmg.toString(utc.getUnixTimeMs())
           << ",vmg_kts," << ourVmg.toString(utc.getUnixTimeMs())
           << ",is_fetch," << (isValid ? std::to_string(isFetching) : "")
           << ",ldlt_meters," <<  (isValid ? std::to_string(int(legDistLostToTargetMeters)) : "" )
           << ",ltlt_sec," <<  (isValid ? std::to_string(int(legTimeLostToTargetSec)) : "" )
           << ",rdlt_meters," <<  (isValid ? std::to_string(int(raceDistLostToTargetMeters)) : "" )
           << ",rtlt_sec," <<  (isValid ? std::to_string(int(raceTimeLostToTargetSec)) : "" )
                ;
        return ss.str();
    }

    [[nodiscard]] std::string toCsv(bool isHeader, UtcTime &utc) const {
        std::stringstream ss;
        std::istringstream tokenStream(toString(utc));

        std::string item;
        int count = 0;

        while (std::getline(tokenStream, item, ',')) {
            bool isName  = (count % 2) == 0;
            bool isValue = (count % 2) != 0;
            bool doPrint = (isName && isHeader) || (isValue && !isHeader);
            if ( doPrint ){
                ss << item;
                ss << ",";
            }
            count ++;
        }

        return ss.str();
    }


};


#endif //SAILVUE_PERFORMANCE_H
