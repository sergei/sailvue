#ifndef SAILVUE_CALIBRATION_H
#define SAILVUE_CALIBRATION_H

#include "navcomputer/InstrumentInput.h"

class Calibration {
public:
    Calibration(double twaOffset):m_twaOffset(twaOffset){}
    void calibrate(InstrumentInput &ii);
private:
    double m_twaOffset;
};


#endif //SAILVUE_CALIBRATION_H
