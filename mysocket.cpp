#include "mysocket.h"
static unsigned int nSerialNumber = 0x01;
mySocket::mySocket()
{
    WSAStartup(MAKEWORD(2,2),&mWasData);
}
mySocket::~mySocket()
{
    closesocket(m_socketClient_tcp);
    closesocket(m_socketClient_udp);
    WSACleanup();
}
static void convert_char_hex( char *peer, int peer_len,char* buff_send)
{
    memset(buff_send,0,sizeof (buff_send));
    char gbuff_recv[1024];
    memset(gbuff_recv,0,sizeof (gbuff_recv));
    memcpy(gbuff_recv,peer,(size_t)peer_len);
    for (int i = 0, j = 0 ;i < peer_len; i=i+2,j++)
    {
        if((peer[i]>= 'a') && (peer[i] <= 'z')) //当前字符为小写字母a~z时
        {
            gbuff_recv[i] = peer[i] -'a' + 0x0a;
        }
        else
        {
            gbuff_recv[i] = peer[i] - 0x30;
        }
        if((peer[i+1]>= 'a') && (peer[i+1] <= 'z')) //当前字符为小写字母a~z时
        {
            gbuff_recv[i+1] = peer[i+1] -'a'+ 0x0a;
        }
        else
        {
            gbuff_recv[i+1] =peer[i+1] - 0x30;
        }
        buff_send[j] = (gbuff_recv[i]<<4) + gbuff_recv[i+1];
    }
}
static void printf_char(char* buff,int bytelen)
{
    int a = 2;
    for (int i = 0;i<bytelen;i=i+2,a=a+2)
    {
        printf("%c%c",buff[i], buff[i+1]);
        printf(" ");
        if( a == 16)
        {
            printf("\n");
            a = 0;
        }
    }
    printf("\n");
}
static bool am_little_endian()
{
    unsigned short i = 1;
    return (int)*((char *)(&i)) ? true : false;
}
static int decodeConnectEncode(char *buff_send)
{
    char buff[1024];
    memset(buff,0,1024);
    unsigned char cVersion = 0x00;//版本号
    unsigned int nMsgLen = 0x36;
    unsigned int nMsgSerialNumber = nSerialNumber;//消息流水号
    unsigned char cIdentification = 0x01;//标识位
    unsigned int nMsgNumber = 0x0605;//消息编号
    unsigned int nSenderId;
    unsigned int nRecvId;
    if(am_little_endian())
    {
        printf("little_endian!\n");
        nSenderId = 192*pow(256,3)+168*pow(256,2)+0*pow(256,1)+91*pow(256,0);//发送设备编码 IP地址
        nRecvId = 192*pow(256,3)+168*pow(256,2)+6*pow(256,1)+235*pow(256,0);//接受设备编码 IP地址
    }
    else
    {
        printf("big_endian!\n");
        nSenderId = 192*pow(256,0)+168*pow(256,1)+0*pow(256,2)+91*pow(256,3);//发送设备编码 IP地址
        nRecvId = 192*pow(256,0)+168*pow(256,1)+6*pow(256,2)+235*pow(256,3);//接受设备编码 IP地址
    }
    unsigned char cReserve[17];//预留
    memset(cReserve,0,17);
    unsigned char EquipmentNum = 0x00;//编码器设备编号
    short int udpPort = 0x2328;//UDP端口
    unsigned char gbuff_recv = 0x20;
    sprintf(buff, "%02x%08x%08x%02x%08x%x%x%02x%034x%08x%08x%08x%04x",
            cVersion,
            nMsgLen,
            nMsgSerialNumber,
            cIdentification,
            nMsgNumber,
            nSenderId,
            nRecvId,
            gbuff_recv,
            *cReserve,
            nSenderId,
            EquipmentNum,
            nRecvId,
            udpPort);
    printf_char(buff,144);
    convert_char_hex(buff,strlen(buff),buff_send);
    nSerialNumber++;
    return 0;
}
static int decodeLoginEncode(char *buff_send)
{
    char buff[1024];
    memset(buff,0,1024);
    unsigned char cVersion = 0x00;//版本号
    //消息包长度 头长40，消息体定长73
    unsigned int nMsgLen = 0x71;
    unsigned int nMsgSerialNumber = nSerialNumber;//消息流水号
    unsigned char cIdentification = 0x01;//标识位
    unsigned int nMsgNumber = 0x0601;//消息编号
    unsigned int nSenderId;
    unsigned int nRecvId = 0;
    if(am_little_endian())
    {
        printf("little_endian!\n");
        nSenderId = 192*pow(256,3)+168*pow(256,2)+0*pow(256,1)+91*pow(256,0);//发送设备编码 IP地址
        nRecvId = 192*pow(256,3)+168*pow(256,2)+6*pow(256,1)+235*pow(256,0);//接受设备编码 IP地址
    }
    else
    {
        printf("big_endian!\n");
        nSenderId = 192*pow(256,0)+168*pow(256,1)+0*pow(256,2)+91*pow(256,3);//发送设备编码 IP地址
        nRecvId = 192*pow(256,0)+168*pow(256,1)+6*pow(256,2)+235*pow(256,3);//接受设备编码 IP地址
    }
    unsigned char cReserve[18];//预留
    memset(cReserve,0,18);
    unsigned char prior = 0x00;//操作员优先级
    unsigned char cSecretKey[64];//登录密钥
    memset(cSecretKey,0,64);
    unsigned char gbuff_recv = 0x20;

    sprintf(buff, "%02x%08x%08x%02x%08x%x%x%02x%034x%x%x%02x%0128x",
            cVersion,
            nMsgLen,
            nMsgSerialNumber,
            cIdentification,
            nMsgNumber,
            nSenderId,
            nRecvId,
            gbuff_recv,
            *cReserve,
            nSenderId,
            nRecvId,
            prior,
            *cSecretKey);
    printf_char(buff,strlen(buff));
    convert_char_hex(buff,strlen(buff),buff_send);
    nSerialNumber++;
    return 0;
}
int mySocket::DecodeConnectEncode_tcp(QString qStrIp, u_short usPort)
{
    int nRet = 0;
    int nSucessNum = 0;
    char *buff_send = new char[1024];
    memset(buff_send,0,sizeof (buff_send));
    //创建套接字
    m_socketClient_tcp = socket(PF_INET, SOCK_STREAM, 0);
    //向服务器发起请求
    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都用0填充
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(qStrIp.toStdString().c_str());
    sockAddr.sin_port = htons(9000);
    nRet = connect(m_socketClient_tcp, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
    if( nRet < 0)
    {
        printf("socket connect %s:%hd fail \n",qStrIp.toStdString().c_str(),usPort);
        return nRet;
    }
    decodeLoginEncode(buff_send);
    int localBuff = send(m_socketClient_tcp, buff_send, buff_send[4], 0);
    if(localBuff < 0)
    {
        printf("send login msg fail \n");
    }
    char gbuff_recv[TS_PACEKT_SIZE];
    memset(gbuff_recv,0,sizeof (gbuff_recv));
    localBuff = recv(m_socketClient_tcp, gbuff_recv, 1024, 0);
    while(1)
    {
        if( localBuff >= 0)
        {
            switch(gbuff_recv[40])
            {
                case 0x00  ://登陆失败
                {
                    printf("login fail \n");
                    break;
                }
                case 0x01  ://登陆成功
                {
                    nSucessNum++;
                    if( nSucessNum >=2)
                    {
                        break;
                    }
                    //解码器向编码器发送建立编码器->解码器输出连接的请求
                    decodeConnectEncode(buff_send);
                    localBuff = send(m_socketClient_tcp, buff_send, buff_send[4], NULL);
                    if(localBuff < 0)
                    {
                        printf("send decode Connect Encode msg fail \n");
                    }
                    break;
                }
                default :
                    break;
            }
        }
        if( nSucessNum >=2)
        {
            break;
        }
    }
    return nRet;
}
int mySocket::establishUDPconnection(QString qStrIp, u_short usPort)
{
    int nRet = 0;
    m_socketClient_udp = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(m_socketClient_udp < 0)
    {
        perror("socket");
        return 1;
    }
    //服务器地址信息
    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));  //每个字节都用0填充
    servAddr.sin_family = PF_INET;
    servAddr.sin_addr.s_addr = inet_addr(qStrIp.toStdString().c_str());
    servAddr.sin_port = htons(usPort);
    nRet = bind(m_socketClient_udp,(struct sockaddr *)&servAddr,sizeof(servAddr));
    return nRet;
}
