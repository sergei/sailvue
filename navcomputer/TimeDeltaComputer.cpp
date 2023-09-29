#include <iostream>
#include "TimeDeltaComputer.h"

TimeDeltaComputer::TimeDeltaComputer(Polars &polars, std::vector<InstrumentInput> &rInstrDataVector, u_int64_t startIdx,
                                     u_int64_t endIdx)
:m_polars(polars), m_rInstrDataVector(rInstrDataVector)
{
    // Determine the kind of leg 
    double minTwa = 200;
    double maxTwa = -200;
    for(uint64_t i = startIdx; i < endIdx; i++){
        if( m_rInstrDataVector[i].twa.isValid(m_rInstrDataVector[i].utc.getUnixTimeMs())){
            double twa = m_rInstrDataVector[i].twa.getDegrees();
            if ( twa < minTwa)
                minTwa = twa;
            if ( twa > maxTwa)
                maxTwa = twa;
        }
    }

    if ( maxTwa < -190 ){
        std::cout << "No valid wind to compute loss" << std::endl;
    }
    
    bool downWind = abs(minTwa) > 100;
    bool upWind = abs(maxTwa) < 80;
    if ( upWind ){
        m_legType = LEG_TYPE_UPWIND;
    } else if ( downWind ){
        m_legType = LEG_TYPE_DOWNWIND;
    } else {
        m_legType = LEG_TYPE_REACH;
    }

}

int64_t TimeDeltaComputer::getDeltaMs(u_int64_t idx) {

    auto instr = m_rInstrDataVector[idx];
    if(m_prevTimeStamp == 0 ){
        m_prevTimeStamp = instr.utc.getUnixTimeMs();
        return int(m_accumulateTimeToTarget * 1000);
    }
    
    double deltaSec = double(instr.utc.getUnixTimeMs() - m_prevTimeStamp) * 0.001;
    m_prevTimeStamp = instr.utc.getUnixTimeMs();
    double targetSpeed;
    double ourSpeed;
    if ( m_legType == LEG_TYPE_UNKNOWN ) {
        return 0;
    } else if ( m_legType == LEG_TYPE_REACH ){ // Compute delta time using the boat speed vs target boat speed
        targetSpeed = m_polars.getSpeed(instr.twa.getDegrees(), instr.tws.getKnots());
        ourSpeed = instr.sow.getKnots();
    }else {  // Compute delta time using the boat VMG vs target VMG
        std::pair<double, double> targets =  m_polars.getTargets(instr.tws.getKnots(), instr.twa.getDegrees() < 90);
        targetSpeed = abs(targets.second);
        ourSpeed = abs(instr.sow.getKnots() * cos(instr.twa.getRadians()));
    }
    
    double distGainedOnTarget = (ourSpeed - targetSpeed) * deltaSec;
    // Now compute the time to target
    double timeToTarget = 0;
    if ( targetSpeed > 0.01 ){
        timeToTarget = distGainedOnTarget / targetSpeed;
    }
    m_accumulateTimeToTarget += timeToTarget;

    return int(m_accumulateTimeToTarget * 1000);
}
