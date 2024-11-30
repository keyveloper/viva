CC = gcc

ifeq ($(OS),Windows_NT)
	PDCURSES_DIR = ./PDCurses-master
	PDCURSES_LIB_DIR = $(PDCURSES_DIR)/wincon
	CFLAGS = -I$(PDCURSES_DIR)
	LDFLAGS = -L$(PDCURSES_LIB_DIR) -l:pdcurses.a
	TARGET = viva.exe
	RM = del
else ifeq ($(shell uname -s | tr '[:upper:]' '[:lower:]'),linux)
	CFLAGS =
	LDFLAGS = -lncurses
	TARGET = viva
	RM = rm -f
else ifeq ($(shell uname -s | tr '[:upper:]' '[:lower:]'),darwin)
	CFLAGS =
	LDFLAGS = -lncurses
	TARGET = viva
	RM = rm -f
endif

SRCS = viva.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(SRCS)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(TARGET)