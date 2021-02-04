#ifndef MYSOCKET_H
#define MYSOCKET_H

#include <QObject>
#include <winsock2.h>
#include <math.h>
#include "definitionsofmpeg2.h"
class mySocket
{
public:
    mySocket();
    ~mySocket();
    int DecodeConnectEncode_tcp(QString qStrIp, u_short usPort);
    int establishUDPconnection(QString qStrIp, u_short usPort);
    WSADATA mWasData;
    SOCKET m_socketClient_tcp ;
    SOCKET m_socketClient_udp ;
};

#endif // MYSOCKET_H
