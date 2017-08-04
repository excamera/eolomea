#include <iostream>
#include <memory>
#include <cstring>

#include "h264_degrader.hh"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

H264_degrader::H264_degrader(size_t _width, size_t _height) :
    width(_width),
    height(_height)
{

    avcodec_register_all();

    decoder_codec = avcodec_find_decoder(codec_id);
    if(decoder_codec == NULL){
        std::cout << "decoder_codec: " << codec_id << " not found!" << "\n";
        throw;
    }

    encoder_codec = avcodec_find_encoder(codec_id);
    if(encoder_codec == NULL){
        std::cout << "encoder_codec: " << codec_id << " not found!" << "\n";
        throw;
    }
    
    AVCodecContext *encoder_context = avcodec_alloc_context3(encoder_codec);
    if(encoder_context == NULL){
        std::cout << "encoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }
    
    AVCodecContext *decoder_context = avcodec_alloc_context3(decoder_codec);
    if(decoder_context == NULL){
        std::cout << "decoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }

    decoder_context->width = width;
    decoder_context->height = height;
    decoder_context->pix_fmt = pix_fmt;
    decoder_context->time_base = (AVRational){1, 25};
    decoder_context->framerate = (AVRational){25, 1};
    decoder_context->bit_rate = 400000;

    encoder_context->width = width;
    encoder_context->height = height;
    encoder_context->pix_fmt = pix_fmt;
    encoder_context->time_base = (AVRational){1, 25};
    encoder_context->framerate = (AVRational){25, 1};
    encoder_context->bit_rate = 400000;
    
    if(avcodec_open2(decoder_context, decoder_codec, NULL) < 0){
        std::cout << "could not open decoder" << "\n";;
        throw;
    }

    if(avcodec_open2(encoder_context, encoder_codec, NULL) < 0){
        std::cout << "could not open encoder" << "\n";;
        throw;
    }
    
    frame = av_frame_alloc();
    if(frame == NULL) {
        std::cout << "AVFrame not allocated" << "\n";
        throw;
    }
    
    frame->width = width;
    frame->height = height;
    frame->format = pix_fmt;

    if(av_frame_get_buffer(frame, 32) < 0){
        std::cout << "AVFrame could not allocate buffer" << "\n";
        throw;
    }

    for(int i = 0; i < 8; i++){
        std::cout << (uint64_t)frame->linesize[i] << "\n";
    }
    
    packet = av_packet_alloc();
    if(packet == NULL) {
        std::cout << "AVPacket not allocated" << "\n";
        throw;
    }

}

H264_degrader::~H264_degrader(){
    // TODO
}

void H264_degrader::degrade(uint8_t **input, uint8_t **output){
    std::memcpy(output[0], input[0], width*height);
    std::memcpy(output[1], input[1], width*height/2);
    std::memcpy(output[2], input[2], width*height/2);

    if(av_frame_make_writable(frame) < 0){
        std::cout << "Could not make the frame writable" << "\n";
        throw;
    }
    
    // TODO put pixels into frame
    return;

    int ret = avcodec_send_frame(encoder_context, frame);
    if (ret < 0) {
        std::cout << "error sending a frame for encoding\n";
        throw;
    }

    /*
    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder_context, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "error during encoding\n");
            exit(1);
        }
    }
    */


}

