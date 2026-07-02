#pragma once

#include <stdint.h>

// If we are in cpp
#ifdef __cplusplus
    #include "Image.hpp"

    // Single threaded, naive baseline implementation
    void otsu_baseline(ImageView<rgb8> in);

    // Single threaded version of the Otsu Method
    void otsu_st(ImageView<rgb8> in);

    // Multi threaded version of the Otsu Method
    void otsu_mt(ImageView<rgb8> in);

    extern "C" {
#endif

typedef enum {
    BASELINE = 0,
    ST = 1,
    MT = 2
} Method;


void set_otsu_method(Method m);

// C-wrapper to use with gstreamer
void otsu(uint8_t* buffer, int width, int height, int stride);
    

#ifdef __cplusplus
    }
#endif