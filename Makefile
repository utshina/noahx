TARGET = noahx.exe
CC = gcc
CFLAGS = -std=c11 -I.
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

$(TARGET): $(OBJS)
	gcc $(OBJS) -o $@ $(LDFLAGS)

$(foreach SRC,$(SRCS),$(eval $(subst \,,$(shell $(CC) -MM $(SRC)))))

init:
	find "$(WINSDKDIR)" -name "WinHvPlatform*" -exec cp {} . \;

clean:
	rm -f $(TARGET) *.o
