#include "Calibration.h"

void Calibration::calibrate(InstrumentInput &ii) {
    ii.twa = Angle::fromDegrees(ii.twa.getDegrees() + m_twaOffset, ii.utc.getUnixTimeMs());
}
