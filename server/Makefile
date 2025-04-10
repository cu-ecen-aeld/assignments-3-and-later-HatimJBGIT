CC := $(CROSS_COMPILE)gcc
#CC := aarch64-none-linux-gnu-gcc
CFLAGS := -Wall -Wextra -O2 -g
LDFLAGS := -lpthread
TARGET := aesdsocket
OBJS := socket.o

all: $(TARGET)

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

