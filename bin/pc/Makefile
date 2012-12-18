#*******************************************************************************
# * Copyright (c) 2009, Rockwell Automation, Inc.
# * All rights reserved. 
# *
# * Contributors:
# *     <date>: <author>, <author email> - changes
# ******************************************************************************

#CC=g++

ROOT=../..

ENCAP=$(ROOT)/src/enet_encap
CIP=$(ROOT)/src/cip
PORT=$(ROOT)/src/ports/platform-pc

CFLAGS= \
	-g \
	-I . \
	-I$(ROOT) \
	-I$(ENCAP) \
	-I$(CIP) \
	-I$(PORT) \
	-DOPENER_WITH_TRACES \
	-DOPENER_TRACE_LEVEL=0xFF \
	-W -Wall -ansi -std=c99

LDFLAGS=

SOURCES= \
	$(CIP)/appcontype.c \
	$(CIP)/cipassembly.c \
	$(CIP)/cipclass3connection.c \
	$(CIP)/cipcommon.c \
	$(CIP)/cipconnectionmanager.c	\
	$(CIP)/cipethernetlink.c \
	$(CIP)/cipidentity.c \
	$(CIP)/cipioconnection.c \
	$(CIP)/cipmessagerouter.c \
	$(CIP)/ciptcpipinterface.c \
	$(ENCAP)/cpf.c \
	$(ENCAP)/encap.c \
	$(ENCAP)/endianconv.c \
	$(PORT)/main.c \
	$(PORT)/networkhandler.c


OBJECTS= \
	$(CIP)/appcontype.o \
	$(CIP)/cipassembly.o \
	$(CIP)/cipclass3connection.o \
	$(CIP)/cipcommon.o \
	$(CIP)/cipconnectionmanager.o \
	$(CIP)/cipethernetlink.o \
	$(CIP)/cipioconnection.o \
	$(CIP)/cipidentity.o \
	$(CIP)/cipmessagerouter.o \
	$(CIP)/ciptcpipinterface.o \
	$(ENCAP)/cpf.o \
	$(ENCAP)/encap.o \
	$(ENCAP)/endianconv.o \
	$(PORT)/main.o \
	$(PORT)/networkhandler.o 
	
SPLINTFLAGS = -I../../src \
	-I$(ENCAP) \
	-I$(CIP) \
	-I$(PORT) \
	+posixlib -skipposixheaders


EXECUTABLE=opener


all: $(SOURCES) $(EXECUTABLE)
	
splint:
	splint $(SPLINTFLAGS) $(SOURCES) 
    	
$(EXECUTABLE): $(OBJECTS) Makefile
	@echo "  [LD]	" $@
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@echo BUILD COMPLETE

# pull in dependency info for existing .o files
-include .deps/*.d

%.o: %.c .deps Makefile
	@echo "  [CC]	" $*.c
	@$(CC) -c $(CFLAGS) -MMD -MP -MF .deps/$(*F).d $*.c -o $*.o

.deps:
	mkdir .deps


clean:
	@echo "  [CLEAN]"
	@-rm -f $(EXECUTABLE) $(OBJECTS) .deps/*

