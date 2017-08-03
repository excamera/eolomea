#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

#include "h264_degrader.hh"

int main(int argc, char **argv)
{
    if(argc != 3){
        std::cout << "usage: " << argv[0] << " <input.raw> <ouptut.raw>\n";
        return 0;
    }

    const std::string input_filename = argv[1];
    const std::string output_filename = argv[2];

    std::cout << "input: " << input_filename << "\n";
    std::cout << "ouput: " << output_filename << "\n";    

    const size_t width = 1280;
    const size_t height = 720;
    const size_t bytes_per_pixel = 4;
    const size_t frame_size = width*height*bytes_per_pixel;

    std::ifstream infile(input_filename, std::ios::binary);
    if(!infile.is_open()){
        std::cout << "Could not open file: " << input_filename << "\n";
        return 0;
    }
    
    std::ofstream outfile(output_filename, std::ios::binary);
    if(!outfile.is_open()){
        std::cout << "Could not open file: " << input_filename << "\n";
        return 0;
    }

    std::shared_ptr<uint8_t> input_buffer(new uint8_t[frame_size]);
    std::shared_ptr<uint8_t> output_buffer(new uint8_t[frame_size]);
    
    H264_degrader degrader(width, height);

    while(!infile.eof()){
        infile.read((char*)input_buffer.get(), frame_size);
        
        // degrade
        degrader.degrade(input_buffer.get(), output_buffer.get());

        outfile.write((char*)output_buffer.get(), frame_size);
    }

    return 0;
}
/*
int main(int argc, char *argv[]){
    
    if(argc != 3){
        std::cout << "usage: " << argv[0] << " <input.mp4> <ouptut.raw>\n";
        return 0;
    }

    const std::string input_filename = argv[1];
    const std::string output_filename = argv[2];

    std::cout << "input: " << input_filename << "\n";
    std::cout << "ouput: " << output_filename << "\n";
    
    avcodec_register_all();

    const std::string codec_name = "h264";
    AVCodec *codec = avcodec_find_decoder_by_name(codec_name.c_str());
    if(codec == NULL){
        std::cout << "codec: " << codec_name << " not found!" << "\n";
        return EXIT_FAILURE;
    }
    
    AVCodecContext *context = avcodec_alloc_context3(codec);
    if(context == NULL){
        std::cout << "context: " << codec_name << " not found!" << "\n";
        return EXIT_FAILURE;
    }
    
    context->width = 640;
    context->height = 480;

    if(avcodec_open2(context, codec, NULL) < 0){
        std::cout << "could not open avcodec" << "\n";;
        return EXIT_FAILURE;
    }
    
    AVFrame *frame = av_frame_alloc();
    if(frame == NULL) {
        std::cout << "AVFrame not allocated" << "\n";
        return EXIT_FAILURE;        
    }
    
    AVPacket *packet = av_packet_alloc();
    if(packet == NULL) {
        std::cout << "AVPacket not allocated" << "\n";
        return EXIT_FAILURE;        
    }

    AVCodecParserContext *parser = av_parser_init(codec->id);
    if(parser == NULL){
        std::cout << "AVParser not allocated" << "\n";
        return EXIT_FAILURE;
    }
    
    // begin decoding
    FILE *infile = fopen(input_filename.c_str(), "rb");
    if(infile == NULL){
        std::cout << "Could not open " << input_filename << "\n";
        return EXIT_FAILURE;        
    }
    FILE *outfile = fopen(output_filename.c_str(), "wb");
    if(outfile == NULL){
        std::cout << "Could not open " << output_filename << "\n";
        return EXIT_FAILURE;        
    }


    // decoding loop
    std::cout << "starting decode\n";

    uint8_t buffer[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    while(!feof(infile)){
        
        data_size = fread(buffer, 1, INBUF_SIZE, infile);
        if(data_size == 0){
            break;
        }
        
        data = buffer;
        while(data_size > 0){
            size_t bytes_consumed = av_parser_parse2(parser, context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            
            data += bytes_consumed;
            data_size -= bytes_consumed;
            
            

        }

    }

    std::cout << "ending decode\n";

    if(fclose(infile) != 0){
        std::cout << "Could not close " << input_filename;
        return EXIT_FAILURE;                
    }

    if(fclose(outfile) != 0){
        std::cout << "Could not close " << output_filename;
        return EXIT_FAILURE;                
    }
    

    return 0;
    

}
*/
