#ifndef __CONSOLMANAGER__
#define __CONSOLMANAGER__ 

#include<stdlib.h>
#ifdef _WIN32
#include<Windows.h>
#include<conio.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma warning(disable:4996)
#elif __linux
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif


enum class eCursorType
{
	NO_CURSOR,
	SOLID_CURSOR,
	NOMAL_CURSOR
};

class consolCommon
{
public:
	static void gotoxy(int x, int y);
	static bool __kbhit();
	static void __sleep(int miliseconds);
	static void setCursorType(eCursorType c);
	static void clear();
	static int __getch();
};

#endif