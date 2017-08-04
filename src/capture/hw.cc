#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

#include "h264_degrader.hh"

#define PIX(x) (x < 0 ? 0 : (x > 255 ? 255 : x))

void bgra2yuv422p(uint8_t* input, uint8_t** output, size_t width, size_t height){

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

void yuv422p2bgra(uint8_t** input, uint8_t* output, size_t width, size_t height){

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

    std::shared_ptr<uint8_t> y_buffer1(new uint8_t[frame_size/4]);
    std::shared_ptr<uint8_t> u_buffer1(new uint8_t[frame_size/8]);
    std::shared_ptr<uint8_t> v_buffer1(new uint8_t[frame_size/8]);
    std::shared_ptr<uint8_t> y_buffer2(new uint8_t[frame_size/4]);
    std::shared_ptr<uint8_t> u_buffer2(new uint8_t[frame_size/8]);
    std::shared_ptr<uint8_t> v_buffer2(new uint8_t[frame_size/8]);
    
    uint8_t *yuv_input[] = {y_buffer1.get(), u_buffer1.get(), v_buffer1.get()};
    uint8_t *yuv_output[] = {y_buffer2.get(), u_buffer2.get(), v_buffer2.get()};

    H264_degrader degrader(width, height);

    while(!infile.eof()){
        infile.read((char*)input_buffer.get(), frame_size);
        
        // convert to yuv422p
        bgra2yuv422p(input_buffer.get(), yuv_input, width, height);
        
        // degrade
        degrader.degrade(yuv_input, yuv_output);
 
        // convert to bgra
        yuv422p2bgra(yuv_output, output_buffer.get(), width, height);

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
