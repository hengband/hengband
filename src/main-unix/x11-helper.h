/*!
 * @brief X11環境におけるヘルパー関数定義
 * @date 2026/09/23
 * @author Hourier
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <tl/optional.hpp>
#include <vector>
#ifdef USE_X11
#include <X11/Xlib.h>
#ifdef USE_XFT
#include <X11/Xft/Xft.h>
XftColor create_pixel(Display *dpy, uint8_t red, uint8_t green, uint8_t blue);
#else
extern bool smoothRescaling;
unsigned long create_pixel(Display *dpy, uint8_t red, uint8_t green, uint8_t blue);
XImage *ReadBMP(Display *dpy, char *Name);
XImage *ResizeImage(Display *dpy, XImage *Im, int ix, int iy, int ox, int oy);
#ifdef HAVE_LIBPNG
/*!
 * @brief PNG から読み込んだ画像 (画素は 0x00RRGGBB、上の行から順)
 */
struct RgbImage {
    int width;
    int height;
    std::vector<uint32_t> pixels;
};

tl::optional<RgbImage> read_png(const std::filesystem::path &path);
#endif
#endif
#endif

/*
 * Checks if the keysym is a special key or a normal key
 * Assume that XK_MISCELLANY keysyms are special
 *
 * Also appears in "main-x11.c".
 */
#define IsSpecialKey(keysym) ((unsigned)(keysym) >= 0xFF00)
