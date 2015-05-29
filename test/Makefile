#############################
# Makefile tm-test
# Author: Lester
#############################

VERSION    = 1.8
PROGRAM    = tm-test

# cross-compile : set PREFIX and PATH
PREFIX     = /qsi_rootfs/rootfs_l
CC_PATH    = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/
CROSS      = arm-linux-
DEST_PATH  = ../output

# Compiler
HOST       = $(CC_PATH)$(CROSS)
CC         = $(HOST)gcc
DEFINES    = -DQ_ASSERT -D_GNU_SOURCE -DTM_TEST_VERSION='"$(VERSION)"'
CFLAGS     = -O2 -g -Wall -std=gnu99 -march=armv7-a -mfpu=neon $(DEFINES)
INCPATH    = -I$(PREFIX)/usr/include -I$(PREFIX)/usr/local/include -I. -I../include
LINK       = $(HOST)gcc
LIBPATH    = -L$(PREFIX)/usr/lib -L$(PREFIX)/usr/local/lib -L$(PREFIX)/lib
RPATH      =
RPATH_LINK = $(PREFIX)/usr/lib
LFLAGS     = -Wl,-rpath-link=$(RPATH_LINK)
LIBS       = $(LIBPATH) -lpthread -lQSI-IPCLib -lts
AR         = $(HOST)ar
OBJ_PATH   = 

OBJECTS    = $(shell ls ./*.c | sed 's/\.c/.o/g') 
			 

# All Target
all: $(PROGRAM) install

$(PROGRAM): $(OBJECTS)
	@echo -e '\e[1;36mMake $@... \e[0m'
	$(CC) -o $@ $(OBJECTS) $(LFLAGS) $(LIBS)  

$(OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) $(INCPATH) $< -o $@
	
install:
	@test -d $(DESTDIR) || mkdir -p $(DESTDIR)
	@mv $(PROGRAM) $(DEST_PATH)/
	@sync
	@echo -e '\e[1;33mOutput : $(DEST_PATH)/$(PROGRAM)\e[0m'
	
clean:
	@echo -e '\e[1;31mClean $(PROGRAM)...\e[0m'
	@rm -f $(OBJ_PATH)/*.o
	


