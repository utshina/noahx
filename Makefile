TARGET = noahx.exe
CFLAGS = -I.
LDFLAGS = -L/cygdrive/c/Windows/System32 -lWinHvPlatform
WINSDK = /cygdrive/c/Program Files (x86)/Windows Kits/10/Include
HEADERS = WinHvPlatform.h WinHvPlatformDefs.h

$(TARGET): main.c $(HEADERS)
	gcc -std=c11 $< -o $@ $(CFLAGS) $(LDFLAGS)

$(HEADERS):
	find "$(WINSDK)" -name "WinHvPlatform*" -exec cp {} . \;

clean:
	rm -f $(TARGET) $(HEADERS)
