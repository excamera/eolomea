extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
}

class H264_degrader{
public:    
    H264_degrader(size_t _width, size_t _height, size_t _bitrate);
    ~H264_degrader();

    static void bgra2yuv422p(uint8_t* input, uint8_t** output, size_t width, size_t height);
    static void yuv422p2bgra(uint8_t** input, uint8_t* output, size_t width, size_t height);
    
    void degrade(uint8_t **input, uint8_t **output);

private:
    const AVCodecID codec_id = AV_CODEC_ID_H264;
    const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV422P;

    const size_t width;
    const size_t height;
    const size_t bitrate;
    
    size_t frame_count;

    AVCodec *encoder_codec;
    AVCodec *decoder_codec;

    AVCodecContext *encoder_context;
    AVCodecContext *decoder_context;

    AVCodecParserContext *decoder_parser;
    
    AVPacket *encoder_packet;
    AVFrame *encoder_frame;

    AVPacket *decoder_packet;
    AVFrame *decoder_frame;
};

