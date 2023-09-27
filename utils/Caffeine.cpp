#include <iostream>

#include "Caffeine.h"

Caffeine::Caffeine() {

    IOPMAssertionID assertionId = m_osxIOPMAssertionId;

    std::cout << "Caffeine: Creating assertion" << std::endl;
    IOReturn r = IOPMAssertionCreateWithName(kIOPMAssertPreventUserIdleSystemSleep, kIOPMAssertionLevelOn,
                                             CFSTR("sailvue_production"), &assertionId);
    if (r == kIOReturnSuccess) {
        m_osxIOPMAssertionId = assertionId;
        std::cout << "Caffeine: Assertion created " <<  m_osxIOPMAssertionId << std::endl;
    }
}

Caffeine::~Caffeine() {
    if (m_osxIOPMAssertionId) {
        std::cout << "Caffeine: Releasing assertion  " <<  m_osxIOPMAssertionId << std::endl;
        IOPMAssertionRelease((IOPMAssertionID)m_osxIOPMAssertionId);
        m_osxIOPMAssertionId = 0U;
    }
}
