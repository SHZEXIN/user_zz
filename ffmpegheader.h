#ifndef FFMPEGHEADER_H
#define FFMPEGHEADER_H
extern "C"
{
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/bsf.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libavutil/imgutils.h"
#include "libavutil/audio_fifo.h"
#include <libswresample/swresample.h>
#include "libavutil/avstring.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}
#endif // FFMPEGHEADER_H
