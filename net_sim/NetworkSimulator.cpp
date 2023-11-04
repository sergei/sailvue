#include <iostream>
#include <QCoreApplication>
#include <QThread>
#include "NetworkSimulator.h"
#include "n2k/NMEA2000/src/N2kTypes.h"
#include "n2k/NMEA2000/src/N2kMessages.h"

static const unsigned long TX_PGNS[] PROGMEM={
        130306,   // Wind Speed
        128259L,  // Boat speed
        127250,   // Vessel Heading
        127257,   // Attitude
        126992,   // System time
        129025,   // Position, Rapid Update
        129026,   // COG & SOG, Rapid Update
        127258,   // Magnetic Variation
        0};
static const unsigned long RX_PGNS[] PROGMEM={
        0};

NetworkSimulator::NetworkSimulator(std::vector<InstrumentInput> &instrDataVector)
        :m_instrDataVector(instrDataVector)
{

}

void NetworkSimulator::startSimulator(const QString &addr, uint16_t port) {
    std::cout << "startSimulator " << addr.toStdString() << " " << port << std::endl;

    m_nmea2000 = new NMEA2000_UDP(N2K_UDP_TX_PORT, N2K_UDP_RX_PORT);
    initN2k(*m_nmea2000);
    while (m_bKeepRunning){
        QCoreApplication::processEvents();
        m_nmea2000->ParseMessages();
        QThread::msleep(1);
    }

    std::cout << "Simulator finished"  << std::endl;
}

void NetworkSimulator::sendEpoch(InstrumentInput &ii) {
    long long now = duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    if ( now < m_lastEpochMs )  // User scrolled backward 
        m_lastEpochMs = 0;

    uint16_t DaysSince1970 = now / 1000 / 3600 / 24;
    double SecondsSinceMidnight = double(now) / 1000.0 - DaysSince1970 * 3600 * 24;

    transmitSystemTime(DaysSince1970, SecondsSinceMidnight);
    if ( now - m_lastEpochMs >= 1000) {  // Send full GPS update
        transmitFullGpsData(DaysSince1970, SecondsSinceMidnight, ii.utc, ii.loc);    
    }else { // Send rapid GPS update
        transmitRapidGpsData(ii.utc, ii.loc);
    }
    transmitRapidCogSog(ii.utc, ii.cog, ii.sog);
    transmitMagneticHeading(ii.utc, ii.mag);
    transmitAttitude(ii.utc, ii.yaw, ii.pitch, ii.roll);
    transmitWind(ii.utc, ii.awa, ii.aws, true);
    transmitWind(ii.utc, ii.twa, ii.tws, false);
    transmitBoatSpeed(ii.utc, ii.sow);

    m_ucSeqId ++;
    if (m_ucSeqId == 253 )
        m_ucSeqId = 0;

    m_lastEpochMs = now;
}

void NetworkSimulator::idxChanged(uint64_t idx) {
    if ( m_nmea2000 != nullptr && m_lastIdx != idx) {
        InstrumentInput &instrInput = m_instrDataVector[idx];
        sendEpoch(instrInput);
        m_lastIdx = idx;
    }
}

void NetworkSimulator::stopSimulator() {
    m_bKeepRunning = false;
}


void NetworkSimulator::initN2k(tNMEA2000 &n2k) {
//    n2k.SetForwardStream(&m_n2kDebugStream);  // Debug output on idf monitor
//    n2k.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text
//    n2k.EnableForward(true);

    n2k.SetN2kCANMsgBufSize(8);
    n2k.SetN2kCANReceiveFrameBufSize(100);

    n2k.SetDeviceCount(1);
    n2k.SetProductInformation(
            "12345", // Manufacturer's Model serial code
            130,  // Manufacturer's product code
            "SAILVUE SIM",     // Manufacturer's Model ID
            "1.0",   // Manufacturer's Software version code
            "1.0",// Manufacturer's Model version
            0xff,       // Load equivalency Default=1. x * 50 mA
            0xffff,     // N2K version Default=2101
            0xff        // Certification level Default=1
    );
    n2k.SetDeviceInformation(12345,       // Unique number.
                                  130,    // Device function =  Atmospheric.
                                  85,     // Device class    = External Environment.
                                  2020 ,
                                  4       // Marine industry
    );

    n2k.SetConfigurationInformation("https://github.com/sergei/sailvue");
    n2k.SetMode(tNMEA2000::N2km_NodeOnly);
    n2k.ExtendTransmitMessages(TX_PGNS);
    n2k.ExtendReceiveMessages(RX_PGNS);
}

void NetworkSimulator::transmitSystemTime(uint16_t daysSince1970, double secondsSinceMidnight) {
    tN2kMsg N2kMsg;
    SetN2kSystemTime(N2kMsg, this->m_ucSeqId, daysSince1970, secondsSinceMidnight);

    m_nmea2000->SendMsg(N2kMsg);
}

void NetworkSimulator::transmitFullGpsData(uint16_t daysSince1970, double secondsSinceMidnight,  UtcTime &utc, GeoLoc &loc) {
    auto isLocValid = loc.isValid(utc.getUnixTimeMs());
    double Latitude = isLocValid ? loc.getLat() : N2kDoubleNA;
    double Longitude = isLocValid ? loc.getLon() : N2kDoubleNA;
    double Altitude = isLocValid ? 0 : N2kDoubleNA;
    tN2kGNSStype GNSStype = N2kGNSSt_GPS;
    tN2kGNSSmethod GNSSmethod = isLocValid ? N2kGNSSm_GNSSfix: N2kGNSSm_noGNSS;
    unsigned char nSatellites = isLocValid ? 0 : 10;
    double HDOP = isLocValid ? 1 : N2kDoubleNA;

    tN2kMsg N2kMsg;
    SetN2kGNSS(N2kMsg, this->m_ucSeqId, daysSince1970, secondsSinceMidnight,
               Latitude, Longitude, Altitude,
               GNSStype, GNSSmethod,
               nSatellites, HDOP);

    m_nmea2000->SendMsg(N2kMsg);
}

void NetworkSimulator::transmitRapidGpsData(UtcTime &utc, GeoLoc &loc) {
    auto isLocValid = loc.isValid(utc.getUnixTimeMs());
    double Latitude = isLocValid ? loc.getLat() : N2kDoubleNA;
    double Longitude = isLocValid ? loc.getLon() : N2kDoubleNA;

    tN2kMsg N2kMsg;
    SetN2kLatLonRapid(N2kMsg, Latitude,  Longitude);

    m_nmea2000->SendMsg(N2kMsg);
}

void NetworkSimulator::transmitRapidCogSog(UtcTime &utc, Direction &cog, Speed &sog) const {
    double c = cog.isValid(utc.getUnixTimeMs()) ? DegToRad(cog.getDegrees()) : N2kDoubleNA;
    double s = sog.isValid(utc.getUnixTimeMs()) ? KnotsToms(sog.getKnots()): N2kDoubleNA;
    tN2kMsg N2kMsg;
    SetN2kCOGSOGRapid(N2kMsg, this->m_ucSeqId, N2khr_true, c, s);

    m_nmea2000->SendMsg(N2kMsg);
}

void NetworkSimulator::transmitMagneticHeading(UtcTime &utc, Direction &mag) const {
    double c = mag.isValid(utc.getUnixTimeMs()) ? DegToRad(mag.getDegrees()) : N2kDoubleNA;
    tN2kMsg N2kMsg;
    SetN2kMagneticHeading(N2kMsg, this->m_ucSeqId, c);

    m_nmea2000->SendMsg(N2kMsg);
}
void NetworkSimulator::transmitAttitude(UtcTime &utc, Angle &yaw, Angle &pitch, Angle &roll) const {
    double y = yaw.isValid(utc.getUnixTimeMs()) ? DegToRad(yaw.getDegrees()) : N2kDoubleNA;
    double p = pitch.isValid(utc.getUnixTimeMs()) ? DegToRad(pitch.getDegrees()) : N2kDoubleNA;
    double r = roll.isValid(utc.getUnixTimeMs()) ? DegToRad(roll.getDegrees()) : N2kDoubleNA;
    tN2kMsg N2kMsg;
    SetN2kAttitude(N2kMsg, this->m_ucSeqId, y, p, r);

    m_nmea2000->SendMsg(N2kMsg);
}

double NetworkSimulator:: makePositive(double angle) {
    if ( angle < 0 )
        return angle + 360;
    else
        return angle;
}

void NetworkSimulator::transmitWind(UtcTime &utc, Angle &angle, Speed &speed, bool isApparent) const {
    double a = angle.isValid(utc.getUnixTimeMs()) ? DegToRad(makePositive(angle.getDegrees())) : N2kDoubleNA;
    double s = speed.isValid(utc.getUnixTimeMs()) ? KnotsToms(speed.getKnots()): N2kDoubleNA;
    tN2kWindReference windReference = isApparent ? N2kWind_Apparent : N2kWind_True_water;
    tN2kMsg N2kMsg;
    SetN2kWindSpeed(N2kMsg, this->m_ucSeqId, s, a, windReference);
    m_nmea2000->SendMsg(N2kMsg);
}

void NetworkSimulator::transmitBoatSpeed(UtcTime &utc, Speed &sow) const {
    double s = sow.isValid(utc.getUnixTimeMs()) ? KnotsToms(sow.getKnots()): N2kDoubleNA;
    tN2kMsg N2kMsg;
    SetN2kBoatSpeed(N2kMsg, this->m_ucSeqId, s, N2kDoubleNA, N2kSWRT_Paddle_wheel);

    m_nmea2000->SendMsg(N2kMsg);
}

