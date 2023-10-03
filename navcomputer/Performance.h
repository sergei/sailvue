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
    double distLostToTarget=0;
    double timeLostToTarget=0;

    std::string toString(UtcTime utc) const {
        std::stringstream ss;
        ss << "r_idx," << raceIdx
           << ",l_idx," << legIdx
           << ",t_sow," << targetSpeed.toString(utc.getUnixTimeMs())
           << ",t_twa," << targetTwa.toString(utc.getUnixTimeMs())
           << ",t_vmg," << targetVmg.toString(utc.getUnixTimeMs())
           << ",vmg," << ourVmg.toString(utc.getUnixTimeMs())
           << ",fetch," << (isValid ? std::to_string(isFetching) : "")
           << ",dlt," <<  (isValid ? std::to_string(distLostToTarget) : "" )
           << ",tlt," <<  (isValid ? std::to_string(timeLostToTarget) : "" )
                ;
        return ss.str();
    }
};


#endif //SAILVUE_PERFORMANCE_H
