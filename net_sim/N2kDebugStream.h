#ifndef SAILVUE_N2KDEBUGSTREAM_H
#define SAILVUE_N2KDEBUGSTREAM_H


#include "n2k/NMEA2000/src/N2kStream.h"

class N2kDebugStream : public N2kStream {
public:
    int read() override;
    int peek() override;
    size_t write(const uint8_t *data, size_t size) override;
};


#endif //SAILVUE_N2KDEBUGSTREAM_H
