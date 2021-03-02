#include <stdio.h>
#include <dirent.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <sys/time.h>
#include "sal.h"
#include "memmap.h"
#include <iostream>
#include "menu.h"

#define PALETTE_BUFFER_LENGTH    256*2*4
#define SNES_WIDTH  256
//#define SNES_HEIGHT 239


#define DINGUX_ALLOW_DOWNSCALING_FILE     "/sys/devices/platform/jz-lcd.0/allow_downscaling"
#define DINGUX_KEEP_ASPECT_RATIO_FILE     "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio"
#define DINGUX_INTEGER_SCALING_FILE       "/sys/devices/platform/jz-lcd.0/integer_scaling"
#define DINGUX_SHARPNESS_UPSCALING_FILE   "/sys/devices/platform/jz-lcd.0/sharpness_upscaling"
#define DINGUX_SHARPNESS_DOWNSCALING_FILE "/sys/devices/platform/jz-lcd.0/sharpness_downscaling"
#define DINGUX_BATTERY_CAPACITY_FILE      "/sys/class/power_supply/battery/capacity"

extern u16 IntermediateScreen[];
SDL_Surface *mScreen = NULL;
TTF_Font *mFont = NULL;
static u32 mSoundThreadFlag = 0;
static u32 mSoundLastCpuSpeed = 0;
static u32 mPaletteBuffer[PALETTE_BUFFER_LENGTH];
static u32 *mPaletteCurr = (u32 *) &mPaletteBuffer[0];
static u32 *mPaletteLast = (u32 *) &mPaletteBuffer[0];
static u32 *mPaletteEnd = (u32 *) &mPaletteBuffer[PALETTE_BUFFER_LENGTH];
static u32 mInputFirst = 0;

extern struct MENU_OPTIONS mMenuOptions;
s32 mCpuSpeedLookup[1] = {0};

#include <sal_common.h>

static u32 inputHeld[2] = {0, 0};
static SDL_Joystick *joy[2];

static u32 sal_Input(int held, u32 j)
{
    inputHeld[j] = 0;

    if (SDL_NumJoysticks() > 0) {
        int deadzone = 10000;
        if (joy[j] == NULL) joy[j] = SDL_JoystickOpen(j);

        SDL_JoystickUpdate();

        int joy_x = SDL_JoystickGetAxis(joy[j], 0);
        int joy_y = SDL_JoystickGetAxis(joy[j], 1);

        if (joy_x < -deadzone) inputHeld[j] |= SAL_INPUT_LEFT;
        else if (joy_x > deadzone) inputHeld[j] |= SAL_INPUT_RIGHT;

        if (joy_y < -deadzone) inputHeld[j] |= SAL_INPUT_UP;
        else if (joy_y > deadzone) inputHeld[j] |= SAL_INPUT_DOWN;

        if (SDL_JoystickGetButton(joy[j], 0)) inputHeld[j] |= SAL_INPUT_X;
        if (SDL_JoystickGetButton(joy[j], 1)) inputHeld[j] |= SAL_INPUT_A;
        if (SDL_JoystickGetButton(joy[j], 2)) inputHeld[j] |= SAL_INPUT_B;
        if (SDL_JoystickGetButton(joy[j], 3)) inputHeld[j] |= SAL_INPUT_Y;
        if (SDL_JoystickGetButton(joy[j], 4)) inputHeld[j] |= SAL_INPUT_L;
        if (SDL_JoystickGetButton(joy[j], 5)) inputHeld[j] |= SAL_INPUT_R;
        if (SDL_JoystickGetButton(joy[j], 8)) inputHeld[j] |= SAL_INPUT_SELECT;
        if (SDL_JoystickGetButton(joy[j], 9)) inputHeld[j] |= SAL_INPUT_START;
        if (SDL_JoystickGetButton(joy[j], 8) && SDL_JoystickGetButton(joy[j], 9)) inputHeld[j] |= SAL_INPUT_MENU;
        if (SDL_JoystickGetButton(joy[j], 8) && SDL_JoystickGetButton(joy[j], 4)) inputHeld[j] |= SAL_INPUT_QUICKLOAD;
        if (SDL_JoystickGetButton(joy[j], 8) && SDL_JoystickGetButton(joy[j], 5)) inputHeld[j] |= SAL_INPUT_QUICKSAVE;
    }

    u32 extraKeys = 0;
    if (j == 0) {
        u8 *keys = SDL_GetKeyState(NULL);

        if (keys[SDLK_LCTRL])		inputHeld[j] |= SAL_INPUT_A;
        if (keys[SDLK_LALT])		inputHeld[j] |= SAL_INPUT_B;
        if (keys[SDLK_SPACE])		inputHeld[j] |= SAL_INPUT_X;
        if (keys[SDLK_LSHIFT])		inputHeld[j] |= SAL_INPUT_Y;
        if (keys[SDLK_TAB])			inputHeld[j] |= SAL_INPUT_L;
        if (keys[SDLK_BACKSPACE])	inputHeld[j] |= SAL_INPUT_R;
        if (keys[SDLK_PAGEUP])		inputHeld[j] |= SAL_INPUT_L2;
        if (keys[SDLK_PAGEDOWN])	inputHeld[j] |= SAL_INPUT_R2;
        if (keys[SDLK_RETURN])		inputHeld[j] |= SAL_INPUT_START;
        if (keys[SDLK_ESCAPE])		inputHeld[j] |= SAL_INPUT_SELECT;
        if (keys[SDLK_UP])			inputHeld[j] |= SAL_INPUT_UP;
        if (keys[SDLK_DOWN])		inputHeld[j] |= SAL_INPUT_DOWN;
        if (keys[SDLK_LEFT])		inputHeld[j] |= SAL_INPUT_LEFT;
        if (keys[SDLK_RIGHT])		inputHeld[j] |= SAL_INPUT_RIGHT;
        if (keys[SDLK_KP_DIVIDE])		inputHeld[j] |= SAL_INPUT_L3;
        if (keys[SDLK_KP_PERIOD])		inputHeld[j] |= SAL_INPUT_R3;
        //if (keys[SDLK_END] || keys[SDLK_HOME] || (keys[SDLK_ESCAPE] && keys[SDLK_RETURN])) inputHeld[j] |= SAL_INPUT_MENU;

        //if (keys[SDLK_ESCAPE] && keys[SDLK_TAB]) inputHeld[j] |= SAL_INPUT_QUICKLOAD;
        //if (keys[SDLK_ESCAPE] && keys[SDLK_BACKSPACE]) inputHeld[j] |= SAL_INPUT_QUICKSAVE;

        //if (keys[SDLK_KP_DIVIDE]) inputHeld[j] |= SAL_INPUT_QUICKSAVE;
        //if (keys[SDLK_KP_PERIOD]) inputHeld[j] |= SAL_INPUT_QUICKLOAD;
        if (keys[SDLK_ESCAPE] && keys[SDLK_RETURN]) {
            fprintf(stderr, "keys[SDLK_ESCAPE] && keys[SDLK_RETURN\n");
            inputHeld[j] |= SAL_INPUT_QUITE_GAME;
        }


        SDL_Event event;
        if (!SDL_PollEvent(&event)) {
            if (held) return inputHeld[j];
            return 0;
        }

        do {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_HOME:
                            //fprintf(stderr, "sal_Input the input is SDL_KEYDOWN\n");
                            extraKeys |= SAL_INPUT_MENU;
                            break;
                    }
                    break;
            }
        } while(SDL_PollEvent(&event));
    }

    mInputRepeat = inputHeld[j];
    return inputHeld[j] | extraKeys;
}

static int key_repeat_enabled = 1;

u32 sal_InputPollRepeat(u32 j)
{
    if (!key_repeat_enabled) {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
        key_repeat_enabled = 1;
    }
    return sal_Input(0, j);
}
u32 sal_InputPoll(u32 j)
{
    if (key_repeat_enabled) {
        SDL_EnableKeyRepeat(0, 0);
        key_repeat_enabled = 0;
    }
    return sal_Input(1, j);
}



const char *sal_DirectoryGetTemp(void) {
    return "/tmp";
}

void sal_CpuSpeedSet(u32 mhz) {
}

u32 sal_CpuSpeedNext(u32 currSpeed) {
    u32 newSpeed = currSpeed + 1;
    if (newSpeed > 500) newSpeed = 500;
    return newSpeed;
}

u32 sal_CpuSpeedPrevious(u32 currSpeed) {
    u32 newSpeed = currSpeed - 1;
    if (newSpeed > 500) newSpeed = 0;
    return newSpeed;
}

u32 sal_CpuSpeedNextFast(u32 currSpeed) {
    u32 newSpeed = currSpeed + 10;
    if (newSpeed > 500) newSpeed = 500;
    return newSpeed;
}

u32 sal_CpuSpeedPreviousFast(u32 currSpeed) {
    u32 newSpeed = currSpeed - 10;
    if (newSpeed > 500) newSpeed = 0;
    return newSpeed;
}

s32 sal_Init(void) {
    setenv("SDL_NOMOUSE", "1", 1);
//    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) == -1)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK |
                 SDL_INIT_NOPARACHUTE) == -1) {
        return SAL_ERROR;
    }
    sal_TimerInit(60);

    memset(mInputRepeatTimer, 0, sizeof(mInputRepeatTimer));

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	if (TTF_Init() == -1)
	{
		return SAL_ERROR;
	}
    return SAL_OK;
}

u32 sal_VideoInit(u32 bpp) {
    SDL_ShowCursor(0);

    mBpp = bpp;

    //Set up the screen
    mScreen = SDL_SetVideoMode(SAL_SCREEN_WIDTH, SAL_SCREEN_HEIGHT, bpp,
                               SDL_HWSURFACE |
                               #ifdef SDL_TRIPLEBUF
                               SDL_TRIPLEBUF
                               #else
                               SDL_DOUBLEBUF
#endif
                              );

    //If there was an error in setting up the screen
//    if (mScreen == NULL) {
//        sal_LastErrorSet("SDL_SetVideoMode failed");
//        return SAL_ERROR;
//    }
    if (!mScreen) {
#ifdef MAKLOG
        std::cout << "sal.cpp:176" << " " << "setVideoMode fail : "
                  << SDL_GetError() << std::endl;
#endif
        exit(0);
    }

    mFont = TTF_OpenFont("/usr/share/gmenu2x/skins/Default/fonts/SourceHanSans-Regular-04.ttf", 14);

    if (mFont == NULL)
    {
        sal_LastErrorSet("Can't load font");
        return SAL_ERROR;
    }

    return SAL_OK;
}

u32 sal_VideoGetWidth() {
    return mScreen->w;
}

u32 sal_VideoGetHeight() {
    return mScreen->h;
}

u32 sal_VideoGetPitch() {
    return mScreen->pitch;
}

extern int currentWidth;


void sal_VideoEnterGame(u32 fullscreenOption, u32 forceFullScreen, u32 pal, u32 refreshRate)
{

#ifdef GCW_ZERO
    /* Copied from C++ headers which we can't include in C */
	unsigned int Width = 256 /* SNES_WIDTH */,
	             Height = pal ? 239 /* SNES_HEIGHT_EXTENDED */ : 224 /* SNES_HEIGHT */;
	             //<3 not hardware
	unsigned int WIDTHX2 = 512, HEIGHTX2 = pal ? 478 : 448;
	if (fullscreenOption < 3)
	{
		Width = SAL_SCREEN_WIDTH;
		Height = SAL_SCREEN_HEIGHT;
	}
	//hardware scala  X2
	if (fullscreenOption > 3) {
	    Width = 512;
	    if (forceFullScreen) {
	        Height = pal ? 478 /* SNES_HEIGHT_EXTENDED */ : 448 /* SNES_HEIGHT */;
	    } else {
	        //Height = pal ? 480 /* SNES_HEIGHT_EXTENDED */ : 448 /* SNES_HEIGHT */;
	        if (fullscreenOption == 4) {
	        //littlehui modify
	            Height = pal ? 478 /* SNES_HEIGHT_EXTENDED */ : 448 /* SNES_HEIGHT */;
	        } else {
	      	    //pal set 448 scanline not suitable
	            Height = 478;
	        }
	        //fast scanline
	        if (fullscreenOption == 7) {
	            Width = 256;
	        }
	    }
	}
/*	if (fullscreenOption == 4) {
	    //littlehui modify
	    Width = 512;
	    Height = 448;
	}*/
	if (SDL_MUSTLOCK(mScreen)) SDL_UnlockSurface(mScreen);
	mScreen = SDL_SetVideoMode(Width, Height, mBpp, SDL_HWSURFACE |
#ifdef SDL_TRIPLEBUF
		SDL_TRIPLEBUF
#else
		SDL_DOUBLEBUF
#endif
		);
	mRefreshRate = refreshRate;
	if (SDL_MUSTLOCK(mScreen)) SDL_LockSurface(mScreen);
#endif

    //mRefreshRate = refreshRate;
}


void sal_VideoSetPAL(u32 fullscreenOption, u32 forceFullScreen, u32 pal)
{
    //littlehui modify
    if (fullscreenOption >= 3) // hardware scaling
    {
        sal_VideoEnterGame(fullscreenOption, forceFullScreen, pal, mRefreshRate);
    }
}

void sal_VideoExitGame() {
#ifdef GCW_ZERO
    if (SDL_MUSTLOCK(mScreen)) SDL_UnlockSurface(mScreen);
    mScreen = SDL_SetVideoMode(SAL_SCREEN_WIDTH, SAL_SCREEN_HEIGHT, mBpp,
                               SDL_HWSURFACE |
                               #ifdef SDL_TRIPLEBUF
                               SDL_TRIPLEBUF
                               #else
                               SDL_DOUBLEBUF
#endif
                              );
    if (SDL_MUSTLOCK(mScreen)) SDL_LockSurface(mScreen);
#endif
}

void sal_VideoBitmapDim(u16 *img,
                        u32 pixelCount) {
    u32 i;
    for (i = 0; i < pixelCount; i += 2)
        *(u32 *) &img[i] = (*(u32 *) &img[i] & 0xF7DEF7DE) >> 1;
    if (pixelCount & 1)
        img[i - 1] = (img[i - 1] & 0xF7DE) >> 1;
}

void sal_VideoFlip(s32 vsync) {
    if (SDL_MUSTLOCK(mScreen)) SDL_UnlockSurface(mScreen);
    SDL_Flip(mScreen);
    if (SDL_MUSTLOCK(mScreen)) SDL_LockSurface(mScreen);
}

void *sal_VideoGetBuffer() {
    return (void *) mScreen->pixels;
}

void sal_VideoPaletteSync() {

}

void sal_VideoPaletteSet(u32 index,
                         u32 color) {
    *mPaletteCurr++ = index;
    *mPaletteCurr++ = color;
    if (mPaletteCurr > mPaletteEnd) mPaletteCurr = &mPaletteBuffer[0];
}

void sal_Reset(void)
{
	for(int j = 0; j < SDL_NumJoysticks(); j++)
		SDL_JoystickClose(joy[j]);
    sal_AudioClose();
    SDL_Quit();
	if (mFont != NULL)
	{
		TTF_CloseFont(mFont);
		mFont = NULL;
	}
	TTF_Quit();
}


void set_disable_aspect_ratio() {
    FILE *f = fopen(DINGUX_KEEP_ASPECT_RATIO_FILE,
                    "wb");
    if (!f) return;
//    char c = n ? 'Y' : 'N';
    char scale = 'N';
    fwrite(&scale,
           1,
           1,
           f);
    fclose(f);

}
void set_enable_aspect_ratio() {
    FILE *f = fopen(DINGUX_KEEP_ASPECT_RATIO_FILE,
                    "wb");
    if (!f) return;
//    char c = n ? 'Y' : 'N';
    char scale = 'Y';
    fwrite(&scale,
           1,
           1,
           f);
    fclose(f);

}

void set_enable_integer_scale() {

    FILE *f = fopen(DINGUX_INTEGER_SCALING_FILE,
                    "wb");
    if (!f) return;
//    char c = n ? 'Y' : 'N';
    char scale = 'Y';
    fwrite(&scale,
           1,
           1,
           f);
    fclose(f);
}

void set_disable_integer_scale() {

    FILE *f = fopen(DINGUX_INTEGER_SCALING_FILE,
                    "wb");
    if (!f) return;
//    char c = n ? 'Y' : 'N';
    char scale = 'N';
    fwrite(&scale,
           1,
           1,
           f);
    fclose(f);
}
static unsigned int currentMode = 3;

bool updateVideoMode(bool force) {
//    GFX.Screen = (uint8 *) IntermediateScreen;

    if (!force && mMenuOptions.fullScreen == currentMode) {
        if (mMenuOptions.fullScreen == 3 || mMenuOptions.fullScreen == 0) {
            if (IPPU.RenderedScreenWidth == mScreen->w &&  GFX.RealPitch == IPPU.RenderedScreenWidth * sizeof(u16) ) {
#ifdef MAKLOG
                std::cout << "sal.cpp:477" << " "  << "IPPU.RenderedScreenWidth == mScreen->w" << std::endl;
#endif
                return true;
            }

        } else {
            if (GFX.RealPitch == IPPU.RenderedScreenWidth * sizeof(u16)){
#ifdef MAKLOG
                std::cout << "sal.cpp:483" << " "  << "GFX.RealPitch == IPPU.RenderedScreenWidth * sizeof(u16) " << std::endl;
                printf("GFX.RealPitch: %d Render w: %d screen.w: %d screen.pitch: %d\n",

                       GFX.RealPitch, IPPU.RenderedScreenWidth, mScreen->w,
                       mScreen->pitch);
#endif
                return true;
            }
        }
    }

    //sal_VideoClear(0);
    //sal_VideoClear(0);
    //sal_VideoClear(0);


    switch (mMenuOptions.fullScreen) {
        case 0: // origin
            updateWindowSize(IPPU.RenderedScreenWidth, 240, 0, 0);
//            GFX.Screen = (uint8 *) mScreen->pixels;
            set_disable_integer_scale();
            set_enable_aspect_ratio();
            break;
        case 1: // software fast
            updateWindowSize(320, 240, 1, 1);
//            GFX.Screen = (uint8 *) IntermediateScreen;
            break;
        case 2: // software smooth
            updateWindowSize(320, 240, 1, 2);
            break;
        case 3: // hardware
            updateWindowSize(IPPU.RenderedScreenWidth, 240, 0, 3);
            //GFX.Screen = (uint8 *) mScreen->pixels;
            set_disable_integer_scale();
            set_disable_aspect_ratio();
            break;
        case 4: // hardware x2
            updateWindowSize(IPPU.RenderedScreenWidth, 240, 0, 4);
            //GFX.Screen = (uint8 *) mScreen->pixels;
            break;
        case 5: // hardware scanline
            updateWindowSize(IPPU.RenderedScreenWidth, 480, 0, 5);
//            GFX.Screen = (uint8 *) mScreen->pixels;
            break;
        case 6: // hardware grid
            updateWindowSize(IPPU.RenderedScreenWidth, 480, 0, 6);
//            GFX.Screen = (uint8 *) mScreen->pixels
            break;
    }
    GFX.Screen = (uint8 *) IntermediateScreen;
    currentMode = mMenuOptions.fullScreen;

    return false;

}

void updateWindowSize(int width,
                      int height,
                      int isSoftware,
                      u32 fullscreenOption) {

//    if (mScreen->w == width) return;
//    if (isSoftware) {

    GFX.RealPitch = GFX.Pitch = IPPU.RenderedScreenWidth * sizeof(u16);
//    } else {
//
//        GFX.RealPitch = GFX.Pitch = width * sizeof(u16);
//    }
    //GFX.RealPitch = GFX.Pitch = 256 * sizeof(u16);
    GFX.SubScreen = (uint8 *) malloc(GFX.RealPitch * 480 * 2);
    GFX.ZBuffer = (uint8 *) malloc(GFX.RealPitch * 480 * 2);
    GFX.SubZBuffer = (uint8 *) malloc(GFX.RealPitch * 480 * 2);
    GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
    GFX.PPL = GFX.Pitch >> 1;
    GFX.PPLx2 = GFX.Pitch;
    GFX.ZPitch = GFX.Pitch >> 1;

    bool8 PAL = !!(Memory.FillRAM[0x2133] & 4);

#ifdef GCW_ZERO
    /* Copied from C++ headers which we can't include in C */
    unsigned int Width = width /* SNES_WIDTH */,
            Height = PAL ? 239 /* SNES_HEIGHT_EXTENDED */
                         : 224 /* SNES_HEIGHT */;
                         if (isSoftware) Height = 240;

	unsigned int WIDTHX2 = 512, HEIGHTX2 = PAL ? 478 : 448;
	//hardware scala  X2
	if (fullscreenOption > 3) {
	    Width = 512;
	    if (mMenuOptions.forceFullScreen) {
	        Height = PAL ? 478 /* SNES_HEIGHT_EXTENDED */ : 448 /* SNES_HEIGHT */;
	    } else {
	        Height = 478;
	        if (fullscreenOption == 7) {
	            Width = 256;
	        }
	    }
	}

    /*	if (fullscreenOption == 4) {
	    //littlehui modify
	    Width = 512;
	    Height = 448;
	}*/
/*    printf("mode: %d Render w: %d heigh: %d screen.pitch: %d\n",
           fullscreenOption,width, height,mScreen->pitch);*/
    if (mScreen && SDL_MUSTLOCK(mScreen))
        SDL_UnlockSurface(mScreen);
    mScreen = SDL_SetVideoMode(Width, Height, mBpp, SDL_HWSURFACE |
                                                    #ifdef SDL_TRIPLEBUF
                                                    SDL_TRIPLEBUF
                                                    #else
                                                    SDL_DOUBLEBUF
                                                    #endif

                              );

    if (!mScreen) {
//        puts("SDL_SetVideoMode error" + SDL_GetError());
        std::cout << "SDL_SetVideoMode error :" << SDL_GetError() << std::endl;
        exit(0);
    }
    mRefreshRate = Memory.ROMFramesPerSecond;
    if (SDL_MUSTLOCK(mScreen))
        SDL_LockSurface(mScreen);

//    GFX.Screen = (uint8 *) mScreen->pixels;
#endif

}

int mainEntry(int argc, char *argv[]);
// Prove entry point wrapper
int main(int argc,
         char *argv[]) {
    return mainEntry(argc, argv);
//	return mainEntry(argc-1,&argv[1]);
}
