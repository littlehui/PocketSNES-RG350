#
# PocketSNES for the RetroFW
#
# by pingflood; 2019
#

# Define the applications properties here:

TARGET = pocketsnes/PocketSNES.dge

CROSS_COMPILE ?= mipsel-linux-

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs) -lSDL_ttf

INCLUDE = -I src \
		-I sal/linux/include -I sal/include \
		-I src/include \
		-I menu -I src/linux -I src/snes9x

CFLAGS = $(INCLUDE) -DRC_OPTIMIZED -DGCW_ZERO -D__LINUX__ -D__DINGUX__ -DFOREVER_16_BIT -DFOREVER_16_BIT_SOUND -DLAGFIX -DNO_ROM_BROWSER
# CFLAGS += -ggdb3 -Og

CFLAGS += -O3 -fdata-sections -ffunction-sections -mips32r2 -mno-mips16 -fomit-frame-pointer -fno-builtin
CFLAGS += -fno-common -Wno-write-strings -Wno-sign-compare -ffast-math -ftree-vectorize --std=gnu11
CFLAGS += -funswitch-loops -fno-strict-aliasing
CFLAGS += -DFAST_ALIGNED_LSB_WORD_ACCESS
CFLAGS += -flto
CFLAGS += $(SDL_CFLAGS)
#CFLAGS += -DMAKLOG
# -floop-interchange -fno-unroll-loops
ifdef PROFILE_GEN
CFLAGS += -fprofile-generate -fprofile-dir=/media/data/profile/pocketsnes
else
CFLAGS += -fprofile-use -fprofile-dir=./profile
endif

CXXFLAGS = $(filter-out --std=gnu11,$(CFLAGS)) --std=gnu++11 -fno-exceptions -fno-rtti -fno-math-errno -fno-threadsafe-statics

LDFLAGS = $(CXXFLAGS) -lz -lpng $(SDL_LIBS) -Wl,--as-needed
ifndef PROFILE_GEN
LDFLAGS += -Wl,--gc-sections -s
endif

# Find all source files
SOURCE = src/snes9x menu sal/linux sal
SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJS    = $(OBJ_CPP) $(OBJ_C)
#@echo $(info $(OBJS))

.PHONY : all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	$(STRIP) $(TARGET)

ipk: all
	@rm -rf /tmp/.pocketsnes-ipk/ && mkdir -p /tmp/.pocketsnes-ipk/root/home/retrofw/emus/pocketsnes /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	@cp dist/PocketSNES.dge dist/PocketSNES.man.txt dist/PocketSNES.png dist/backdrop.png /tmp/.pocketsnes-ipk/root/home/retrofw/emus/pocketsnes
	@cp dist/pocketsnes.lnk /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators
	@cp dist/snes.pocketsnes.lnk /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	# @sed "s/^Version:.*/Version: $$(date +%Y%m%d)/" dist/control > /tmp/.pocketsnes-ipk/control
	@sed "s/^Version:.*/Version: 20190304/" dist/control > /tmp/.pocketsnes-ipk/control
	@cp dist/conffiles /tmp/.pocketsnes-ipk/
	# echo -e "#!/bin/sh\nmkdir -p /home/retrofw/profile/pocketsnes; exit 0" > /tmp/.pocketsnes-ipk/preinst
	# chmod +x /tmp/.gmenu-ipk/postinst /tmp/.pocketsnes-ipk/preinst
	@tar --owner=0 --group=0 -czvf /tmp/.pocketsnes-ipk/control.tar.gz -C /tmp/.pocketsnes-ipk/ control conffiles #preinst
	@tar --owner=0 --group=0 -czvf /tmp/.pocketsnes-ipk/data.tar.gz -C /tmp/.pocketsnes-ipk/root/ .
	@echo 2.0 > /tmp/.pocketsnes-ipk/debian-binary
	@ar r dist/pocketsnes.ipk /tmp/.pocketsnes-ipk/control.tar.gz /tmp/.pocketsnes-ipk/data.tar.gz /tmp/.pocketsnes-ipk/debian-binary

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY : clean

opk: all
	./opk/make_opk.sh

clean :
	rm -f $(OBJS) pocketsnes/pocketsnes.ipk
