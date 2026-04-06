# Makefile for macOS build (clang + raylib via Homebrew)

UNAME_S := $(shell uname -s)

TARGET = night_market
SRC_DIR = src
SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/minigames/*.c)
ifeq ($(UNAME_S),Darwin)
SRCS := $(filter-out $(SRC_DIR)/intro_video_stub.c,$(SRCS))
INTRO_OBJ = $(SRC_DIR)/intro_video_apple.o
EXTRA_LDFLAGS = -framework AVFoundation -framework CoreMedia
else
INTRO_OBJ =
EXTRA_LDFLAGS =
endif

OBJS = $(SRCS:.c=.o) $(INTRO_OBJ)

# Try both common Homebrew prefixes
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include -I/usr/local/include
LDFLAGS = -L/opt/homebrew/lib -L/usr/local/lib -lraylib \
          -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo \
          $(EXTRA_LDFLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	clang $(OBJS) -o $(TARGET) $(LDFLAGS)

$(SRC_DIR)/intro_video_apple.o: $(SRC_DIR)/intro_video_apple.m $(SRC_DIR)/intro_video.h
	clang -Wall -Wextra -fobjc-arc $(CFLAGS) -c $< -o $@

%.o: %.c
	clang $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(INTRO_OBJ) $(TARGET)