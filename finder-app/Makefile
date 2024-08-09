CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra -O2
TARGET := writer
OBJS := writer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

