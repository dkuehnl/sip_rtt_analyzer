CC =gcc
CFLAGS = -Wall -Wextra -O2 -mtune=native
TARGET = siprta
SRC = main.c
INSTALL_DIR = /usr/bin

all: build install clean 

build: $(TARGET)

$(TARGET): $(SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	cp $(TARGET) $(INSTALL_DIR)

clean: 
	rm -f $(TARGET) $(SRC:.c=.o)