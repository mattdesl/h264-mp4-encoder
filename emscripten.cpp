#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "h264-mp4-encoder.h"

using namespace emscripten;


// uint8_t* create_yuv_buffer(uint32_t width, uint32_t height)
// {
//   return (uint8_t*)malloc(width * height * 3 / 2 * sizeof(uint8_t));
// }

// extern "C" int EMSCRIPTEN_KEEPALIVE myFunc(void *bufAddr, unsigned int size) {
//   // origFunc(static_cast<double *>(bufAddr), size);
//   return 0;
// }

EMSCRIPTEN_BINDINGS(H264MP4EncoderBinding)
{

  class_<H264MP4Encoder>("H264MP4Encoder")
      .constructor<>()

      .property("outputFilename", &H264MP4Encoder::get_outputFilename, &H264MP4Encoder::set_outputFilename)
      .property("width", &H264MP4Encoder::get_width, &H264MP4Encoder::set_width)
      .property("height", &H264MP4Encoder::get_height, &H264MP4Encoder::set_height)
      .property("frameRate", &H264MP4Encoder::get_frameRate, &H264MP4Encoder::set_frameRate)
      .property("kbps", &H264MP4Encoder::get_kbps, &H264MP4Encoder::set_kbps)
      .property("speed", &H264MP4Encoder::get_speed, &H264MP4Encoder::set_speed)
      .property("quantizationParameter", &H264MP4Encoder::get_quantizationParameter, &H264MP4Encoder::set_quantizationParameter)
      .property("groupOfPictures", &H264MP4Encoder::get_groupOfPictures, &H264MP4Encoder::set_groupOfPictures)
      .property("temporalDenoise", &H264MP4Encoder::get_temporalDenoise, &H264MP4Encoder::set_temporalDenoise)
      .property("desiredNaluBytes", &H264MP4Encoder::get_desiredNaluBytes, &H264MP4Encoder::set_desiredNaluBytes)
      .property("debug", &H264MP4Encoder::get_debug, &H264MP4Encoder::set_debug)

      .function("initialize", &H264MP4Encoder::initialize)
      .function("addFrameYuv", &H264MP4Encoder::addFrameYuv)
      .function("addFrameRgb", &H264MP4Encoder::addFrameRgb)
      .function("addFrameRgba", &H264MP4Encoder::addFrameRgba)
      .function("fast_encode_yuv", &H264MP4Encoder::em_fast_encode_yuv, allow_raw_pointers())
      .function("create_yuv_buffer", &H264MP4Encoder::em_create_yuv_buffer, allow_raw_pointers())
      .function("free_yuv_buffer", &H264MP4Encoder::em_free_yuv_buffer, allow_raw_pointers())
      .function("finalize", &H264MP4Encoder::finalize);
}


// &H264MP4Encoder::create_yuv_buffer, allow_raw_pointers()