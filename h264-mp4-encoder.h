// Copyright (C) Trevor Sundberg, MIT License (see LICENSE.md)
#include <string>
#include <stdint.h>
#include <stdio.h>

#pragma once

class H264MP4EncoderPrivate;

typedef void (*H264MP4Encoder_data_callback)(
    void *userdata,
    const uint8_t *data,
    const uint32_t size);

#define PRINTF_ENABLED 1

#if PRINTF_ENABLED
#define HME_CHECK(expr, message)                                       \
  do                                                                   \
  {                                                                    \
    if (!(expr))                                                       \
    {                                                                  \
      printf("ERROR %s(%d): %s\n    EXPR: %s\n    FUNC: %s\n",         \
             __FILE__, __LINE__, message, #expr, __PRETTY_FUNCTION__); \
      abort();                                                         \
    }                                                                  \
  } while (false)
#else
#define HME_CHECK(expr, message)                                       \
  do                                                                   \
  {                                                                    \
    if (!(expr))                                                       \
    {                                                                  \
      abort();                                                         \
    }                                                                  \
  } while (false)
#endif

#define HME_CHECK_INTERNAL(expr) HME_CHECK(expr, "Internal error")

#define HME_PROPERTY(type, name, default_value)                     \
private:                                                            \
  type name = default_value;                                        \
                                                                    \
public:                                                             \
  const type &get_##name() const { return name; };                  \
  void set_##name(const type &value)                                \
  {                                                                 \
    HME_CHECK(!private_, "Cannot set properties after initialize"); \
    name = value;                                                   \
  };

class H264MP4Encoder
{
public:
  friend class H264MP4EncoderPrivate;

  HME_PROPERTY(std::string, outputFilename, "output.mp4");

  HME_PROPERTY(uint32_t, width, 0);

  HME_PROPERTY(uint32_t, height, 0);

  HME_PROPERTY(uint32_t, frameRate, 30);

  // The bitrate in kbps relative to the frame_rate. Overwrites quantization_parameter.
  HME_PROPERTY(uint32_t, kbps, 0);

  // Speed where 0 means best quality [0..10].
  HME_PROPERTY(uint32_t, speed, 0);

  // Higher means better compression, and lower means better quality [10..51].
  HME_PROPERTY(uint32_t, quantizationParameter, 33);

  // Key frame period.
  HME_PROPERTY(uint32_t, groupOfPictures, 20);

  // Use temporal noise supression.
  HME_PROPERTY(bool, temporalDenoise, false);

  // Each NAL unit will be approximately capped at this size (0 means unlimited).
  HME_PROPERTY(uint32_t, desiredNaluBytes, 0);

  HME_PROPERTY(bool, sequential, false);
  HME_PROPERTY(bool, fragmentation, false);

  // Prints extra debug information.
  HME_PROPERTY(bool, debug, false);

  void initialize();

  void addFrameYuv(const std::string &yuv_buffer);

  void addFrameRgbPixels(const std::string &rgba_buffer, int stride);
  void addFrameRgba(const std::string &rgba_buffer);
  void addFrameRgb(const std::string &rgb_buffer);

  void fast_encode_yuv(uint8_t* yuv);
  void em_fast_encode_yuv(uintptr_t i);

  uint8_t* create_buffer(uint32_t length);
  uintptr_t em_create_buffer(uint32_t length);

  void free_buffer(uint8_t* p);
  void em_free_buffer(uintptr_t i);

  void finalize();

  ~H264MP4Encoder();

private:
  H264MP4EncoderPrivate *private_ = nullptr;
};
