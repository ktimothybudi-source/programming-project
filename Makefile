# Makefile for macOS build (clang + raylib via Homebrew)

TARGET = night_market
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

# Try both common Homebrew prefixes
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include -I/usr/local/include
LDFLAGS = -L/opt/homebrew/lib -L/usr/local/lib -lraylib \
          -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

all: $(TARGET)

$(TARGET): $(OBJS)
	clang $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	clang $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)