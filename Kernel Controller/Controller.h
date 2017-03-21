//#define WIN32_LEAN_AND_MEAN

#include <thread>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

void Xor(char *text, int length);

void Xor(char* text, int length) {
	char key = 'K'; //Any char will work

	for (int i = 0; i < length; i++)
	{
		text[i] ^= key;
	}
}