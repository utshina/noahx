TARGET = noahx.exe
CC = clang++
CFLAGS = -std=c++17 -I. -idirafter$(WINSDKINC) -Wno-parentheses -Wno-unknown-pragmas -D_POSIX_C_SOURCE=200809L -MMD -MP
LDFLAGS = -L/cygdrive/c/Windows/System32 -lWinHvPlatform
WINSDKDIR = /cygdrive/c/Program Files (x86)/Windows Kits/10/Include
WINSDKINC = "$(shell find "$(WINSDKDIR)" -name WinHvPlatform.h -printf "%h\n" | tail -n 1)"
SRCS = $(shell ls *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
CPPSRCS = $(shell ls *.cpp)
CPPOBJS = $(patsubst %.cpp,%.opp,$(CPPSRCS))
INCS = $(shell ls *.h)

.SUFFIXES: .opp .exe

.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.opp:
	$(CC) $(CFLAGS) -c $< -o `basename $< .cpp`.opp

.cpp.exe:
	$(CC) $(CFLAGS) -DTEST $< -o $@

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
	echo "extern const char utsversion[] = \"#1 SMP `env LANG=en date`\";" > utsversion.c
	$(CC) -c utsversion.c -o utsversion.o
	$(CC) $(OBJS) $(CPPOBJS) -o $@ $(LDFLAGS)

-include *.d

clean:
	rm -f $(TARGET) *.o *.d

Range.exe: Range.cpp

test: Range.exe
	./$<
