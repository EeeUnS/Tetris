#ifdef _WIN32
#include "consolCommon.h"
void consolCommon::gotoxy(int x, int y)
{
    COORD pos = { 2 * (short)x,(short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
bool consolCommon::__kbhit()
{
    return kbhit();
}
void consolCommon::__sleep(int miliseconds)
{
    Sleep(miliseconds);
}
void consolCommon::setCursorType(eCursorType c)
{
    CONSOLE_CURSOR_INFO curInfo;
    switch (c)
    {
    case eCursorType::NO_CURSOR:
        curInfo.dwSize = 1;
        curInfo.bVisible = FALSE;
        break;
    case eCursorType::SOLID_CURSOR:
        curInfo.dwSize = 100;
        curInfo.bVisible = TRUE;
        break;
    case eCursorType::NOMAL_CURSOR:
        curInfo.dwSize = 20;
        curInfo.bVisible = TRUE;
        break;
    default:
        break;
    }
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}
 void consolCommon::clear()
{
    system("cls");
}
 int consolCommon::__getch()
{
    return getch();
}


#endif // WIND