/*
 * https://raw.github.com/dmitrysmagin/snes9x4d-rzx50/master/dingux-sdl/scaler.cpp
 */

#include "scaler.h"

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

/*
 * Approximately bilinear scaler, 256x224 to 320x240
 *
 * Copyright (C) 2014 hi-ban, Nebuleon <nebuleon.fumika@gmail.com>
 *
 * This function and all auxiliary functions are free software; you can
 * redistribute them and/or modify them under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * These functions are distributed in the hope that they will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Support math
#define Half(A) (((A) >> 1) & 0x7BEF)
#define Quarter(A) (((A) >> 2) & 0x39E7)
// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x0821)
#define RestQuarter(A) ((A) & 0x1863)

// Error correction expressions for quarters of pixels
#define Corr1_3(A, B)     Quarter(RestQuarter(A) + (RestHalf(B) << 1) + RestQuarter(B))
#define Corr3_1(A, B)     Quarter((RestHalf(A) << 1) + RestQuarter(A) + RestQuarter(B))

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x0821)

// Quarters
#define Weight1_3(A, B)   (Quarter(A) + Half(B) + Quarter(B) + Corr1_3(A, B))
#define Weight3_1(A, B)   (Half(A) + Quarter(A) + Quarter(B) + Corr3_1(A, B))

// Halves
#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

#define cR(A) (((A) & 0xf800) >> 8)
#define cG(A) (((A) & 0x7e0) >> 3)
#define cB(A) (((A) & 0x1f) << 3)

#define Weight2_1(A, B)  ((((cR(A) + cR(A) + cR(B)) / 3) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(B)) / 3) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(B)) / 3) & 0xf8) >> 3)


uint16_t hexcolor_to_rgb565(const uint32_t color)
{
    uint8_t colorr = ((color >> 16) & 0xFF);
    uint8_t colorg = ((color >> 8) & 0xFF);
    uint8_t colorb = ((color) & 0xFF);

    uint16_t r = ((colorr >> 3) & 0x1f) << 11;
    uint16_t g = ((colorg >> 2) & 0x3f) << 5;
    uint16_t b = (colorb >> 3) & 0x1f;

    return (uint16_t) (r | g | b);
}

/* Upscales a 256x224 image to 320x240 using an approximate bilinear
 * resampling algorithm that only uses integer math.
 *
 * Input:
 *   src: A packed 256x224 pixel image. The pixel format of this image is
 *     RGB 565.
 *   width: The width of the source image. Should always be 256.
 * Output:
 *   dst: A packed 320x240 pixel image. The pixel format of this image is
 *     RGB 565.
 */

/*
    Upscale 256x224 -> 320x240

    Horizontal upscale:
        320/256=1.25  --  do some horizontal interpolation
        8p -> 10p
        4dw -> 5dw

        coarse interpolation:
        [ab][cd][ef][gh] -> [ab][(bc)c][de][f(fg)][gh]

        fine interpolation
        [ab][cd][ef][gh] -> [a(0.25a+0.75b)][(0.5b+0.5c)(0.75c+0.25d)][de][(0.25e+0.75f)(0.5f+0.5g)][(0.75g+0.25h)h]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 320x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
*/

/*
void upscale_256x240_to_320x240(uint32_t *dst,
                                uint32_t *src,
                                int width) {
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;
    for (y = 0; y < 239; y++) {
        source = dh * width / 2;

        for (x = 0; x < 320/10; x++)
        {
            register uint32_t ab, cd, ef, gh;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + width/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + width/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
                ef = AVERAGE(ef, src[source + width/2 + 2]) & 0xF7DEF7DE; // to prevent overflow
                gh = AVERAGE(gh, src[source + width/2 + 3]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = ab;
            *dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + (cd << 16);
            *dst++  = (cd >> 16) + (ef << 16);
            *dst++  = (ef >> 16) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
            *dst++  = gh;

            source += 4;

        }
        Eh += 224; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}
*/


/*
    Upscale 256x224 -> 384x240 (for 400x240)

    Horizontal interpolation
        384/256=1.5
        4p -> 6p
        2dw -> 3dw

        for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
        [ab][cd] => [a(ab)][bc][(cd)d]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 400x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction is made
*/

/*
    Upscale 256x224 -> 384x272 (for 480x240)

    Horizontal interpolation
        384/256=1.5
        4p -> 6p
        2dw -> 3dw

        for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
        [ab][cd] => [a(ab)][bc][(cd)d]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 480x272x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction is made
*/


void upscale_256x240_to_512x480(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = a;
            *(BlockDst +  512 + 1)  = a;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x480_scanline(uint32_t *dst, uint32_t *src, int width) {

}
void upscale_256x240_to_512x480_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x480_grid(uint32_t *dst, uint32_t *src, int width) {

}

void upscale_256x240_to_512x480_grid(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( color, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (BlockX + 1) >= 256 ? 0 : (BlockX + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color;
            *(BlockDst + 1) = scanline_color;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}


void upscale_256x224_to_512x448(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = a;
            *(BlockDst +  512 + 1)  = a;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x448_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}


void upscale_256x224_to_512x448_grid(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color = *(BlockSrc);
            uint16_t  scanline_color = Weight3_1(color, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (x + 1) >= GBC_SCREEN_WIDTH ? 0 : (x + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));
            // -- Row 1 --
            *(BlockDst) = color;
            *(BlockDst + 1) = scanline_color;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_256x448_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 256 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            //*(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  256 )  = scanline_color;
            //*(BlockDst +  256 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 1;
        }
    }
}

void upscale_256x240_to_256x480_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 256 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( color, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (BlockX + 1) >= 256 ? 0 : (BlockX + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color;
            // -- next row 2 --
            *(BlockDst +  256 )  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}


void upscale_512x240_to_512x480_scanline(uint32_t *dst,
                                           uint32_t *src,
                                           int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 512 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 512; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)
             * (scanline)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            BlockSrc += 1;
            BlockDst += 1;
        }
    }
}

void upscale_512x224_to_512x448_scanline(uint32_t *dst,
                                         uint32_t *src,
                                         int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 224; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 512 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 512; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)
             * (scanline)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            BlockSrc += 1;
            BlockDst += 1;
        }
    }
}

void upscale_512x224_to_512x448_grid(uint32_t *dst,
                                       uint32_t *src,
                                       int width) {


    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 224; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 512 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 128; BlockX ++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */

            //one
            uint16_t  color1 = *(BlockSrc);
            uint16_t  color2 = *(BlockSrc + 1);
            uint16_t  color3 = *(BlockSrc + 2);
            uint16_t  color4 = *(BlockSrc + 3);

            uint16_t  nextRowScanLine_color1 = Weight3_1(color1, gcolor);
            uint16_t  nextRowScanLine_color2 = Weight3_1(color2, gcolor);

            uint16_t  nextRowScanLine_color3 = Weight3_1(color3, gcolor);
            uint16_t  nextRowScanLine_color4 = Weight3_1(color4, gcolor);

            uint16_t  scanline_color2 = Weight3_1(color2, gcolor);
            uint16_t  scanline_color4 = Weight3_1(color4, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (x + 1) >= GBC_SCREEN_WIDTH ? 0 : (x + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color1;
            *(BlockDst + 1) = color2;

            // -- next row 2 --
            *(BlockDst +  512 )  = nextRowScanLine_color1;
            *(BlockDst +  512 + 1)  = nextRowScanLine_color2;

            *(BlockDst + 2) = scanline_color2;
            *(BlockDst + 3) = scanline_color4;

            // -- next row 2 --
            *(BlockDst +  512 + 2)  = nextRowScanLine_color3;
            *(BlockDst +  512 + 3)  = nextRowScanLine_color4;


            BlockSrc += 4;
            BlockDst += 4;
        }
    }
}

void upscale_512x240_to_512x480_grid(uint32_t *dst,
                                     uint32_t *src,
                                     int width) {

    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 512 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 128; BlockX ++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */

            //one
            uint16_t  color1 = *(BlockSrc);
            uint16_t  color2 = *(BlockSrc + 1);
            uint16_t  color3 = *(BlockSrc + 2);
            uint16_t  color4 = *(BlockSrc + 3);

            uint16_t  nextRowScanLine_color1 = Weight3_1(color1, gcolor);
            uint16_t  nextRowScanLine_color2 = Weight3_1(color2, gcolor);

            uint16_t  nextRowScanLine_color3 = Weight3_1(color3, gcolor);
            uint16_t  nextRowScanLine_color4 = Weight3_1(color4, gcolor);

            uint16_t  scanline_color2 = Weight3_1(color2, gcolor);
            uint16_t  scanline_color4 = Weight3_1(color4, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (x + 1) >= GBC_SCREEN_WIDTH ? 0 : (x + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color1;
            *(BlockDst + 1) = color2;

            // -- next row 2 --
            *(BlockDst +  512 )  = nextRowScanLine_color1;
            *(BlockDst +  512 + 1)  = nextRowScanLine_color2;

            *(BlockDst + 2) = scanline_color2;
            *(BlockDst + 3) = scanline_color4;

            // -- next row 2 --
            *(BlockDst +  512 + 2)  = nextRowScanLine_color3;
            *(BlockDst +  512 + 3)  = nextRowScanLine_color4;


            BlockSrc += 4;
            BlockDst += 4;
        }
    }
}
