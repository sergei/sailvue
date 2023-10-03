#include <iostream>
#include "TimeDeltaComputer.h"

TimeDeltaComputer::TimeDeltaComputer(Polars &polars, std::vector<InstrumentInput> &rInstrDataVector)
:m_polars(polars), m_rInstrDataVector(rInstrDataVector)
{

}

void TimeDeltaComputer::startLeg() {
    m_accumulateDistToTargetMeters = 0;
    m_accumulateTimeToTarget = 0;
    m_prevTimeStamp = 0;
}

void TimeDeltaComputer::updatePerformance(uint64_t idx, Performance &performance, bool isFetch) {

    auto instr = m_rInstrDataVector[idx];

    double targetSpeed;
    double ourSpeed;
    if ( isFetch ){ // Compute delta time using the boat speed vs target boat speed
        targetSpeed = m_polars.getSpeed(instr.twa.getDegrees(), instr.tws.getKnots());
        performance.targetSpeed = Speed::fromKnots(targetSpeed, instr.utc.getUnixTimeMs());
        performance.targetTwa = Angle::INVALID;
        performance.targetVmg = Speed::INVALID;
        ourSpeed = instr.sow.getKnots();
        performance.ourVmg = Speed::INVALID;
    }else {  // Compute delta time using the boat VMG vs target VMG
        std::pair<double, double> targets =  m_polars.getTargets(instr.tws.getKnots(), instr.twa.getDegrees() < 90);
        targetSpeed = abs(targets.second);
        performance.targetSpeed = Speed::INVALID;
        performance.targetTwa = Angle::fromDegrees(targets.first, instr.utc.getUnixTimeMs());
        performance.targetVmg = Speed::fromKnots(targetSpeed, instr.utc.getUnixTimeMs());
        ourSpeed = abs(instr.sow.getKnots() * cos(instr.twa.getRadians()));
        performance.ourVmg = Speed::fromKnots(ourSpeed, instr.utc.getUnixTimeMs());
    }
    performance.isValid = true;
    performance.isFetching = isFetch;

    if(m_prevTimeStamp == 0 ){
        m_prevTimeStamp = instr.utc.getUnixTimeMs();
        performance.distLostToTarget=0;
        performance.timeLostToTarget=0;
        return;
    }

    double deltaSec = double(instr.utc.getUnixTimeMs() - m_prevTimeStamp) * 0.001;

    m_prevTimeStamp = instr.utc.getUnixTimeMs();

    auto deltaSpeedMeterPerSec = (ourSpeed - targetSpeed) * 1852. / 3600. ;
    auto targetSpeedMeterPerSec = targetSpeed * 1852. / 3600.;

    double distGainedOnTargetMeters = deltaSpeedMeterPerSec * deltaSec;
    // Now compute the time to target
    double timeToTarget = 0;
    if ( targetSpeed > 0.01 ){  // If we don't expect to move, don't increase time to target
        timeToTarget = distGainedOnTargetMeters / targetSpeedMeterPerSec;
    }
    m_accumulateTimeToTarget += timeToTarget;
    m_accumulateDistToTargetMeters += distGainedOnTargetMeters;

    performance.distLostToTarget = m_accumulateDistToTargetMeters;
    performance.timeLostToTarget = m_accumulateTimeToTarget;
}




