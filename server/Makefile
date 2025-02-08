CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra -O2 -g
TARGET := aesdsocket
OBJS := socket.o

all: $(TARGET)

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

