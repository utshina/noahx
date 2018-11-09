TARGET = noahx.exe
CC = gcc
CFLAGS = -std=c11 -I. -Wall -Werror -Wno-parentheses -Wno-unknown-pragmas -D_POSIX_C_SOURCE=200809L
LDFLAGS = -L/cygdrive/c/Windows/System32 -lWinHvPlatform
WINSDKDIR = /cygdrive/c/Program Files (x86)/Windows Kits/10/Include
HEADERS = WinHvPlatform.h WinHvPlatformDefs.h
SRCS = $(shell ls *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
INCS = $(shell ls *.h)

.c.o:
	gcc $(CFLAGS) -c $<


.PHONY: all init clean

all: $(TARGET)

$(TARGET): utsversion $(OBJS)
	gcc $(OBJS) -o $@ $(LDFLAGS)

utsversion:
	echo "const char utsversion[] = \"#1 SMP `env LANG=en date`\";" > utsversion.c
	gcc -c utsversion.c -o utsversion.o

$(foreach SRC,$(SRCS),$(eval $(subst \,,$(shell $(CC) -MM $(SRC)))))

init:
	find "$(WINSDKDIR)" -name "WinHvPlatform*" -exec cp {} . \;

clean:
	rm -f $(TARGET) *.o
