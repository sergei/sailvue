#ifndef SAILVUE_NMEA2000_UDP_H
#define SAILVUE_NMEA2000_UDP_H


#include <QUdpSocket>
#include "n2k/NMEA2000/src/NMEA2000.h"

/**
 * This class implements low level CAN interface for tNMEA2000 using UDP packets
 */
class NMEA2000_UDP : public tNMEA2000 {
public:  // Methods the tNMEA2000 wants us to implement for given hardware
    NMEA2000_UDP(uint16_t txPort, uint16_t rxPort);

    bool CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent) override;
    bool CANOpen() override;
    bool CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf) override;
private:
    const uint16_t m_txPort;
    const uint16_t m_rxPort;

    QUdpSocket *m_RxUdpSocket = nullptr;
    QUdpSocket *m_TxUdpSocket = nullptr;
};


#endif //SAILVUE_NMEA2000_UDP_H
