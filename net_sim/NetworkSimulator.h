#ifndef SAILVUE_NETWORKSIMULATOR_H
#define SAILVUE_NETWORKSIMULATOR_H

#include <QObject>
#include "navcomputer/InstrumentInput.h"
#include "NMEA2000_UDP.h"
#include "N2kDebugStream.h"

static const int N2K_UDP_TX_PORT = 2023;
static const int N2K_UDP_RX_PORT = 2024;

class NetworkSimulator : public QObject{
Q_OBJECT

public:
explicit NetworkSimulator(std::vector<InstrumentInput> &instrDataVector);


public slots:
    void startSimulator(const QString &addr, uint16_t port);
    void stopSimulator();
    void idxChanged(uint64_t idx);
signals:

private:
    std::vector<InstrumentInput> &m_instrDataVector;
    bool m_bKeepRunning = true;
    void initN2k(tNMEA2000 &n2k);

    NMEA2000_UDP *m_nmea2000 = nullptr;
    N2kDebugStream m_n2kDebugStream;

    void sendEpoch(InstrumentInput &ii);
    long long m_lastEpochMs = 0;
    uint64_t m_lastIdx = -1;

    void transmitSystemTime(uint16_t daysSince1970, double secondsSinceMidnight);
    void transmitFullGpsData(uint16_t daysSince1970, double secondsSinceMidnight, UtcTime &utc, GeoLoc &loc);
    void transmitRapidGpsData(UtcTime &utc, GeoLoc &loc);
    void transmitRapidCogSog(UtcTime &utc, Direction &cog, Speed &sog) const;
    void transmitMagneticHeading(UtcTime &utc, Direction &mag) const;
    void transmitAttitude(UtcTime &utc, Angle &yaw, Angle &pitch, Angle &roll) const;
    void transmitWind(UtcTime &utc, Angle &angle, Speed &speed, bool isApparent) const;
    void transmitBoatSpeed(UtcTime &utc, Speed &sow) const;
    uint8_t uc_SeqId=0;
};


#endif //SAILVUE_NETWORKSIMULATOR_H
