#include "panicw.hpp"
#include <iostream>

void
panicw(HRESULT result, const char *msg)
{
	std::cerr << "Vm: " << msg;
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM |
		      FORMAT_MESSAGE_IGNORE_INSERTS,
		      NULL, result,
		      MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		      (LPTSTR) &lpMsgBuf, 0, NULL);
	std::cerr << lpMsgBuf;
	exit(EXIT_FAILURE);
}

