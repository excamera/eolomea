extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
//#include "libavutil/imgutils.h"
//#include "libavutil/common.h"
//#include "libavutil/mathematics.h"
}

class H264_degrader{
public:    
    H264_degrader(size_t _width, size_t _height);
    ~H264_degrader();
    
    void degrade(uint8_t **input, uint8_t **output);

private:
    const AVCodecID codec_id = AV_CODEC_ID_H264;
    const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV422P;

    const size_t width;
    const size_t height;
    
    size_t frame_count;

    AVCodec *encoder_codec;
    AVCodec *decoder_codec;

    AVCodecContext *encoder_context;
    AVCodecContext *decoder_context;
    
    AVPacket *packet;
    AVFrame *frame;
    
};

