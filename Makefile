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
CFLAGS     = -O2 -Wall -Werror -std=gnu99 -march=armv7-a -mfpu=neon $(DEFINES)
INCPATH    = -I$(PREFIX)/usr/include -I$(PREFIX)/usr/local/include -I. -I./include
LINK       = $(HOST)gcc
LIBPATH    = -L$(PREFIX)/usr/lib -L$(PREFIX)/usr/local/lib -L$(PREFIX)/lib
RPATH      =
RPATH_LINK = $(PREFIX)/usr/lib
LFLAGS     = -Wl,-rpath-link=$(RPATH_LINK)
LIBS       = $(LIBPATH) -lpthread -lmtdev -lQSI-IPCLib 
AR         = $(HOST)ar



OBJECTS    = ./src/main.o      \
             ./src/tm.o        \
             ./src/tmMapping.o \
             ./src/tmInput.o   \
             ./src/tmIpc.o     \
			 ./src/qUtils.o    \
			 ./test/tm_test.o
			 
			 
# All Target
all: tm-daemon

tm-daemon: $(OBJECTS)
	@echo 'Building target: $@'
	$(CC) -o $@ $(OBJECTS) $(LFLAGS) $(LIBS)  

$(OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) $(INCPATH) $< -o $@
	

	
clean:
	rm -f $(OBJECTS)
	


