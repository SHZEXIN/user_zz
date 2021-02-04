#ifndef DEFINITIONSOFMPEG2_H
#define DEFINITIONSOFMPEG2_H
#include <vector>
#include <string>
//====================功能开关 0：关   1：开
#define SDL_RENDER_YUV 1    //SDL播放YUV
#define SAVE_TS 0           //保存ts流
#define SAVE_ES 0           //保存es流
#define SAVE_YUV 0          //保存yuv
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)
#define TS_PACEKT_SIZE 1358
#define PES_HEAD_FIX_LEN 9
/*************************************************/
typedef struct _VideoStreamInfo
{
    _VideoStreamInfo()
    {
        PID = 0;
        encodeProtocol = "";
        transformProtocol = "";
        height = 0;
        width = 0;
        gopSize = 0;
        FPS = 0;
        ifFixRateBit = 0;
        ratebit = 0;
        muxeFormat = "";
    }
    unsigned int PID;//
    std::string encodeProtocol;//
    std::string transformProtocol;//
    unsigned int height;
    unsigned int width;
    int gopSize;
    unsigned int FPS;
    int ifFixRateBit;//0:是固定码流，1：不是固定码流
    unsigned int ratebit;
    std::string muxeFormat;//封装格式
}VideoStreamInfo;
typedef struct _TS_head
{
    unsigned char sync_byte                     :8;           //就是0x47,这是DVB TS规定的同步字节,固定是0x47.
    unsigned char transport_error_indicator     :1;           // 0 表示当前包没有发生传输错误.
    unsigned char payload_unit_start_indicator  :1;           // 含义参考ISO13818-1标准文档
    unsigned char transport_priority            :1;           //    表示当前包是低优先级.
    unsigned short PID                          :13;
    unsigned char transport_scrambling_control  :2;           //表示节目没有加密
    unsigned char adaptation_field_control      :2;           // 即0x01,具体含义请参考ISO13818-1
    unsigned char continuity_counte             :4;           //即0x02,表示当前传送的相同类型的包是第3个
}TS_head;

typedef struct TS_PAT_Program
{
    unsigned program_number                 :  16;  //节目号
    unsigned reserved_3                     :  3; // 保留位
    unsigned program_map_PID_network_PID    :  13; // 节目映射表的PID，节目号大于0时对应的PID，每个节目对应一个
}TS_PAT_Program;
typedef struct TS_PAT
{
    unsigned table_id                     : 8; //固定为0x00 ，标志是该表是PAT表
    unsigned section_syntax_indicator     : 1; //段语法标志位，固定为1
    unsigned zero                         : 1; //0
    unsigned reserved_1                   : 2; // 保留位
    unsigned section_length               : 12; //表示从下一个字段开始到CRC32(含)之间有用的字节数
    unsigned transport_stream_id          : 16; //该传输流的ID，区别于一个网络中其它多路复用的流
    unsigned reserved_2                   : 2;// 保留位
    unsigned version_number               : 5; //范围0-31，表示PAT的版本号
    unsigned current_next_indicator       : 1; //发送的PAT是当前有效还是下一个PAT有效
    unsigned section_number               : 8; //分段的号码。PAT可能分为多段传输，第一段为00，以后每个分段加1，最多可能有256个分段
    unsigned last_section_number          : 8;  //最后一个分段的号码
    std::vector<TS_PAT_Program> vProgram;
    //unsigned reserved_3                   : 3; // 保留位
    //unsigned network_PID                  : 13; //网络信息表（NIT）的PID,节目号为0时对应的PID为network_PID
    unsigned CRC_32                       : 32;  //CRC32校验码
} TS_PAT;
typedef struct TS_PMT_Stream
{
 unsigned stream_type                       : 8; //指示特定PID的节目元素包的类型。该处PID由elementary PID指定
 unsigned elementary_PID                    : 13; //该域指示TS包的PID值。这些TS包含有相关的节目元素
 unsigned ES_info_length                    : 12; //前两位bit为00。该域指示跟随其后的描述相关节目元素的byte数
 unsigned descriptor;
}TS_PMT_Stream;
//PMT 表结构体
typedef struct TS_PMT
{
    unsigned table_id                        : 8; //固定为0x02, 表示PMT表
    unsigned section_syntax_indicator        : 1; //固定为0x01
    unsigned zero                            : 1; //0x01
    unsigned reserved_1                      : 2; //0x03
    unsigned section_length                  : 12;//首先两位bit置为00，它指示段的byte数，由段长度域开始，包含CRC。
    unsigned program_number                    : 16;// 指出该节目对应于可应用的Program map PID
    unsigned reserved_2                        : 2; //0x03
    unsigned version_number                    : 5; //指出TS流中Program map section的版本号
    unsigned current_next_indicator            : 1; //当该位置1时，当前传送的Program map section可用；
                                                     //当该位置0时，指示当前传送的Program map section不可用，下一个TS流的Program map section有效。
    unsigned section_number                    : 8; //固定为0x00
    unsigned last_section_number            : 8; //固定为0x00
    unsigned reserved_3                        : 3; //0x07
    unsigned PCR_PID                        : 13; //指明TS包的PID值，该TS包含有PCR域，
            //该PCR值对应于由节目号指定的对应节目。
            //如果对于私有数据流的节目定义与PCR无关，这个域的值将为0x1FFF。
    unsigned reserved_4                        : 4; //预留为0x0F
    unsigned program_info_length            : 12; //前两位bit为00。该域指出跟随其后对节目信息的描述的byte数。

    std::vector<TS_PMT_Stream> vPMT_Stream;  //每个元素包含8位, 指示特定PID的节目元素包的类型。该处PID由elementary PID指定
    unsigned reserved_5                        : 3; //0x07
    unsigned reserved_6                        : 4; //0x0F
    unsigned CRC_32                            : 32;
} TS_PMT;
#endif // DEFINITIONSOFMPEG2_H
