#include <iostream>
#include <memory>
#include <cstring>
#include <vector>

#include "h264_degrader.hh"

extern "C" {
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
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

    // if((decoder_codec->capabilities)&CODEC_CAP_TRUNCATED){
    //     (decoder_context->flags) |= CODEC_FLAG_TRUNCATED;
    // }

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

    encoder_context->bit_rate = 4000;
    encoder_context->bit_rate_tolerance = 0;

    encoder_context->time_base = (AVRational){1, 50};
    encoder_context->framerate = (AVRational){25, 1};
    encoder_context->gop_size = 10;
    encoder_context->max_b_frames = 1;
    encoder_context->qmin = 0;
    encoder_context->qmax = 64;
    encoder_context->qcompress = 0.5;

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

    // for(size_t i = 0; i < 8; i++){
    //     std::cout << frame->linesize[i] << "\n";
    // }

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
    // TODO
}

void H264_degrader::degrade(uint8_t **input, uint8_t **output, bool &output_set){
    
    std::memset(output[0], 255, width*height);
    std::memset(output[1], 128, width*height/2);
    std::memset(output[2], 128, width*height/2);
    output_set = false;

    if(av_frame_make_writable(encoder_frame) < 0){
        std::cout << "Could not make the frame writable" << "\n";
        throw;
    }

    // copy frame into buffer
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
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }
        else if (ret < 0) {
            std::cout << "error during encoding" << "\n";
            throw;
        }

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
                                       frame_count);
        
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

    frame_count += 1;
}

