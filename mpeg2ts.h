#ifndef MPEG2TS_H
#define MPEG2TS_H

#include <QObject>
#include <definitionsofmpeg2.h>
#include <ffmpegheader.h>
#include "mysocket.h"
#include <SDL.h>
#include <QThread>
#include <QDebug>
class mpeg2ts: public QObject
{
    Q_OBJECT
public:
    mpeg2ts(QObject *parent = nullptr);
    ~mpeg2ts();
    int _start();
private:
    int getStream();//获取udp流
    int analysisStream();//分析ts流
    void insertVector(unsigned char *buff,int len);//插入vOneFullPacket
    void getDecodeData(int nIfUseFilter = 1);//获取需要编码的数据,//nIfUseFilter:0不用滤镜，1使用滤镜
    void writeESData();//写ES数据
    void processPESWithHeaderAndAdaptivefield(int nTsPacket);//处理有包头，有自适应字段的PES包
    void processPESWithHeaderButAdaptivefield(int nTsPacket);//处理有没有包头，有自适应字段的PES包
    void processPESWithAdaptivefieldButHeader(int nTsPacket);//处理没有包头，有自适应字段的PES包
    void processPESWithOnlyES(int nTsPacket);//处理没有包头，没有自适应字段的PES包
    void processProgram(int nTsPacket,TS_head &ts_head);//处理一个ts包
    int openDecoder();//打开解码器
    int _initSDL();//初始化SDL
    void releaseSDL();//释放SDL
    int _SDLShowVideo(AVFrame *avFrame);//SDL窗口展示
    int decodeToYUV(int nIfUseFilter = 1);//nIfUseFilter:0不用滤镜，1使用滤镜
    int establishUDPconnection(QString qStrIp, u_short usPort);//建立UDP连接
    int probeStreamBasicInfo();//先读几帧获取基本数据
    int ifGetAllVideoInfo();//返回0：所有基础信息已经获取；返回1：还有数据获取
    int getBasicInfo();//获取一个TS包的数据
    bool is_I_frame(unsigned char* buff,int len);//判断是否是I帧
    void getPixFormat();//获取分辨率
    int initAVFilter();//初始化滤镜
    int waitSDLEvent();//SDL事件判断
    void getPAT(int nTsPacket);//获取PAT
    void getPMT(int nTsPacket,TS_head &ts_head);//获取PMT
    void get_ts_header(TS_head &ts_head,unsigned char *buff);//获取ts的包头
    void adjust_PMT_table (TS_PMT * ts_pmt, unsigned char * buffer);//
    void adjust_PAT_table( TS_PAT * ts_pat, unsigned char * buffer);//
    int waterMark(AVFrame *frame_in,AVFrame *frame_out);//帖水印
    bool isPESHead(unsigned char * buffer);//判断是否是有PES包头
    int writeYUV(AVFrame *frame,FILE *f);//写YUV数据
    bool doeshavePictureHeader(unsigned char* buff,int len);//判断是否有PictureHeader，用来判断是否是I帧
private:
    std::vector<unsigned char> m_vOneFullPacket;
    std::vector<int> m_vPIDVideo;
private://SDL
    int m_screen_w;
    int m_screen_h;
    int m_pixel_w;
    int m_pixel_h;
    SDL_Window *m_pWindow;
    SDL_Renderer*m_pSdlRenderer;
    SDL_Texture* m_pSdlTexture;
    SDL_Rect m_sdlRect;
    SDL_Thread *m_pRefresh_thread;
    SDL_Thread *m_pDecode_thread;
    int m_ifActivateHearbaet;
    VideoStreamInfo m_videoInfo;
    SDL_Event m_event;
private://ffmpeg
    AVFilterGraph *filter_graph = nullptr; //最关键的过滤器结构体
    AVFilterContext* filter_ctx = nullptr;//上下文
    AVFilterInOut *outputs  = nullptr;
    AVFilterInOut *inputs = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;
    AVCodecContext* m_pDecodeCtx = nullptr;
    AVCodec *m_pCodec = nullptr;
    AVCodecParserContext *parser = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *frame_watermark = nullptr;
    char m_args[256];
    char m_filter_descr[500];
private://socket
    mySocket m_mySocket;
    char m_buff_recv[TS_PACEKT_SIZE];
    QObject *m_pParent;
};

#endif // MPEG2TS_H
