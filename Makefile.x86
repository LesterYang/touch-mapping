#############################
# Makefile
# Author: Lester
#############################

# cross-compile : set PREFIX and HOST
PREFIX     = 
HOST       = 

# Compiler
CC         = $(HOST)gcc
DEFINES    = -DQSI_ASSERT -D_GNU_SOURCE
CFLAGS     = -g3 -Wall -std=gnu99 $(DEFINES)
INCPATH    = -I$(PREFIX)/usr/include -I$(PREFIX)/usr/local/include -I. -I./include
LINK       = $(HOST)gcc
LIBPATH    = -L$(PREFIX)/usr/lib -L$(PREFIX)/usr/local/lib -L$(PREFIX)/lib
# -Wl,-rpath=dir : Add a directory to the runtime library search path
RPATH      =
# -Wl,-rpath-link : When using ELF, one shared library may require another. It's only effective at link time
RPATH_LINK = $(PREFIX)/usr/lib
LFLAGS     =                                  
LIBS       = $(LIBPATH) -lpthread -lmtdev
AR         = $(HOST)ar


TARGET     = tm-daemon

OBJECTS    = ./src/main.o      \
             ./src/tm.o        \
             ./src/tmMapping.o \
             ./src/tmInput.o   \
             ./src/tmIpc.o     \
			 ./src/qUtils.o    \
			 ./test/tm_test.o
			 
			 
# All Target
all: tm

tm: $(OBJECTS)
	@echo 'Building target: $@'
	$(CC) -o $(TARGET) $(OBJECTS) $(LFLAGS) $(LIBS)  

$(OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) $(INCPATH) $< -o $@
	

	
clean:
	rm -f $(OBJECTS)
	


