#############################
# Makefile tm-daemon
# Author: Lester
#############################

VERSION    = 1.1

# cross-compile : set PREFIX and PATH
PREFIX     = /Space/ltib2/ltib/rootfs_l
CC_PATH    = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/
CROSS      = arm-linux-
DEST_PATH  = release/

# Compiler
HOST       = $(CC_PATH)$(CROSS)
CC         = $(HOST)gcc
DEFINES    = -DQ_ASSERT -D_GNU_SOURCE -DTM_VERSION='"$(VERSION)"'
CFLAGS     = -g3 -O2 -Wall -Werror -std=gnu99 -march=armv7-a -mfpu=neon $(DEFINES)
INCPATH    = -I$(PREFIX)/usr/include -I$(PREFIX)/usr/local/include -I. -I./include
LINK       = $(HOST)gcc
LIBPATH    = -L$(PREFIX)/usr/lib -L$(PREFIX)/usr/local/lib -L$(PREFIX)/lib
RPATH      =
RPATH_LINK = $(PREFIX)/usr/lib
LFLAGS     = -Wl,-rpath-link=$(RPATH_LINK)
LIBS       = $(LIBPATH) -lpthread -lmtdev -lQSI-IPCLib 
AR         = $(HOST)ar


OBJECTS    =  $(shell ls ./src/*.c | sed 's/\.c/.o/g')
    		 
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
	cp tm-daemon $(DEST_PATH) && rm tm-daemon
	cp test/tm-test $(DEST_PATH) && rm test/tm-test
	sync
	
clean:
	rm -f $(OBJECTS)
	cd test; make clean


