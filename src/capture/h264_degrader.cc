#include <iostream>
#include <memory>
#include <cstring>

#include "h264_degrader.hh"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

H264_degrader::H264_degrader(size_t _width, size_t _height) :
    width(_width),
    height(_height),
    frame_count(0)
{

    avcodec_register_all();

    encoder_codec = avcodec_find_encoder(codec_id);
    if(encoder_codec == NULL){
        std::cout << "encoder_codec: " << codec_id << " not found!" << "\n";
        throw;
    }
    
    decoder_codec = avcodec_find_decoder(codec_id);
    if(decoder_codec == NULL){
        std::cout << "decoder_codec: " << codec_id << " not found!" << "\n";
        throw;
    }

    encoder_context = avcodec_alloc_context3(encoder_codec);
    if(encoder_context == NULL){
        std::cout << "encoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }

    AVCodecContext *decoder_context = avcodec_alloc_context3(decoder_codec);
    if(decoder_context == NULL){
        std::cout << "decoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }

    encoder_context->pix_fmt = pix_fmt;
    encoder_context->width = width;
    encoder_context->height = height;
    encoder_context->time_base = (AVRational){1, 25};
    encoder_context->framerate = (AVRational){25, 1};
    encoder_context->gop_size = 10;
    encoder_context->max_b_frames = 1;
    encoder_context->bit_rate = 400000;
    
    decoder_context->pix_fmt = pix_fmt;
    decoder_context->width = width;
    decoder_context->height = height;
    decoder_context->time_base = (AVRational){1, 25};
    decoder_context->framerate = (AVRational){25, 1};
    decoder_context->gop_size = 10;
    decoder_context->max_b_frames = 1;
    decoder_context->bit_rate = 400000;

    if(avcodec_open2(encoder_context, encoder_codec, NULL) < 0){
        std::cout << "could not open encoder" << "\n";;
        throw;
    }
    
    if(avcodec_open2(decoder_context, decoder_codec, NULL) < 0){
        std::cout << "could not open decoder" << "\n";;
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
    frame->pts = 0;

    if(av_frame_get_buffer(frame, 32) < 0){
        std::cout << "AVFrame could not allocate buffer" << "\n";
        throw;
    }

    // for(size_t i = 0; i < 8; i++){
    //     std::cout << frame->linesize[i] << "\n";
    // }

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

    if(av_frame_make_writable(frame) < 0){
        std::cout << "Could not make the frame writable" << "\n";
        throw;
    }

    // copy frame into buffer
    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){
            frame->data[0][y*frame->linesize[0] + x] = input[0][y*width + x];
        }
    }
    for(size_t y = 0; y < height/2; y++){
        for(size_t x = 0; x < width/2; x++){
            frame->data[1][y*frame->linesize[1] + x] = input[1][y*width + x];
            frame->data[2][y*frame->linesize[2] + x] = input[2][y*width + x];
        }
    }

    // encode frame
    frame->pts = frame_count;
    int ret = avcodec_send_frame(encoder_context, frame);
    if (ret < 0) {
        std::cout << "error sending a frame for encoding" << "\n";
        throw;
    }
    frame_count += 1;

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder_context, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            continue;
        }
        else if (ret < 0) {
            std::cout << "error during encoding" << "\n";
            throw;
        }

        // decode frame
        // TODO
    }


    std::memcpy(output[0], input[0], width*height);
    std::memcpy(output[1], input[1], width*height/2);
    std::memcpy(output[2], input[2], width*height/2);
}

