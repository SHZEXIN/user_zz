#include "mpeg2ts.h"

static TS_PAT gPAT;
static TS_PMT gPMT;
static bool gif_I_Frame = false;//第一个I帧是否出现
static bool nActicateGOPCount = false;
static bool nSecond_I_frame = false;
static bool bPESHead = false;
static uint64_t size = 0;
static uint8_t *data = nullptr;
static uint64_t size_count = 0;
static uint64_t nCountFrame = 0;
static bool bFlag_ifGetPMT = false;
static int thread_exit = 0;
static AVPacket pkt;
#if SAVE_TS
static FILE *ts = fopen("C:/Users/zhou/Desktop/ts/cam126.ts","wb+");
#endif
#if SAVE_ES
static FILE *f_es = fopen("C:/Users/zhou/Desktop/ts/cam126.mpeg2","wb+");
#endif
#if SAVE_YUV
static FILE *f_yuv = fopen("C:/Users/zhou/Desktop/ts/cam126.yuv","wb+");
#endif
mpeg2ts::mpeg2ts(QObject *parent):m_pParent(parent)
{
    m_vOneFullPacket.clear();
    m_vPIDVideo.clear();
    m_screen_w = 720;
    m_screen_h = 576;
    m_pixel_w = m_screen_w;
    m_pixel_h = m_screen_h;
    m_pWindow = nullptr;
    m_pSdlRenderer = nullptr;
    m_pSdlTexture = nullptr;
    m_ifActivateHearbaet = 0;
    frame = av_frame_alloc();
    frame_watermark = av_frame_alloc();
}
mpeg2ts::~mpeg2ts()
{
    WSACleanup();
    m_vOneFullPacket.clear();
    SDL_Quit();
    releaseSDL();
    av_frame_free(&frame);
    av_frame_free(&frame_watermark);
    avcodec_close(m_pDecodeCtx);
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    avfilter_graph_free(&filter_graph);
#if SAVE_TS
    fclose(ts);
#endif
#if SAVE_ES
    fclose(f_es);
#endif
#if SAVE_YUV
    fclose(f_yuv);
#endif
}

static int SDLThreadRefreshVideo(void* arg)
{
    while (!thread_exit)
    {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(5);
    }
    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);
    thread_exit=0;
    return 0;
}
/*
 函数说明：对字符串中所有指定的子串进行替换
 参数：
string resource_str            //源字符串
string sub_str                //被替换子串
string new_str                //替换子串
返回值: string
 */
static std::string subreplace(std::string resource_str, std::string sub_str, std::string new_str)
{
    std::string::size_type pos = 0;
    while((pos = resource_str.find(sub_str)) != std::string::npos)   //替换所有指定子串
    {
        resource_str.replace(pos, sub_str.length(), new_str);
    }
    return resource_str;
}

void mpeg2ts::get_ts_header(TS_head &ts_head,unsigned char *buff)
{
    ts_head.sync_byte = (unsigned char)buff[0];
    ts_head.transport_error_indicator = buff[1] >>7;
    ts_head.payload_unit_start_indicator =  (buff[1] >>6) & 0x1;
    ts_head.transport_priority = buff[1] >>5 & 0x1;
    ts_head.PID = ((buff[1] & 0x1F)<< 8) | buff[2];
    ts_head.transport_scrambling_control = buff[3]>>6 & 0x3;
    ts_head.adaptation_field_control = buff[3]>>4 & 0x3;
    ts_head.continuity_counte =  buff[3] & 0xF;
}
//frame_in:需要加水印的输入帧
//frame_out:完成加水印后取到的帧，执行完函数并完成相关逻辑后需要av_frame_unref(frame_out);
int mpeg2ts::initAVFilter()
{
     AVBufferSinkParams *buffersink_params;
    /*********************************************初始化结构体****************************************/
    int ret;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");                 //输入，原始数据输入处
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");        //输出，处理后的数据输出
    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();
    filter_graph = avfilter_graph_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P };  //摄像头的像素格式
    snprintf(m_args, sizeof(m_args),"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             720, 576, AV_PIX_FMT_YUV420P,1, 25, 1, 1);   //
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", m_args, nullptr, filter_graph);  //初始化并创建输入过滤器参数，通过参数指向具体的过滤器
    if (ret < 0)
    {
        return -1;
    }
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;   //设置输出的像素格式
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",nullptr, buffersink_params, filter_graph); //创建输出过滤器
    av_free(buffersink_params);
    if (ret < 0)
    {
        return -2;
    }
    //参考和总结其他人的参数配置，实现水印格式添加
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    //将一串通过字符串描述的Graph添加到FilterGraph中
    memset(m_filter_descr,0,sizeof(m_filter_descr));//FreeSans verdanaz
    sprintf(m_filter_descr,"[in]drawtext=fontfile=FreeSans.ttf:x=4:y=0:fontsize=15:text='',scale=720:576[a];"
                         "[a]drawtext=fontfile=FreeSans.ttf:x=4:y=15:fontsize=15:text='pix=;GOP=',scale=720:576[b];"
                         "[b]drawtext=fontfile=FreeSans.ttf:x=4:y=30:fontsize=15:text='PID=;FPS=;',scale=720:576[c];"
                        "[c]drawtext=fontfile=FreeSans.ttf:x=4:y=45:fontsize=15:text='transformat=;muxer=',scale=720:576[d];"
                        "[d]drawtext=fontfile=FreeSans.ttf:x=4:y=60:fontsize=15:text='encode=;ratebit=',scale=720:576[out]");

    char temp[24];
    memset(temp,0,sizeof(temp));
    sprintf(temp,"%d%s%d%s%d",m_videoInfo.width,"*",m_videoInfo.height,";  GOP=",m_videoInfo.gopSize);
    std::string aa = subreplace(m_filter_descr,"pix=;GOP=",temp);

    memset(temp,0,sizeof(temp));
    sprintf(temp,"%s%d%s%d"," PID=",m_videoInfo.PID,";     FPS=",m_videoInfo.FPS);
    aa = subreplace(aa.c_str(),"PID=;FPS=;",temp);

    memset(temp,0,sizeof(temp));
    sprintf(temp,"%s%s%s%s"," transformat=",m_videoInfo.transformProtocol.c_str(),"; muxer=",m_videoInfo.muxeFormat.c_str());
    aa = subreplace(aa.c_str(),"transformat=;muxer=",temp);

    memset(temp,0,sizeof(temp));
    sprintf(temp,"%s%s%s%d"," encode=",m_videoInfo.encodeProtocol.c_str(),"; ratebit=",m_videoInfo.ratebit);
    aa = subreplace(aa.c_str(),"encode=;ratebit=",temp);
    if ((ret = avfilter_graph_parse_ptr(filter_graph, aa.c_str(),&inputs, &outputs, nullptr)) < 0)
        return ret;
    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0)
        return ret;
    int parsed_drawtext_0_index = -1;
    for (int i = 0; i < filter_graph->nb_filters; i++)
    {
        AVFilterContext* filter_ctxn = filter_graph->filters[i];
        fprintf(stdout, "[%s](%d) filter_ctxn->name=%s\n",__func__,__LINE__,filter_ctxn->name);
        if(!strcmp(filter_ctxn->name,"Parsed_drawtext_0"))
            parsed_drawtext_0_index = i;
    }
    if(parsed_drawtext_0_index == -1)
        fprintf(stderr, "[%s](%d) no Parsed_drawtext_0\n",__func__,__LINE__);
    filter_ctx = filter_graph->filters[parsed_drawtext_0_index];
    return  0;
}
int mpeg2ts::waterMark(AVFrame *frame_in,AVFrame *frame_out)
{
    int ret = 0;
    char sys_time[64];
    memset(sys_time,0,sizeof (sys_time));
    time_t ltime;
    time(&ltime);
    struct tm* today = localtime(&ltime);
    strftime(sys_time, sizeof(sys_time), "%Y-%m-%d %a %H\\:%M\\:%S", today);       //24小时制
    av_opt_set(filter_ctx->priv, "text", sys_time, 0 );  //设置text到过滤器
    // 往源滤波器buffer中输入待处理的数据
    if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
    {
        return  -3;
    }
    // 从目的滤波器buffersink中输出处理完的数据
    ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
    if (ret < 0)
    {
        return  ret;
    }
    return 0;
}
void mpeg2ts::releaseSDL()
{
    if(m_pSdlRenderer != nullptr)
    {
        SDL_DestroyRenderer(m_pSdlRenderer);
        m_pSdlRenderer = nullptr;
    }
    if(m_pSdlTexture != nullptr)
    {
        SDL_DestroyTexture(m_pSdlTexture);
        m_pSdlTexture = nullptr;
    }
    if(m_pWindow != nullptr)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }
}
int mpeg2ts::_initSDL()
{
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS);
    m_pWindow = SDL_CreateWindow("YUV420P SDL2 Player ",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              m_screen_w,
                              m_screen_h,
                              SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    m_pSdlRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
    if(nullptr == m_pSdlRenderer)
    {
        printf("SDL: could not create renderer - exiting:%s\n",SDL_GetError());
        return 1;
    }
    m_pSdlTexture = SDL_CreateTexture(m_pSdlRenderer,
                                   SDL_PIXELFORMAT_IYUV,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   m_pixel_w,
                                   m_pixel_h);
    if(nullptr == m_pSdlTexture)
    {
        printf("SDL: could not create Texture - exiting:%s\n",SDL_GetError());
        return 1;
    }
    m_sdlRect.x=0;
    m_sdlRect.y=0;
    m_sdlRect.w=m_screen_w;
    m_sdlRect.h=m_screen_h;
//    m_pRefresh_thread = SDL_CreateThread(SDLThreadRefreshVideo,nullptr,nullptr);
//    if(nullptr == m_pRefresh_thread)
//    {
//        printf("SDL: could not create sdlthread - exiting:%s\n",SDL_GetError());
//        return 1;
//    }
    return 0;
}

int mpeg2ts::_SDLShowVideo(AVFrame *avFrame)
{
    SDL_UpdateYUVTexture(m_pSdlTexture,
                         nullptr,
                         avFrame->data[0], avFrame->linesize[0],
                         avFrame->data[1], avFrame->linesize[1],
                         avFrame->data[2], avFrame->linesize[2]);
    //FIX: If window is resize
    m_sdlRect.x = 0;
    m_sdlRect.y = 0;
    m_sdlRect.w = m_screen_w;
    m_sdlRect.h = m_screen_h;
    SDL_RenderClear( m_pSdlRenderer);
    SDL_RenderCopy( m_pSdlRenderer, m_pSdlTexture, nullptr, &m_sdlRect);
    SDL_RenderPresent( m_pSdlRenderer);
    SDL_Event event;
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0;
}
void mpeg2ts::adjust_PAT_table( TS_PAT * ts_pat, unsigned char * buffer)
{
    ts_pat->table_id                    = buffer[0];
    ts_pat->section_syntax_indicator    = buffer[1] >> 7;
    ts_pat->zero                        = buffer[1] >> 6 & 0x1;
    ts_pat->reserved_1                  = buffer[1] >> 4 & 0x3;
    ts_pat->section_length              = (buffer[1] & 0x0F) << 8 | buffer[2];
    ts_pat->transport_stream_id         = buffer[3] << 8 | buffer[4];
    ts_pat->reserved_2                  = buffer[5] >> 6;
    ts_pat->version_number              = buffer[5] >> 1 &  0x1F;
    ts_pat->current_next_indicator      = buffer[5] & 0x1;
    ts_pat->section_number              = buffer[6];
    ts_pat->last_section_number         = buffer[7];
    int len = 0;
    len = 3 + ts_pat->section_length;
    ts_pat->CRC_32   = (buffer[len-4] & 0x000000FF) << 24
                       | (buffer[len-3] & 0x000000FF) << 16
                       | (buffer[len-2] & 0x000000FF) << 8
                       | (buffer[len-1] & 0x000000FF);

    int n = 0;
    TS_PAT_Program PAT_program;
    for ( n = 0; n < ts_pat->section_length - 12; n += 4 )
    {
        unsigned  program_num = buffer[8 + n ] << 8 | buffer[9 + n ];
        //ts_pat->reserved_3 = buffer[10 + n ] >> 5;
        //ts_pat->network_PID = 0x00;
        PAT_program.reserved_3 = buffer[10 + n ] >> 5;
        if ( program_num == 0x00)
        {
            //ts_pat->network_PID = (buffer[10 + n ] & 0x1F) << 8 | buffer[11 + n ];
            PAT_program.program_map_PID_network_PID =  (buffer[10 + n ] & 0x1F) << 8 | buffer[11 + n ];
            PAT_program.program_number = program_num;
            //TS_network_Pid = packet->network_PID; //记录该TS流的网络PID
           // printf(" TS new PID = %0x \n\n", PAT_program.program_map_PID_network_PID  );
        }
        else
        {
           PAT_program.program_map_PID_network_PID = (buffer[10 + n] & 0x1F) << 8 | buffer[11 + n];
           PAT_program.program_number = program_num;
           ts_pat->vProgram.push_back( PAT_program );
          // printf(" TS  PAT_program  PID %0x \n\n", PAT_program.program_map_PID_network_PID  );
        }
    }
}
void mpeg2ts::adjust_PMT_table ( TS_PMT * ts_pmt, unsigned char * buffer )
{
    ts_pmt->table_id                            = buffer[0];
    ts_pmt->section_syntax_indicator            = buffer[1] >> 7;
    ts_pmt->zero                                = buffer[1] >> 6 & 0x01;
    ts_pmt->reserved_1                            = buffer[1] >> 4 & 0x03;
    ts_pmt->section_length                        = (buffer[1] & 0x0F) << 8 | buffer[2];
    ts_pmt->program_number                        = buffer[3] << 8 | buffer[4];
    ts_pmt->reserved_2                            = buffer[5] >> 6;
    ts_pmt->version_number                        = buffer[5] >> 1 & 0x1F;
    ts_pmt->current_next_indicator                = (buffer[5] << 7) >> 7;
    ts_pmt->section_number                        = buffer[6];
    ts_pmt->last_section_number                    = buffer[7];
    ts_pmt->reserved_3                            = buffer[8] >> 5;
    ts_pmt->PCR_PID                                = ((buffer[8] << 8) | buffer[9]) & 0x1FFF;

 //PCRID = packet->PCR_PID;

    ts_pmt->reserved_4                            = buffer[10] >> 4;
    ts_pmt->program_info_length                    = (buffer[10] & 0x0F) << 8 | buffer[11];
    // Get CRC_32
    int len = 0;
    len = ts_pmt->section_length + 3;
    ts_pmt->CRC_32 = (buffer[len-4] & 0x000000FF) << 24
                     | (buffer[len-3] & 0x000000FF) << 16
                     | (buffer[len-2] & 0x000000FF) << 8
                     | (buffer[len-1] & 0x000000FF);

    int pos = 12;
    // program info descriptor
    if ( ts_pmt->program_info_length != 0 )
        pos += ts_pmt->program_info_length;
    // Get stream type and PID
    for ( ; pos <= (ts_pmt->section_length + 2 ) -  4; )
    {
        TS_PMT_Stream pmt_stream;
        pmt_stream.stream_type =  buffer[pos];
        ts_pmt->reserved_5  =   buffer[pos+1] >> 5;
        pmt_stream.elementary_PID =  ((buffer[pos+1] << 8) | buffer[pos+2]) & 0x1FFF;
        ts_pmt->reserved_6     =   buffer[pos+3] >> 4;
        pmt_stream.ES_info_length =   (buffer[pos+3] & 0x0F) << 8 | buffer[pos+4];
        pmt_stream.descriptor = 0x00;
        if (pmt_stream.ES_info_length != 0)
        {
            pmt_stream.descriptor = buffer[pos + 5];

            for( int len = 2; len <= pmt_stream.ES_info_length; len ++ )
            {
                pmt_stream.descriptor = pmt_stream.descriptor<< 8 | buffer[pos + 4 + len];
            }
            pos += pmt_stream.ES_info_length;
        }
        pos += 5;
        ts_pmt->vPMT_Stream.push_back( pmt_stream );
        //TS_Stream_type.push_back( pmt_stream );
    }
}
bool mpeg2ts::isPESHead(unsigned char * buffer)
{
    if( buffer[0] == 0x00
        && buffer[1] == 0x00
        && buffer[2] == 0x01 )
    {
        return true;
    }
    else
    {
        return false;
    }
}
int mpeg2ts::openDecoder()
{
    /* find the MPEG-1 video decoder */
    m_pCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
    if (!m_pCodec)
    {
        fprintf(stderr, "Codec not found\n");
        return 1;
    }
    // 获取裸流的解析器 AVCodecParserContext(数据)  +  AVCodecParser(方法)
    parser = av_parser_init(m_pCodec->id);
    if (!parser) {
        fprintf(stderr, "Parser not found\n");
        return 1;
    }
    m_pDecodeCtx = avcodec_alloc_context3(m_pCodec);
    if (!m_pDecodeCtx)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        return 1;
    }
    if (avcodec_open2(m_pDecodeCtx, m_pCodec, nullptr) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        return 1;
    }
    return 0;
}
int mpeg2ts::writeYUV(AVFrame *frame,FILE *f)
{
    //每帧大小
    int newSize = frame->height *frame->width * 1.5;
    int a=0,i;
    unsigned char *buf = new unsigned char[newSize];
    for (i=0; i<frame->height; i++)
    {
        memcpy(buf+a,frame->data[0] + i * frame->linesize[0], frame->width);
        a+=frame->width;
    }
    for (i=0; i<frame->height/2; i++)
    {
        memcpy(buf+a,frame->data[1] + i * frame->linesize[1], frame->width/2);
        a+=frame->width/2;
    }
    for (i=0; i<frame->height/2; i++)
    {
        memcpy(buf+a,frame->data[2] + i * frame->linesize[2], frame->width/2);
        a+=frame->width/2;
    }
    fwrite(buf, 1, newSize, f);
    delete [] buf;
    buf = nullptr;
    nCountFrame++;
    printf("yuv frame %d \n",nCountFrame);
    fflush(f);
    return 0;
}
int mpeg2ts::decodeToYUV(int nIfUseFilter)
{
    int nRet = avcodec_send_packet(m_pDecodeCtx, &pkt);
    while( 0 == nRet)
    {
        int got_picture =avcodec_receive_frame(m_pDecodeCtx, frame);
        if(0 == got_picture)//
        {
            if( 1 == nIfUseFilter)
            {
                waterMark(frame,frame_watermark);
#if SDL_RENDER_YUV
                _SDLShowVideo(frame_watermark);
#endif
#if SAVE_YUV
                writeYUV(frame_watermark,f_yuv);
#endif
                av_frame_unref(frame_watermark);
            }
            else
            {
#if SDL_RENDER_YUV
                _SDLShowVideo(frame);
#endif
#if SAVE_YUV
                writeYUV(frame,f_yuv);
#endif
                av_frame_unref(frame);
            }
        }
        else
        {
            av_frame_unref(frame_watermark);
            break;
        }
    }
    av_packet_unref(&pkt);
    av_frame_unref(frame_watermark);
    return 0;
}
bool mpeg2ts::doeshavePictureHeader(unsigned char* buff,int len)
{
    for (int i = 0; i < len; i++)
    {
        if( buff[i] == 0x00
            && buff[i+1] == 0x00
            && buff[i+2] == 0x01
            && buff[i+3] == 0x00)
        {
            return true;
        }
    }
    return false;
}
bool mpeg2ts::is_I_frame(unsigned char* buff,int len)
{
    for (int i = 0; i < len; i++)
    {
        if( buff[i] == 0x00
            && buff[i+1] == 0x00
            && buff[i+2] == 0x01
            && buff[i+3] == 0x00)
        {
            if(0x1 == (buff[i+5]&0x38)>>3)//I帧
            {
                return true;
            }
        }
    }
    return false;
}
void mpeg2ts::insertVector(unsigned char *buff,int len)
{
    for(int i = 0; i < len; i++)
    {
       m_vOneFullPacket.push_back(buff[i]);
    }
}
void mpeg2ts::getDecodeData(int nIfUseFilter)
{
    //如果未到文件结尾
    int ret = 0;
    uint8_t *data_pkt = nullptr;
    printf(" size_count = %lld \n",size_count);
    if(size_count >= 4096)
    {
        unsigned char OneFullPacket[size_count];
        memset(OneFullPacket,0,size_count);
        memcpy(OneFullPacket,&m_vOneFullPacket[0],size_count);
        data_pkt = OneFullPacket;
        while(size_count > 0 )
        {
            av_frame_unref(frame);
            ret = av_parser_parse2(parser, m_pDecodeCtx, &pkt.data, &pkt.size,
                                   data_pkt, size_count,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data_pkt += ret;   // 跳过已经解析的数据
            size_count -= ret;   // 对应的缓存大小也做相应减小
            m_vOneFullPacket.erase(m_vOneFullPacket.begin(),m_vOneFullPacket.begin()+ret);
            if (pkt.size)
            {
                decodeToYUV(nIfUseFilter);//decode
            }
        }
    }
}
void mpeg2ts::getPAT(int nTsPacket)
{
    gPAT.vProgram.clear();
    adjust_PAT_table(&gPAT,(unsigned char*)&m_buff_recv[5+nTsPacket*188]);
    if(bFlag_ifGetPMT == true)
    {
        gPMT.vPMT_Stream.clear();
        bFlag_ifGetPMT = false;
    }
}
void mpeg2ts::getPMT(int nTsPacket,TS_head &ts_head)
{
    auto iter = gPAT.vProgram.begin();
    for ( ;iter != gPAT.vProgram.end(); iter++)
    {
        if(0x0001 == (*iter).program_number //PMT
                && ts_head.PID == (*iter).program_map_PID_network_PID) //PID一致
        {
            adjust_PMT_table( &gPMT, (unsigned char*)&m_buff_recv[5+nTsPacket*188]);
            bFlag_ifGetPMT = true;
        }
    }
}
void mpeg2ts::writeESData()
{
#if SAVE_ES
    fwrite(data,1,size,f_es);
    fflush(f_es);
#endif
    insertVector(data,size);
    size_count+=size;
}
void mpeg2ts::processPESWithHeaderAndAdaptivefield(int nTsPacket)
{
    int remain_pes_head_len = 0;
    if(bPESHead == false)
    {
        bPESHead = isPESHead((unsigned char*)&m_buff_recv[nTsPacket*188+4+1+(uint8_t)m_buff_recv[4+nTsPacket*188]]);
    }
    if(bPESHead == true)
    {
        remain_pes_head_len = (uint8_t)m_buff_recv[nTsPacket*188 + 4 + 1 + (uint8_t)m_buff_recv[4+nTsPacket*188] + PES_HEAD_FIX_LEN-1];
        remain_pes_head_len = remain_pes_head_len & 0x0f;
        size = 188-4-1-(uint8_t)m_buff_recv[4+nTsPacket*188]-PES_HEAD_FIX_LEN-remain_pes_head_len;
        data = (uint8_t *)&m_buff_recv[nTsPacket*188 + 4 + 1 + (uint8_t)m_buff_recv[4+nTsPacket*188] + PES_HEAD_FIX_LEN+remain_pes_head_len];
        if(gif_I_Frame == true)
        {
            writeESData();
        }
        else
        {
            gif_I_Frame = is_I_frame(data,size);
            if(gif_I_Frame == true)
            {
                writeESData();
                getPixFormat();
            }
        }
     }
}
void mpeg2ts::processPESWithHeaderButAdaptivefield(int nTsPacket)
{
    int remain_pes_head_len = 0;
    if(bPESHead == false)
    {
        bPESHead = isPESHead((uint8_t *)&m_buff_recv[ nTsPacket*188+4 + (uint8_t)m_buff_recv[4+nTsPacket*188]]);
    }
    if(bPESHead == true)
    {
        remain_pes_head_len = (uint8_t)m_buff_recv[nTsPacket*188 + 4  + PES_HEAD_FIX_LEN-1];
        remain_pes_head_len = remain_pes_head_len & 0x0f;
        size = 188-4-PES_HEAD_FIX_LEN-remain_pes_head_len-remain_pes_head_len;
        data = (uint8_t *)&m_buff_recv[nTsPacket*188+4  + PES_HEAD_FIX_LEN+remain_pes_head_len];
        if(gif_I_Frame == true)
        {
            writeESData();
        }
        else
        {
            gif_I_Frame = is_I_frame(data,size);
            if(gif_I_Frame == true)
            {
                writeESData();
                getPixFormat();
            }
        }
    }
}
void mpeg2ts::processPESWithAdaptivefieldButHeader(int nTsPacket)//处理没有包头，有自适应字段的PES包
{
    if(bPESHead == false)
    {
        bPESHead = isPESHead((uint8_t *)&m_buff_recv[nTsPacket*188+4]);
    }
    if(bPESHead == true)
    {
        size = 188-4-1-(uint8_t)m_buff_recv[4+nTsPacket*188];
        data = (uint8_t *)&m_buff_recv[nTsPacket*188 + 4 + 1 + (uint8_t)m_buff_recv[4+nTsPacket*188]];
        if(gif_I_Frame == true)
        {
            writeESData();
        }
        else
        {
            gif_I_Frame = is_I_frame(data,size);
            if(gif_I_Frame == true)
            {
                writeESData();
                getPixFormat();
            }
        }
    }
}

void mpeg2ts::processPESWithOnlyES(int nTsPacket)//处理没有包头，没有自适应字段的PES包
{
    if(bPESHead == false)
    {
        bPESHead = isPESHead((uint8_t *)&m_buff_recv[nTsPacket*188+4]);
    }
    if(bPESHead == true)
    {
        size = 188-4;
        data = (uint8_t *)&m_buff_recv[nTsPacket*188+4];
        if(gif_I_Frame == true)
        {
            writeESData();
        }
        else
        {
            gif_I_Frame = is_I_frame(data,size);
            if(gif_I_Frame == true)
            {
                writeESData();
                getPixFormat();
            }
        }
    }
}
void mpeg2ts::processProgram(int nTsPacket,TS_head &ts_head)
{
    auto iter = gPMT.vPMT_Stream.begin();
    for ( ;iter != gPMT.vPMT_Stream.end(); iter++)
    {
        if((*iter).elementary_PID == ts_head.PID)
        {
            if(0x1 == ts_head.payload_unit_start_indicator)//pes包头
            {
                if( ts_head.adaptation_field_control == 2
                        || ts_head.adaptation_field_control == 3)//有自适应字段
                {
                    processPESWithHeaderAndAdaptivefield(nTsPacket);//有pes包头，有自适应字段
                }
                else
                {
                    processPESWithHeaderButAdaptivefield(nTsPacket);//有pes包头，无自适应字段
                }
            }
            else
            {
                if( ts_head.adaptation_field_control == 2
                        || ts_head.adaptation_field_control == 3)
                {
                    processPESWithAdaptivefieldButHeader(nTsPacket);//无pes包头，有自适应字段
                }
                else
                {
                    processPESWithOnlyES(nTsPacket);//无pes包头，无自适应字段
                }
            }
        }
    }
}
int mpeg2ts::getBasicInfo()
{
    int nRet = 0;
    TS_head ts_head;
    //一个ts有7个包
    int nTsPacket = 0;
    getDecodeData(0);
    while( nTsPacket < 7)
    {
        get_ts_header(ts_head,(unsigned char*)&m_buff_recv[nTsPacket*188]);
        if(ts_head.sync_byte == 0x47)
        {
            m_videoInfo.transformProtocol = "TS";
            m_videoInfo.muxeFormat = "ts";
        }
        else
        {
            printf("同步字节不是0x47，错误 \n");
            return 1;
        }
        if(ts_head.PID == 0x0000)//获取PAT
        {
            getPAT(nTsPacket);
        }
        else
        {
            if( false == bFlag_ifGetPMT )//获取PMT
            {
                getPMT(nTsPacket,ts_head);
                if (gPMT.vPMT_Stream.size() > 0)
                {
                    m_videoInfo.PID = gPMT.vPMT_Stream[0].elementary_PID;
                    if(gPMT.vPMT_Stream[0].stream_type == 2)
                    {
                        m_videoInfo.encodeProtocol = "MPEG2";
                    }
                }
            }
            else
            {
                processProgram(nTsPacket,ts_head);//处理网络流
                if(true == nActicateGOPCount && true == doeshavePictureHeader(data,size) && false == is_I_frame(data,size))
                {
                    m_videoInfo.gopSize++;
                }
                if(true == nActicateGOPCount && true == is_I_frame(data,size) && m_videoInfo.gopSize >0)
                {
                    nSecond_I_frame = true;
                }
            }
        }
        nTsPacket++;
    }
    return nRet;
}
int mpeg2ts::analysisStream()
{
    int nRet = 0;
    TS_head ts_head;
    //一个ts有7个包
    int nTsPacket = 0;
    getDecodeData();
    while( nTsPacket < 7)
    {
        get_ts_header(ts_head,(unsigned char*)&m_buff_recv[nTsPacket*188]);
        if(ts_head.sync_byte != 0x47)
        {
            printf("同步字节不是0x47，错误 \n");
            return 1;
        }
        if(ts_head.PID == 0x0000)//获取PAT
        {
            getPAT(nTsPacket);
        }
        else
        {
            if( false == bFlag_ifGetPMT )//获取PMT
            {
                getPMT(nTsPacket,ts_head);
            }
            else
            {
                processProgram(nTsPacket,ts_head);//处理网络流
            }
        }
        nTsPacket++;
    }
    return nRet;
}

void mpeg2ts::getPixFormat()
{
    m_videoInfo.width = ((unsigned int)data[4]<<4)+((unsigned int)data[5]>>4);
    m_videoInfo.height = ((unsigned int)data[5]<<8)+(unsigned int)data[6];
    if((data[7]&0x0F) == 3)
    {
        m_videoInfo.FPS = 25;
    }
    m_videoInfo.ratebit = (((unsigned int)data[9])+((unsigned int)data[8]<<8))<<2;
    m_videoInfo.ratebit = m_videoInfo.ratebit + (data[10]>>6);
    nActicateGOPCount = true;
}
int mpeg2ts::ifGetAllVideoInfo()
{
    return m_videoInfo.FPS &&
            m_videoInfo.PID &&
            m_videoInfo.width&&
            m_videoInfo.height&&
            (nSecond_I_frame == true)&&
            m_videoInfo.ratebit&&
            (m_videoInfo.muxeFormat!="")&&
            (m_videoInfo.encodeProtocol!="")&&
            (m_videoInfo.transformProtocol!="");
}
int mpeg2ts::probeStreamBasicInfo()//先读几帧获取基本数据
{
    int nRet = 0;
    size_t n = 0;
    while(!ifGetAllVideoInfo())
    {
#if SDL_RENDER_YUV
        waitSDLEvent();
#endif
        //获取数据
        memset(m_buff_recv, 0, sizeof(m_buff_recv));
        nRet = recv(m_mySocket.m_socketClient_udp, m_buff_recv, TS_PACEKT_SIZE, 0);
        if( nRet >= 0)
        {
#if SAVE_TS
            fwrite(gbuff_recv,1,nRet,ts);
#endif
            getBasicInfo();
            printf("get ts %lld \n",n++);
        }
        else
        {
             return 1;
        }
    }
}
int mpeg2ts::waitSDLEvent()
{
    SDL_WaitEvent(&m_event);
    switch (m_event.type)
    {
        case REFRESH_EVENT:
        {
            break;
        }
        case SDL_QUIT:
        {
            thread_exit = 1;
            SDL_DestroyRenderer(m_pSdlRenderer);
            SDL_DestroyTexture(m_pSdlTexture);
            SDL_DestroyWindow(m_pWindow);
            m_pWindow = nullptr;
            return 1;
        }
        case SFM_BREAK_EVENT:
        {
            return 1;
        }
        case SDL_WINDOWEVENT:
        {
            SDL_GetWindowSize(m_pWindow, &m_screen_w, &m_screen_h);
            break;
        }
        default:
            break;
    }
    SDL_Event event;
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0;
}
int mpeg2ts::getStream()
{
    int nRet = 0;
    size_t n = 0;
    while(1)
    {
#if SDL_RENDER_YUV
       waitSDLEvent();
#endif
        //获取数据
        memset(m_buff_recv, 0, sizeof(m_buff_recv));
        nRet = recv(m_mySocket.m_socketClient_udp, m_buff_recv, TS_PACEKT_SIZE, 0);
        if( nRet >= 0)
        {
#if SAVE_TS
            fwrite(gbuff_recv,1,nRet,ts);
#endif
            analysisStream();
            printf("get ts %lld \n",n++);
        }
        else
        {
            printf("udp datasize null \n",n++);
            return 1;
        }
    }
}

int mpeg2ts::_start()
{
    qDebug()<<"mpeg2 thread: tid  = "<<QThread::currentThreadId();
    if(0 != m_mySocket.DecodeConnectEncode_tcp("192.168.7.126",9000))
    {
        printf("connect tcp 192.168.7.126:9000 fail \n");
        return 1;
    }
    if(0 != m_mySocket.establishUDPconnection("192.168.0.91",9000))
    {
        printf("connect udp 192.168.7.126:9000 fail \n");
        return 1;
    }
    if(0 != openDecoder())
    {
        printf("open decoder fail \n");
        return 1;
    }
#if SDL_RENDER_YUV
    //初始化SDL
    if(0 != _initSDL())
    {
        printf("init SDL fail \n");
        return 1;
    }
#endif
    //先探查视频流的基本信息
    if(0 != probeStreamBasicInfo())
    {
        printf("probe stream basic information fail \n");
        return 1;
    }
    //初始化滤镜，用来帖水印
    if(0 != initAVFilter())
    {
        printf("init avfilter fail \n");
        return 1;
    }
    while(1)
    {
        if(!getStream())
        {
            break;
        }
    }
    return 0;
}

