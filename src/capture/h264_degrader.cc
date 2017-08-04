#include <iostream>
#include <memory>
#include <cstring>
#include <vector>

#include "h264_degrader.hh"

#define PIX(x) (x < 0 ? 0 : (x > 255 ? 255 : x))

extern "C" {
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

void H264_degrader::bgra2yuv422p(uint8_t* input, uint8_t** output, size_t width, size_t height){
    // https://www.fourcc.org/fccyvrgb.php

    // Y-plane
    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){
            size_t offset = 4*(y*width + x);
            uint16_t b = input[offset + 0];
            uint16_t g = input[offset + 1];
            uint16_t r = input[offset + 2];

            uint16_t Y = 0.098*b + 0.504*g + 0.257*r + 16;
            output[0][y*width + x] = PIX(Y);
        }
    }

    // U and Y planes
    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x+=2){
            size_t offset = 4*(y*width + x);
            int16_t b1 = input[offset + 0];
            int16_t g1 = input[offset + 1];
            int16_t r1 = input[offset + 2];

            int16_t b2 = input[offset + 4];
            int16_t g2 = input[offset + 5];
            int16_t r2 = input[offset + 6];

            int16_t cb1 = 0.439*b1 - 0.291*g1 - 0.148*r1 + 128;
            int16_t cb2 = 0.439*b2 - 0.291*g2 - 0.148*r2 + 128;
            int16_t cb = (cb1 + cb2) / 2;
            output[1][y*width/2 + x/2] = PIX(cb); 

            int16_t cr1 = -0.071*b1 - 0.368*g1 + .439*r1 + 128;
            int16_t cr2 = -0.071*b2 - 0.368*g2 + .439*r2 + 128;
            int16_t cr = (cr1 + cr2) / 2;
            output[2][y*width/2 + x/2] = PIX(cr);
        }
    }
}

void H264_degrader::yuv422p2bgra(uint8_t** input, uint8_t* output, size_t width, size_t height){
    // https://www.fourcc.org/fccyvrgb.php

    // BGRA-packed
    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){
            size_t offset = 4*(y*width + x);

            int16_t Y = input[0][y*width + x];
            int16_t u = input[1][y*width/2 + x/2];
            int16_t v = input[2][y*width/2 + x/2];
            
            int16_t b = 1.164*(Y-16) + 2.018*(u-128);
            output[offset] = PIX(b);

            int16_t g = 1.164*(Y-16) - 0.391*(u-128) - 0.813*(v-128);
            output[offset + 1] = PIX(g);

            int16_t r = 1.164*(Y-16) + 1.596*(v-128);
            output[offset + 2] = PIX(r);

            output[offset + 3] = 255;
        }
    }
}

H264_degrader::H264_degrader(size_t _width, size_t _height, size_t _bitrate) :
    width(_width),
    height(_height),
    bitrate(_bitrate),
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

    decoder_context = avcodec_alloc_context3(decoder_codec);
    if(decoder_context == NULL){
        std::cout << "decoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }

    // encoder context parameter
    encoder_context->pix_fmt = pix_fmt;
    encoder_context->width = width;
    encoder_context->height = height;

    encoder_context->bit_rate = bitrate;
    encoder_context->bit_rate_tolerance = 0;

    encoder_context->time_base = (AVRational){1, 20};
    encoder_context->framerate = (AVRational){60, 1};
    encoder_context->gop_size = 0;
    encoder_context->max_b_frames = 0;
    encoder_context->qmin = 0;
    encoder_context->qmax = 64;
    encoder_context->qcompress = 0.5;
    av_opt_set(encoder_context->priv_data, "tune", "zerolatency", 0); // forces no frame buffer delay (https://stackoverflow.com/questions/10155099/c-ffmpeg-h264-creating-zero-delay-stream)

    // decoder context parameter
    decoder_context->pix_fmt = pix_fmt;
    decoder_context->width = width;
    decoder_context->height = height;

    decoder_context->bit_rate = encoder_context->bit_rate;
    encoder_context->bit_rate_tolerance = encoder_context->bit_rate_tolerance;

    decoder_context->time_base = encoder_context->time_base;
    decoder_context->framerate = encoder_context->framerate;
    decoder_context->gop_size = encoder_context->gop_size;
    decoder_context->max_b_frames = encoder_context->max_b_frames;
    decoder_context->qmin = encoder_context->qmin;
    decoder_context->qmax = encoder_context->qmax;
    decoder_context->qcompress = encoder_context->qcompress;

    if(avcodec_open2(encoder_context, encoder_codec, NULL) < 0){
        std::cout << "could not open encoder" << "\n";;
        throw;
    }
    
    if(avcodec_open2(decoder_context, decoder_codec, NULL) < 0){
        std::cout << "could not open decoder" << "\n";;
        throw;
    }

    decoder_parser = av_parser_init(decoder_codec->id);
    if(decoder_parser == NULL){
        std::cout << "Decoder parser could not be initialized" << "\n";
        throw;
    }

    encoder_frame = av_frame_alloc();
    if(encoder_frame == NULL) {
        std::cout << "AVFrame not allocated: encoder" << "\n";
        throw;
    }

    decoder_frame = av_frame_alloc();
    if(decoder_frame == NULL) {
        std::cout << "AVFrame not allocated: decoder" << "\n";
        throw;
    }
    
    encoder_frame->width = width;
    encoder_frame->height = height;
    encoder_frame->format = pix_fmt;
    encoder_frame->pts = 0;

    decoder_frame->width = width;
    decoder_frame->height = height;
    decoder_frame->format = pix_fmt;
    decoder_frame->pts = 0;

    if(av_frame_get_buffer(encoder_frame, 32) < 0){
        std::cout << "AVFrame could not allocate buffer: encoder" << "\n";
        throw;
    }

    if(av_frame_get_buffer(decoder_frame, 32) < 0){
        std::cout << "AVFrame could not allocate buffer: decoder" << "\n";
        throw;
    }

    encoder_packet = av_packet_alloc();
    if(encoder_packet == NULL) {
        std::cout << "AVPacket not allocated: encoder" << "\n";
        throw;
    }

    decoder_packet = av_packet_alloc();
    if(decoder_packet == NULL) {
        std::cout << "AVPacket not allocated: decoder" << "\n";
        throw;
    }
}

H264_degrader::~H264_degrader(){
    av_parser_close(decoder_parser);

    avcodec_free_context(&decoder_context);
    avcodec_free_context(&encoder_context);

    av_frame_free(&decoder_frame);
    av_frame_free(&encoder_frame);

    av_packet_free(&decoder_packet);
    av_packet_free(&encoder_packet);
}

void H264_degrader::degrade(uint8_t **input, uint8_t **output){
    
    bool output_set = false;

    if(av_frame_make_writable(encoder_frame) < 0){
        std::cout << "Could not make the frame writable" << "\n";
        throw;
    }

    // copy frame into buffer 
    // TODO(jremons) make faster
    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){
            encoder_frame->data[0][y*encoder_frame->linesize[0] + x] = input[0][y*width + x];
        }
    }
    for(size_t y = 0; y < height/2; y++){
        for(size_t x = 0; x < width/2; x++){
            encoder_frame->data[1][y*encoder_frame->linesize[1] + x] = input[1][y*width + x];
            encoder_frame->data[2][y*encoder_frame->linesize[2] + x] = input[2][y*width + x];
        }
    }

    // encode frame
    encoder_frame->pts = frame_count;
    int ret = avcodec_send_frame(encoder_context, encoder_frame);
    if (ret < 0) {
        std::cout << "error sending a frame for encoding" << "\n";
        throw;
    }
    
    std::shared_ptr<uint8_t> buffer;
    int buffer_size = 0;
    int count = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder_context, encoder_packet);

        if (ret == AVERROR(EAGAIN)){
            break;
            std::cout << "did I make it here?\n";
        }
        else if(ret == AVERROR_EOF){
            break;
        }
        else if (ret < 0) {
            std::cout << "error during encoding" << "\n";
            throw;
        }

        // TODO(jremmons) memory allocation is slow
        buffer = std::move(std::shared_ptr<uint8_t>(new uint8_t[encoder_packet->size + AV_INPUT_BUFFER_PADDING_SIZE]));
        buffer_size = encoder_packet->size;
        std::memcpy(buffer.get(), encoder_packet->data, encoder_packet->size);
        
        if(count > 0){
            std::cout << "error! multiple parsing passing!" << "\n";
            throw;
        }
        count += 1;
    }
    av_packet_unref(encoder_packet);
    
    // decode frame
    uint8_t *data = buffer.get();
    int data_size = buffer_size;
    while(data_size > 0){
        size_t ret1 = av_parser_parse2(decoder_parser,
                                       decoder_context, 
                                       &decoder_packet->data, 
                                       &decoder_packet->size,
                                       data,
                                       data_size,
                                       AV_NOPTS_VALUE, 
                                       AV_NOPTS_VALUE, 
                                       0);
        
        if(ret1 < 0){
            std::cout << "error while parsing the buffer: decoding" << "\n";
            throw;
        }

        data += ret1;
        data_size -= ret1;

        if(decoder_packet->size > 0){
            if(avcodec_send_packet(decoder_context, decoder_packet) < 0){
                std::cout << "error while decoding the buffer: send_packet" << "\n";
                throw;                
            }

            size_t ret2 = avcodec_receive_frame(decoder_context, decoder_frame);
            if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF){
                continue;
            }
            else if (ret2 < 0) {
                std::cout << "error during decoding: receive frame" << "\n";
                throw;
            }
            
            // copy output in output_buffer
            // TODO(jremmons) make faster
            for(size_t y = 0; y < height; y++){
                for(size_t x = 0; x < width; x++){
                    output[0][y*width + x] = decoder_frame->data[0][y*decoder_frame->linesize[0] + x];
                }
            }
            for(size_t y = 0; y < height/2; y++){
                for(size_t x = 0; x < width/2; x++){
                    output[1][y*width + x] = decoder_frame->data[1][y*decoder_frame->linesize[1] + x];
                    output[2][y*width + x] = decoder_frame->data[2][y*decoder_frame->linesize[2] + x];
                }
            }
            output_set = true;
        }
    }
    av_packet_unref(decoder_packet);

    if(!output_set){
        // make white if output not set
        std::memset(output[0], 255, width*height);
        std::memset(output[1], 128, width*height/2);
        std::memset(output[2], 128, width*height/2);
    }

    frame_count += 1;
}

