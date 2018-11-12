TARGET = noahx.exe
CC = gcc
CFLAGS = -std=c11 -I. -idirafter$(WINSDKINC) -Wall -Werror -Wno-parentheses -Wno-unknown-pragmas -D_POSIX_C_SOURCE=200809L -MMD -MP
LDFLAGS = -L/cygdrive/c/Windows/System32 -lWinHvPlatform
WINSDKDIR = /cygdrive/c/Program Files (x86)/Windows Kits/10/Include
WINSDKINC = "$(shell find "$(WINSDKDIR)" -name WinHvPlatform.h -printf "%h\n" | tail -n 1)"
SRCS = $(shell ls *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
INCS = $(shell ls *.h)

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	echo "const char utsversion[] = \"#1 SMP `env LANG=en date`\";" > utsversion.c
	$(CC) -c utsversion.c -o utsversion.o
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

-include *.d

clean:
	rm -f $(TARGET) *.o *.d
