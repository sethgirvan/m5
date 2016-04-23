CC = gcc
CXX = g++

LDFLAGS = -g -pthread
EXTERN_OBJECTS =

EXTERN_INCLUDES =
CPPFLAGS =
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra $(addprefix -I, $(EXTERN_INCLUDES)) -g

DEPS =

BUILDDIR = build
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp)
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(notdir $(basename $(SOURCES)))))

TARGET = fakecom

.PHONY: all
all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS) $(DEPS)
	$(CC) $(OBJECTS) $(EXTERN_OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $@ $(CFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/*
	rm -fd $(BUILDDIR)
	rm -f $(TARGET)
