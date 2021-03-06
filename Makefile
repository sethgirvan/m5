PLATFORM = pc

ifeq ($(PLATFORM), avr)
	CC = avr-gcc
	CXX = avr-g++
else
	CC = gcc
	CXX = g++
endif

MCU = atmega2560
F_CPU = 16000000UL
avr_CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DAVR

EXTERN_INCLUDES = ../io/src $($(PLATFORM)_EXTERN_INCLUDES)
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra -I$(SRCDIR) -g $($(PLATFORM)_CFLAGS) $(addprefix -I, $(EXTERN_INCLUDES))
CPPFLAGS = -DIEEE754 $($(PLATFORM)_CPPFLAGS)

BUILDDIR = build_$(PLATFORM)
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp $(SRCDIR)/$(PLATFORM)/*.c $(SRCDIR)/$(PLATFORM)/*.cpp)
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(notdir $(basename $(SOURCES)))))


.PHONY: all
all: $(BUILDDIR) $(OBJECTS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $@ $(CXXFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/$(PLATFORM)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/$(PLATFORM)/%.cpp
	$(CXX) $< -o $@ $(CXXFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f build_pc/*
	rm -f build_avr/*
	rm -fd build_pc
	rm -fd build_avr
