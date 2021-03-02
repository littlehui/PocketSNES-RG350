/*
 * https://raw.github.com/dmitrysmagin/snes9x4d-rzx50/master/dingux-sdl/scaler.h
 */

#include <stdint.h>

//void upscale_256x224_to_320x240(uint32_t *dst, uint32_t *src, int width);
//void upscale_256x224_to_384x240_for_400x240(uint32_t *dst, uint32_t *src, int width);
//void upscale_256x224_to_384x272_for_480x272(uint32_t *dst, uint32_t *src, int width);

//extern void (*upscale_p)(uint32_t *dst, uint32_t *src, int width);
//extern void upscale_256x240_to_320x240(uint32_t *dst, uint32_t *src, int width);
//extern void upscale_256x224_to_320x240_bilinearish(uint32_t *dst, uint32_t *src, int width);
//extern void upscale_256x240_to_320x240_bilinearish(uint32_t* dst, uint32_t* src, int width);

//extern void upscale_256x240_to_640x480_bilinearish(uint32_t* dst, uint32_t* src, int width);
//extern void upscale_256x224_to_640x480_bilinearish(uint32_t *dst, uint32_t *src, int width);

//void upscale_256x224_to_512x480(uint32_t *dst, uint32_t *src, int width);
void upscale_256x240_to_512x480(uint32_t *dst, uint32_t *src, int width);

void upscale_256x224_to_512x480_scanline(uint32_t *dst, uint32_t *src, int width);
void upscale_256x240_to_512x480_scanline(uint32_t *dst, uint32_t *src, int width);


void upscale_256x224_to_512x480_grid(uint32_t *dst, uint32_t *src, int width);
void upscale_256x240_to_512x480_grid(uint32_t *dst, uint32_t *src, int width);

void upscale_256x224_to_512x448(uint32_t *dst, uint32_t *src, int width);
void upscale_256x224_to_512x448_scanline(uint32_t *dst, uint32_t *src, int width);
void upscale_256x224_to_512x448_grid(uint32_t *dst, uint32_t *src, int width);
//void upscale_256x224_to_512x448_scanline_v(uint32_t *dst, uint32_t *src, int width);

//void upscale_256x224_to_512x448_grid1(uint32_t *dst, uint32_t *src, int width);


void upscale_256x224_to_256x448_scanline(uint32_t *dst, uint32_t *src, int width);
void upscale_256x240_to_256x480_scanline(uint32_t *dst, uint32_t *src, int width);

//void downscale_512x240_to_320x240(uint32_t *dst,uint32_t *src,int width) ;
//void downscale_512x224_to_320x240(uint32_t *dst,uint32_t *src,int width);
//void downscale_512x224_to_320x240_bilinearish(uint32_t *dst,uint32_t *src,int width);
//void downscale_512x240_to_320x240_bilinearish(uint32_t *dst,uint32_t *src,int width);

void upscale_512x240_to_512x480_scanline(uint32_t *dst,
                                  uint32_t *src,
                                  int width) ;
void upscale_512x224_to_512x448_scanline(uint32_t *dst,
                                         uint32_t *src,
                                         int width) ;

void upscale_512x224_to_512x448_grid(uint32_t *dst,
                                  uint32_t *src,
                                  int width);
void upscale_512x240_to_512x480_grid(uint32_t *dst,
                                     uint32_t *src,
                                     int width);