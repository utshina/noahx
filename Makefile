TARGET = noahx.exe
CFLAGS = -I.
LDFLAGS = -L/cygdrive/c/Windows/System32 -lWinHvPlatform
WINSDK = /cygdrive/c/Program Files (x86)/Windows Kits/10/Include
HEADERS = WinHvPlatform.h WinHvPlatformDefs.h
OBJS = main.o vmm.o
INCS = $(shell ls *.h)

.c.o:
	gcc -std=c11 $(CFLAGS) -c $<

$(TARGET): $(INCS) $(OBJS) $(HEADERS)
	gcc $(OBJS) -o $@ $(LDFLAGS)

$(HEADERS):
	find "$(WINSDK)" -name "WinHvPlatform*" -exec cp {} . \;

clean:
	rm -f $(TARGET) $(HEADERS)
