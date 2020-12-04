// Copyright (C) Trevor Sundberg, MIT License (see LICENSE.md)

// Reference:
// https://github.com/Thinkerfans/lib-mp4v2/blob/master/mp4v2/mp4record.c
// https://github.com/lieff/minih264/blob/master/minih264e_test.c
// https://stackoverflow.com/questions/9465815/rgb-to-yuv420-algorithm-efficiency

// #include <string.h>
// #include <stdlib.h>

#undef H264E_MAX_THREADS
#define H264E_MAX_THREADS 0

#define EMSCRIPTEN_SIMD_ENABLED

#ifdef EMSCRIPTEN_SIMD_ENABLED
  #define MINIH264_ONLY_SIMD 1
  #define H264E_ENABLE_NEON 0
#endif

#define MINIH264_IMPLEMENTATION
#define MINIMP4_IMPLEMENTATION

#include "minih264e.h"
#include "minimp4.h"

// #include "mp4v2/mp4v2.h"

#include "h264-mp4-encoder.h"

#define INITIALIZE_MESSAGE "Function initialize has not been called"

#define ALIGNED_ALLOC(n, size) aligned_alloc(n, (size + n - 1) / n * n)

#define TIMESCALE 90000

class H264MP4EncoderPrivate
{
public:
  H264MP4Encoder *encoder = nullptr;
  MP4E_mux_t *mux = nullptr;
  FILE *mp4 = nullptr;

  mp4_h26x_writer_t mp4wr;
  H264E_io_yuv_t yuv_planes;
  H264E_run_param_t run_param;
  
  int frame = 0;
  H264E_persist_t *enc = nullptr;
  H264E_scratch_t *scratch = nullptr;
  std::string rgba_to_yuv_buffer;

  static void nalu_callback(const uint8_t *nalu_data, int sizeof_nalu_data, void *token);
  static int write_callback(int64_t offset, const void *buffer, size_t size, void *token);
};

void H264MP4EncoderPrivate::nalu_callback(
    const uint8_t *nalu_data, int sizeof_nalu_data, void *token)
{
  H264MP4EncoderPrivate *encoder_private = (H264MP4EncoderPrivate *)token;
  H264MP4Encoder *encoder = encoder_private->encoder;

  uint8_t *data = const_cast<uint8_t *>(nalu_data - STARTCODE_4BYTES);
  const int nal_size = sizeof_nalu_data + STARTCODE_4BYTES;

  HME_CHECK_INTERNAL(nal_size >= 5);
  HME_CHECK_INTERNAL(data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1);

  HME_CHECK(MP4E_STATUS_OK == mp4_h26x_write_nal(&encoder_private->mp4wr, data, nal_size, TIMESCALE/encoder->frameRate), "error: mp4_h26x_write_nal failed");
}

int H264MP4EncoderPrivate::write_callback (int64_t offset, const void *buffer, size_t size, void *token)
{
  FILE *f = (FILE*)token;
  fseek(f, offset, SEEK_SET);
  size_t r = fwrite(buffer, 1, size, f);
  return r != size;
}

void H264MP4Encoder::initialize()
{
  HME_CHECK(!private_, "Cannot call initialize more than once without calling finalize");
  private_ = new H264MP4EncoderPrivate();
  HME_CHECK_INTERNAL(private_);
  private_->encoder = this;

  HME_CHECK_INTERNAL(!outputFilename.empty());
  HME_CHECK_INTERNAL(width > 0);
  HME_CHECK_INTERNAL(height > 0);
  HME_CHECK_INTERNAL(frameRate > 0);
  HME_CHECK_INTERNAL(quantizationParameter >= 10 && quantizationParameter <= 51);
  HME_CHECK_INTERNAL(speed <= 10);

  private_->mp4 = fopen(outputFilename.c_str(), "wb");

  int is_hevc = 0;
  private_->mux = MP4E_open(sequential, fragmentation, private_->mp4, &H264MP4EncoderPrivate::write_callback);
  HME_CHECK(MP4E_STATUS_OK == mp4_h26x_write_init(&private_->mp4wr, private_->mux, width, height, is_hevc), "error: mp4_h26x_write_init failed");

  H264E_create_param_t create_param;
  memset(&create_param, 0, sizeof(create_param));
  create_param.enableNEON = 1;
#if H264E_SVC_API
  create_param.num_layers = 1;
  create_param.inter_layer_pred_flag = 1;
  create_param.inter_layer_pred_flag = 0;
#endif
  create_param.gop = groupOfPictures;
  create_param.height = height;
  create_param.width = width;
  create_param.fine_rate_control_flag = 0;
  create_param.const_input_flag = 1;
  create_param.vbv_size_bytes = 100000 / 8;
  create_param.temporal_denoise_flag = temporalDenoise;

  int sizeof_persist = 0;
  int sizeof_scratch = 0;
  int sizeof_result = H264E_sizeof(&create_param, &sizeof_persist, &sizeof_scratch);
  HME_CHECK(sizeof_result != H264E_STATUS_SIZE_NOT_MULTIPLE_2, "Size must be a multiple of 2");
  HME_CHECK_INTERNAL(sizeof_result == H264E_STATUS_SUCCESS);
#if PRINTF_ENABLED
  if (debug)
  {
    printf("sizeof_persist=%d, sizeof_scratch=%d\n", sizeof_persist, sizeof_scratch);
  }
#endif

  private_->enc = (H264E_persist_t *)ALIGNED_ALLOC(64, sizeof_persist);
  private_->scratch = (H264E_scratch_t *)ALIGNED_ALLOC(64, sizeof_scratch);

  HME_CHECK_INTERNAL(H264E_init(private_->enc, &create_param) == H264E_STATUS_SUCCESS);

  memset(&private_->run_param, 0, sizeof(private_->run_param));
  private_->run_param.frame_type = 0;
  private_->run_param.encode_speed = speed;
  private_->run_param.desired_nalu_bytes = desiredNaluBytes;

  if (kbps)
  {
    private_->run_param.desired_frame_bytes = kbps * 1000 / 8 / frameRate;
    private_->run_param.qp_min = 10;
    private_->run_param.qp_max = 50;
  }
  else
  {
    private_->run_param.qp_min = private_->run_param.qp_max = quantizationParameter;
  }

  private_->run_param.nalu_callback_token = this->private_;
  private_->run_param.nalu_callback = &H264MP4EncoderPrivate::nalu_callback;

  private_->yuv_planes.stride[0] = width;
  private_->yuv_planes.stride[1] = width / 2;
  private_->yuv_planes.stride[2] = width / 2;

}

uint8_t* H264MP4Encoder::create_buffer(uint32_t length)
{
  // yuv = width * height * 3 / 2
  // rgb = width * height * 3
  return (uint8_t *)malloc(length * sizeof(uint8_t));
}

uintptr_t H264MP4Encoder::em_create_buffer(uint32_t length)
{
  uint8_t* p = create_buffer(length);
  return (uintptr_t)p;
}

void H264MP4Encoder::em_free_buffer(uintptr_t i)
{
  free_buffer(reinterpret_cast<uint8_t*>(i));
}

void H264MP4Encoder::free_buffer(uint8_t* p) {
  free(p);
}

void H264MP4Encoder::em_fast_encode_yuv(uintptr_t i)
{
  fast_encode_yuv(reinterpret_cast<uint8_t*>(i));
}

void H264MP4Encoder::fast_encode_yuv(uint8_t* yuv)
{
  private_->yuv_planes.yuv[0] = yuv;
  private_->yuv_planes.yuv[1] = yuv + width * height;
  private_->yuv_planes.yuv[2] = yuv + width * height * 5 / 4;

  int sizeof_coded_data = 0;
  uint8_t *coded_data = nullptr;
  HME_CHECK_INTERNAL(H264E_encode(
                         private_->enc,
                         private_->scratch,
                         &private_->run_param,
                         &private_->yuv_planes,
                         &coded_data,
                         &sizeof_coded_data) == H264E_STATUS_SUCCESS);
#if PRINTF_ENABLED
  if (debug)
  {
    printf("frame=%d, bytes=%d\n", private_->frame, sizeof_coded_data);
  }
#endif
  ++private_->frame;
}

void H264MP4Encoder::addFrameYuv(const std::string &yuv_buffer)
{
  HME_CHECK(private_, INITIALIZE_MESSAGE);
  HME_CHECK(yuv_buffer.size() == width * height * 3 / 2 /*YUV*/,
            "Incorrect buffer size for YUV (width * height * 3 / 2)");
  uint8_t *yuv = (uint8_t *)yuv_buffer.data();
  fast_encode_yuv(yuv);
}

void H264MP4Encoder::addFrameRgbPixels(const std::string &rgba_buffer, int stride)
{
  HME_CHECK(private_, INITIALIZE_MESSAGE);
  HME_CHECK(rgba_buffer.size() == width * height * stride /*RGB or RGBA*/,
            "Incorrect buffer size for RGBA (width * height * stride)");
  size_t yuv_size = width * height * 3 / 2;
  private_->rgba_to_yuv_buffer.resize(yuv_size);
  
  uint8_t *rgba = (uint8_t *)rgba_buffer.data();
  uint8_t *buffer = (uint8_t *)private_->rgba_to_yuv_buffer.data();

  size_t image_size = width * height;
  size_t upos = image_size;
  size_t vpos = upos + upos / 4;
  size_t i = 0;

  for (size_t line = 0; line < height; ++line)
  {
    if (!(line % 2))
    {
      for (size_t x = 0; x < width; x += 2)
      {
        uint8_t r = rgba[stride * i];
        uint8_t g = rgba[stride * i + 1];
        uint8_t b = rgba[stride * i + 2];
        buffer[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
        buffer[upos++] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
        buffer[vpos++] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;
        r = rgba[stride * i];
        g = rgba[stride * i + 1];
        b = rgba[stride * i + 2];
        buffer[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
      }
    }
    else
    {
      for (size_t x = 0; x < width; x += 1)
      {
        uint8_t r = rgba[stride * i];
        uint8_t g = rgba[stride * i + 1];
        uint8_t b = rgba[stride * i + 2];
        buffer[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
      }
    }
  }

  addFrameYuv(private_->rgba_to_yuv_buffer);
}

void H264MP4Encoder::addFrameRgba(const std::string &rgba_buffer)
{
  addFrameRgbPixels(rgba_buffer, 4);
}

void H264MP4Encoder::addFrameRgb(const std::string &rgb_buffer)
{
  addFrameRgbPixels(rgb_buffer, 3);
}

void H264MP4Encoder::finalize()
{
  HME_CHECK(private_, INITIALIZE_MESSAGE);
  MP4E_close(private_->mux);
  mp4_h26x_write_close(&private_->mp4wr);
  if (private_->mp4) fclose(private_->mp4);
  free(private_->enc);
  free(private_->scratch);
  delete private_;
  private_ = nullptr;
}

H264MP4Encoder::~H264MP4Encoder()
{
  HME_CHECK(private_ == nullptr, "Function finalize was not called before the encoder destructed");
}
