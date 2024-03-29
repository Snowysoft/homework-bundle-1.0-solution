CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS =
SRC = client2.c
OBJ = $(SRC:.c=.o)
TARGET = client2

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
