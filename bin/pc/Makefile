#*******************************************************************************
# * Copyright (c) 2009, Rockwell Automation, Inc.
# * All rights reserved. 
# *
# * Contributors:
# *     <date>: <author>, <author email> - changes
# ******************************************************************************

#CC=g++

CFLAGS= \
	-g \
	-I . \
	-I ../../src \
	-I ../../src/enet_encap \
	-I ../../src/cip \
	-I ../../src/ports/platform-pc \
	-DOPENER_WITH_TRACES \
	-DOPENER_TRACE_LEVEL=0xFF \
	-W -Wall -ansi -std=c99

LDFLAGS=

SOURCES= \
	../../src/ports/platform-pc/main.c \
	../../src/cip/cipconnectionmanager.c	\
	../../src/cip/cipcommon.c \
	../../src/cip/cipmessagerouter.c \
	../../src/cip/cipassembly.c \
	../../src/cip/ciptcpipinterface.c \
	../../src/cip/cipidentity.c \
	../../src/cip/cipethernetlink.c \
	../../src/cip/appcontype.c \
	../../src/enet_encap/cpf.c \
	../../src/enet_encap/endianconv.c \
	../../src/enet_encap/encap.c \
	../../src/ports/platform-pc/networkhandler.c


OBJECTS= \
	../../src/ports/platform-pc/main.o \
	../../src/ports/platform-pc/networkhandler.o \
	../../src/cip/cipconnectionmanager.o \
	../../src/cip/cipcommon.o \
	../../src/cip/cipmessagerouter.o \
	../../src/cip/cipassembly.o \
	../../src/cip/ciptcpipinterface.o \
	../../src/cip/cipidentity.o \
	../../src/cip/cipethernetlink.o \
	../../src/cip/appcontype.o  \
	../../src/enet_encap/cpf.o \
	../../src/enet_encap/endianconv.o \
	../../src/enet_encap/encap.o
	
SPLINTFLAGS = -I../../src \
	-I../../src/enet_encap \
	-I../../src/cip \
	-I../../src/ports/platform-pc \
	+posixlib


EXECUTABLE=opener


all: $(SOURCES) $(EXECUTABLE)	
	
splint:
	splint $(SPLINTFLAGS) $(SOURCES) 
    	
$(EXECUTABLE): $(OBJECTS) Makefile
	@echo "  [LD]	" $@
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# pull in dependency info for *existing* .o files
-include ./.deps/*.d
-include ../ethernet/.deps/*.d
-include ../cip/.deps/*.d

# compile and generate dependency info;
# see http://www.cs.berkeley.edu/~smcpeak/autodepend/autodepend.html

DEP="`dirname $*`/.deps/`basename $*.d`"

%.o: %.c Makefile
	@echo "  [CC]	" $*.c
	@$(CC) -c $(CFLAGS) $*.c -o $*.o
	@install -d `dirname $*`/.deps
	@$(CC) -MM $(CFLAGS) $*.c > $(DEP)
	@mv -f $(DEP) $(DEP).tmp
	@sed -e 's|.*:|$*.o:|' < $(DEP).tmp > $(DEP)
	@sed -e 's/.*://' -e 's/\\$$//' < $(DEP).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(DEP)
	@rm -f $(DEP).tmp


clean :
	@echo "  [CLEAN]"
	@-rm -f $(EXECUTABLE) $(OBJECTS)
	@-find .. -name .deps | xargs rm -rf

