#############################
# Makefile tm-daemon
# Author: Lester
#############################

VERSION    = 1.6

# cross-compile : set PREFIX and PATH
PREFIX     = /Space/ltib2/ltib/rootfs_l
CC_PATH    = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/
CROSS      = arm-linux-
DEST_PATH  = release

# Compiler
HOST       = $(CC_PATH)$(CROSS)
CC         = $(HOST)gcc
DEFINES    = -DQ_ASSERT -D_GNU_SOURCE -DTM_VERSION='"$(VERSION)"'
CFLAGS     = -O2 -Wall -Werror -std=gnu99 -march=armv7-a -mfpu=neon $(DEFINES)
INCPATH    = -I$(PREFIX)/usr/include -I$(PREFIX)/usr/local/include -I. -I./include
LINK       = $(HOST)gcc
LIBPATH    = -L$(PREFIX)/usr/lib -L$(PREFIX)/usr/local/lib -L$(PREFIX)/lib
RPATH      =
RPATH_LINK = $(PREFIX)/usr/lib
LFLAGS     = -Wl,-rpath-link=$(RPATH_LINK)
LIBS       = $(LIBPATH) -lpthread -lmtdev -lQSI-IPCLib -lrt
AR         = $(HOST)ar
OBJ_PATH   = $(DEST_PATH)/.obj

OBJECTS    = $(shell ls ./src/*.c | sed 's/\.c/.o/g')
    		 
# All Target
all: tm-daemon tm-test move

tm-daemon: $(OBJECTS)
	@echo 'Building target: $@'
	$(CC) -o $@ $(OBJECTS) $(LFLAGS) $(LIBS)  

$(OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) $(INCPATH) $< -o $@
	
tm-test:
	cd test; make
	
move:
	test -d $(OBJ_PATH) || mkdir -p $(OBJ_PATH)
	mv tm-daemon $(DEST_PATH)
	mv test/tm-test $(DEST_PATH)
	mv $(OBJECTS) $(OBJ_PATH)
	cd test; make move
	sync
	
clean:
	rm -f $(OBJ_PATH)/*.o
	cd test; make clean


