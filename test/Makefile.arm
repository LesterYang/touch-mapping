#############################
# Makefile
# Author: Lester
#############################

# cross-compile : set PREFIX and HOST
PREFIX     = /Space/ltib2/ltib/rootfs_l
HOST       = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-linux-

# Compiler
CC         = $(HOST)gcc
DEFINES    = -DQSI_ASSERT -D_GNU_SOURCE
CFLAGS     = -O2 -Wall -std=gnu99 -march=armv7-a -mfpu=neon $(DEFINES)
INCPATH    = -I$(PREFIX)/usr/include -I$(PREFIX)/usr/local/include -I. -I./include
LINK       = $(HOST)gcc
LIBPATH    = -L$(PREFIX)/usr/lib -L$(PREFIX)/usr/local/lib -L$(PREFIX)/lib
RPATH      =
RPATH_LINK = $(PREFIX)/usr/lib
LFLAGS     = -Wl,-rpath-link=$(RPATH_LINK)
LIBS       = $(LIBPATH) -lpthread -lts -lQSI-IPCLib 
AR         = $(HOST)ar



OBJECTS    = ./tm_test.o   \
			 ./fbutils.o   \
			 ./font_8x8.o  \
			 
# All Target
all: tm-test

tm-test: $(OBJECTS)
	@echo 'Building target: $@'
	$(CC) -o $@ $(OBJECTS) $(LFLAGS) $(LIBS)  

$(OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) $(INCPATH) $< -o $@
	

	
clean:
	rm -f $(OBJECTS)
	


