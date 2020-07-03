#ifdef __linux__

void consolCommon::gotoxy(int x, int y)
{
	printf("%c[%d;%df", 0x1B, y, x);
}
bool consolCommon::__kbhit()
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}
void consolCommon::__sleep(int miliseconds)
{
	sleep(miliseconds);
}
void consolCommon::setCursorType(eCursorType c)
{
}

void consolCommon::clear()
{
	system("clear");
}
int consolCommon::__getch()
{
	int ch;
	struct termios buf, save;
	tcgetattr(0, &save);
	buf = save;
	buf.c_lflag &= ~(ICANON | ECHO);
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	tcsetattr(0, TCSAFLUSH, &buf);
	ch = getchar();
	tcsetattr(0, TCSAFLUSH, &save);
	return ch;
}
#endif