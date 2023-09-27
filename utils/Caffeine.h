#ifndef SAILVUE_CAFFEINE_H
#define SAILVUE_CAFFEINE_H

#include <IOKit/pwr_mgt/IOPMLib.h>

/**
 * Prevents the system from going to sleep.
 * Create an instance of this class to prevent the system from going to sleep.
 */
class Caffeine {
public:
    Caffeine();
    virtual ~Caffeine();
private:
    IOPMAssertionID m_osxIOPMAssertionId=0;
};


#endif //SAILVUE_CAFFEINE_H
