# 컴파일러 설정
CC = gcc

# 운영체제 감지
ifeq ($(OS),Windows_NT)
    # Windows PDCurses 경로 설정
	PDCURSES_DIR = ./PDCurses-master
	PDCURSES_LIB_DIR = $(PDCURSES_DIR)/wincon

    #INCLUDE
	CFLAGS = -I$(PDCURSES_DIR) 
	LDFLAGS = -L$(PDCURSES_LIB_DIR) -l:pdcurses.a
	TARGET = viva.exe
	RM = del
else ifeq ($(shell uname -s),Linux)
    #Linux 설정 
	CFLAGS = 
	LDFLAGS = -lcurses
    TARGET = viva
    RM = rm -f

else ifeq ($(shell uname -s),Darwin)
    CFLAGS =
    LDFLAGS = -lncurses
    TARGET = viva
    RM = rm -f
endif

# output 파일 이름 
SRCS = viva.c

# 객체 파일
OBJS = $(SRCS:.c=.o)

$(TARGET): $(SRCS)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LDFLAGS)

# 개별 소스 파일 컴파일 규칙
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(TARGET)