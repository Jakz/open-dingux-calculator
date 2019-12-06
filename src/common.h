#pragma once

#include <cstdint>
#include <algorithm>

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

using s32 = int32_t;
using s64 = int64_t;


#ifdef _WIN32
#define MOUSE_ENABLED true
#define WINDOW_SCALE 1
#else
#define MOUSE_ENABLED false
#endif

union color_t
{
  struct { u8 b, g, r, a; };
  u32 data;

  inline color_t(u32 argb) : data(argb) { }
  inline color_t(u32 r, u32 g, u32 b, u32 a) : r(r), g(g), b(b), a(a) { }
  inline color_t(u32 r, u32 g, u32 b) : r(r), g(g), b(b), a(255) { }
  inline color_t& operator=(u32 data) { this->data = data; return *this; }
  void setRGB(color_t rgb) { data = (data & 0xff000000) | (rgb.data & 0x00ffffff); }
  void blendAdd(color_t rgb, u32 alpha) {
    r = std::min(r + (u32)(rgb.r * alpha / 255.0f), 255U);
    g = std::min(g + (u32)(rgb.g * alpha / 255.0f), 255U);
    b = std::min(b + (u32)(rgb.b * alpha / 255.0f), 255U);
    a = 255;
  }

  inline static color_t black() { return color_t(0xff000000); }
  inline static color_t white() { return color_t(0xffffffff); }

};
