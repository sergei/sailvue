#include "NMEA2000_UDP.h"
#include <iostream>
#include <ctime>
#include <QNetworkDatagram>

extern "C" {
    // Current uptime in milliseconds. Must be implemented by application.
    uint32_t millis(){
        struct timespec ticker{};

        clock_gettime(CLOCK_MONOTONIC, &ticker);
        return (((uint64_t)ticker.tv_sec * 1000) + (ticker.tv_nsec / 1000000));
    }
}

NMEA2000_UDP::NMEA2000_UDP(uint16_t txPort, uint16_t rxPort)
: m_txPort(txPort),
  m_rxPort(rxPort)
{
    m_RxUdpSocket = new QUdpSocket();
    m_RxUdpSocket->bind(QHostAddress::LocalHost, m_rxPort);
    m_TxUdpSocket = new QUdpSocket();
}

bool NMEA2000_UDP::CANOpen() {
    return true;
}

const int TWAI_FRAME_MAX_DLC = 8;           /**< Max data bytes allowed in TWAI */
const int MAX_UDP_FRAME_SIZE = 5 + TWAI_FRAME_MAX_DLC; // 4 bytes id + 1 byte DLC + 8 bytes data

bool NMEA2000_UDP::CANSendFrame(unsigned long msg_id, unsigned char msg_len, const unsigned char *buf, bool wait_sent) {
    unsigned char udp_data[MAX_UDP_FRAME_SIZE];

    int net_id = htonl(msg_id);
    memcpy(udp_data, &net_id, 4);
    memcpy(udp_data + 4, &msg_len, 1);
    for(int i = 0; i < msg_len; i++){
        udp_data[5 + i] = buf[i];
    }
    int len = 5 + msg_len;
    m_TxUdpSocket->writeDatagram((char*)udp_data, len, QHostAddress::Broadcast, m_txPort);
    return true;
}

bool NMEA2000_UDP::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf) {
    if ( m_RxUdpSocket->hasPendingDatagrams() ){
        QNetworkDatagram datagram = m_RxUdpSocket->receiveDatagram();
        std::cout << "Received datagram from " << datagram.senderAddress().toString().toStdString() << std::endl;

        char *recvbuf =  datagram.data().data();
        int net_id = 0;
        memcpy(&net_id, recvbuf, 4);
        id = ntohl(net_id);
        len = recvbuf[4];
        if ( len < TWAI_FRAME_MAX_DLC ){
            for(int i = 0; i < len; i++){
                buf[i] = recvbuf[5 + i];
            }
        }else{
            std::cout << "Received frame with invalid length " << len << std::endl;
        }
        return true;
    }else{
        return false;
    }
}

