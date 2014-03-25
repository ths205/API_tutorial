/* This file is term.c.  It's written in C.
Uncomment exactly one of the following three macro definitions: */

/* #define UNIX*/  
/* #define MICROSOFT*/ 
/* #define BORLAND */

#if defined UNIX + defined MICROSOFT + defined BORLAND != 1
#error must uncomment exactly one of the above three macro definitions.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef UNIX
#include <curses.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef MICROSOFT
#include <string.h>
#include <windows.h>
#endif

#ifdef BORLAND
/* http://www.borland.com/techpubs/bcppbuilder/v5/updates/std.html */
#include <conio.h>
#include <dos.h>
#endif

#include "term.h"

/* Global variables */

/* Number of calls to term_construct minus number of calls to term_destruct.
Should be only 0 or 1. */
static int count = 0;

#ifdef UNIX
int visibility;	/* cursor visibility, set by curs_set */
#endif

#ifdef MICROSOFT
HANDLE hStdIn;
HANDLE hStdOut;
SMALL_RECT rect = {0, 0, 79, 23};
static void message(const char *name);
#endif

#ifdef BORLAND
static struct text_info ti;
#endif

static void check_count(const char *name);
static void check(const char *name, unsigned x, unsigned y);

void term_construct(void)
{
	if (count != 0) {
		const int save = count;
		if (count > 0) {
			term_destruct();
		}

		fprintf(stderr,
			"term_construct has already been called %d time%s.\n",
			save, save == 1 ? "" : "s");
#ifdef MICROSOFT
		system("PAUSE");
#endif
		exit(EXIT_FAILURE);
	}
	++count;

#ifdef UNIX
	/* inintscr or newterm must be the first curses call. */
	if (initscr() == NULL) {
		fprintf(stderr, "initscr returned NULL\n");
	}

	/* cbreak mode: don't wait for RETURN. */
	if (cbreak() == ERR) {
		fprintf(stderr, "cbreak returned ERR\n");
	}

	/* Don't display typed characters on screen. */
	if (noecho() == ERR) {
		fprintf(stderr, "echo returned ERR\n");
	}

	/* Tell getch to always return immediately:
	   don't wait for user to press key. */
	if (nodelay(stdscr, TRUE) == ERR) {
		fprintf(stderr, "nodelay returned ERR\n");
	}

	/* Turn off type ahead.  Don't postpone screen update even if keystrokes
	   are waiting. */
	if (typeahead(-1) == ERR) {
		fprintf(stderr, "typeahead returned ERR\n");
	}

	/*
	If you comment in the calls to keypad, then getch will return KEY_LEFT,
	etc., for the arrow keys.  In telnet, you have to select Terminal,
	Preferences, VT100 Arrows.
	*/

	/* keypad(stdscr, TRUE);   treat arrow key as a single char */

	/* Make cursor invisible.  ERR if not supported. */
	visibility = curs_set(0);
	if (visibility == ERR && strcmp(getenv("TERM"), "vt100") == 0) {
		printf("\033[?25l"); /* vt100 hide cursor */
	}

	refresh();
#endif

#ifdef MICROSOFT
	if ((hStdIn = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
		message("GetStdHandle(STD_INPUT_HANDLE)");
	}

	if ((hStdOut=GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
		message("GetStdHandle(STD_OUTPUT_HANDLE)");
	}

	if (!SetConsoleWindowInfo(hStdOut, TRUE, &rect)) {
		message("SetConsoleWindowInfo");
	}

	/* white letters on black background */
	if (!SetConsoleTextAttribute(hStdOut,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)) {
		message("SetConsoleTextAttribute");
	}

	/* So no events before term_construct can effect us. */
	if (!FlushConsoleInputBuffer(hStdIn)) {
		message("FlushConsoleInputBuffer");
	}
#endif

#ifdef BORLAND
	window(1, 1, 80, 25);   /* left, top, right, bottom */
	textcolor(WHITE);       /* these are #define's in conio.h */
	textbackground(BLACK);
	clrscr();
	gettextinfo(&ti);
#endif
}

void term_destruct(void)   /* Undo everything that term_construct did. */
{
#ifdef UNIX
	int flags;
#endif

	check_count("term_destruct");
	/* Discard all the keystrokes that have already been made. */
	while (term_key() != '\0') {
	}
	--count;	/* Must come after call to term_key. */

#ifdef UNIX
	/* Make cursor visible again. */
	if (visibility != ERR) {
		curs_set(visibility);
	} else if (strcmp(getenv("TERM"), "vt100") == 0) {
		printf("\033[?25h"); /* vt100 show cursor */
	}

	/* keypad(stdscr, FALSE); */

	if (nodelay(stdscr, FALSE) == ERR) {
		fprintf(stderr, "nodelay returned ERR\n");
	}

	if (echo() == ERR) {
		fprintf(stderr, "echo returned ERR\n");
	}

	if (nocbreak() == ERR) {
		fprintf(stderr, "nocbreak returned ERR\n");
	}

	/* Must be the last curses call. */
	if (endwin() != OK) {
		fprintf(stderr, "endwin did not return OK\n");
	}

	/* Because of a bug in curses, we have to turn off the
	O_NDELAY bit ourselves. */

	flags = fcntl(0, F_GETFL, 0);
	if (flags < 0) {
		fprintf(stderr, "fcntl(F_GETFL) failed\n");
	}
	flags &= ~O_NDELAY;
	if (fcntl(0, F_SETFL, flags) == -1) {
		fprintf(stderr, "fcntl(F_SETFL, %d) failed\n", flags);
	}
#endif

#ifdef MICROSOFT
#endif

#ifdef BORLAND
#if 0
	/* Already done above. */
	while (kbhit()) {
		getch();
	}
#endif

	cprintf("Press any key.\n");
	getch();   /* wait for a keystroke */
#endif
}

unsigned term_xmax(void)
{
	check_count("term_xmax");

#ifdef UNIX
	return stdscr->_maxx - stdscr->_begx;
#endif

#ifdef MICROSOFT
	return rect.Right - rect.Left + 1;
#endif

#ifdef BORLAND
	return ti.winright - ti.winleft + 1;
#endif
}

unsigned term_ymax(void)
{
	check_count("term_ymax");

#ifdef UNIX
	return stdscr->_maxy - stdscr->_begy;
#endif

#ifdef MICROSOFT
	return rect.Bottom - rect.Top + 1;
#endif

#ifdef BORLAND
	return ti.winbottom - ti.wintop + 1;
#endif
}

void term_put(unsigned x, unsigned y, char c)
{
#ifdef MICROSOFT
	COORD coord = {x + rect.Left, y + rect.Top};
	DWORD dword;
#ifdef UNICODE
	WCHAR wc = c;
#endif
#endif

#ifdef BORLAND
	/* White character on black background.  Borland needs parentheses. */
	static char a[3] = {'?', (BLACK << 4) | WHITE, '\0'};
#endif

	check_count("term_ymax");
	check("term_put", x, y);

	if (!isprint((unsigned char)c)) {
		term_destruct();
		fprintf(stderr, "unprintable char to term_put(%u, %u, %u)\n",
			x, y, (unsigned char)c);
#ifdef MICROSOFT
		system("PAUSE");
#endif
		exit(EXIT_FAILURE);
	}

#ifdef UNIX
	mvprintw(y + stdscr->_begy, x + stdscr->_begx, "%c", c);
	refresh();
#endif

#ifdef MICROSOFT
	WriteConsoleOutputCharacter(hStdOut,
#ifdef UNICODE
		&wc,
#else
		&c,
#endif
		1, coord, &dword);
#endif

#ifdef BORLAND
	a[0] = c;
	puttext(x + ti.winleft, y + ti.wintop,
	        x + ti.winleft, y + ti.wintop, a);
#endif
}

void term_puts(unsigned x, unsigned y, const char *s)
{
#ifdef MICROSOFT
	COORD coord = {x + rect.Left, y + rect.Top};
#endif
	size_t len;
	const char *p;

	check_count("term_puts");
	check("term_puts", x, y);

	len = strlen(s);
	if (y * term_xmax() + x + len >= term_ymax() * term_xmax()) {
		term_destruct();
		fprintf(stderr,
			"term_puts had no room to print string of length %u\n"
			"starting at position (%u, %u).\n", len, x, y);
#ifdef MICROSOFT
		system("PAUSE");
#endif
		exit(EXIT_FAILURE);
	}

	for (p = s; *p != '\0'; ++p) {
		if (!isprint((unsigned char)*p)) {
			term_destruct();
			fprintf(stderr,
				"unprintable char %u at subscript %u "
				"in term_puts(%u, %u)\n",
				(unsigned char)*p, p - s, x, y);
#ifdef MICROSOFT
			system("PAUSE");
#endif
			exit(EXIT_FAILURE);
		}
	}

#ifdef UNIX
	mvprintw(y + stdscr->_begy, x + stdscr->_begx, "%s", s);
	refresh();
#endif

#ifdef MICROSOFT
	/*
	Can't pass the whole string to WriteConsoleOutputCharacter all at once,
	because #ifdef UNICODE, WriteConsoleOutputCharacter will expect a
	pointer to a WCHAR.
	*/
	for (; *s != '\0'; ++s) {
		term_put(x, y, *s);
		if (++x >= term_xmax()) {
			/* Wrap around to start of new line. */
			x = 0;
			if (++y >= term_ymax()) {
				/* Wrap around to upper left corner of screen.*/
				y = 0;
			}
		}
	}
#endif

#ifdef BORLAND
	for (; *s != '\0'; ++s) {
		term_put(x, y, *s);
		if (++x >= term_xmax()) {
			/* Wrap around to start of new line. */
			x = 0;
			if (++y >= term_ymax()) {
				/* Wrap around to upper left corner of screen.*/
				y = 0;
			}
		}
	}
#endif
}

char term_get(unsigned x, unsigned y)
{
#ifdef MICROSOFT
	COORD coord = {x + rect.Left, y + rect.Top};
#ifdef UNICODE
	WCHAR c;	/* LPTSTR is a pointer to WCHAR */
#else
	char c;		/* LPTSTR is a pointer to char */
#endif
	DWORD dword;
#endif

#ifdef BORLAND
	char a[3];
#endif

	check_count("term_get");
	check("term_get", x, y);

#ifdef UNIX
	refresh();
	return A_CHARTEXT & mvinch(y + stdscr->_begy, x + stdscr->_begx);
#endif

#ifdef MICROSOFT
	if (!ReadConsoleOutputCharacter(hStdOut, &c, 1, coord, &dword)) {
		message("term_get");
	}
#ifdef UNICODE
	return (char)c;
#else
	return c;
#endif
#endif

#ifdef BORLAND
	gettext(x + ti.winleft, y + ti.wintop,
	        x + ti.winleft, y + ti.wintop, a);
	return a[0];
#endif
}

/*
Return the key the user pressed.  If no key was pressed, return '\0'
immediately.
*/

char term_key(void)
{
#ifdef UNIX
	int c;
#endif

#ifdef MICROSOFT
	INPUT_RECORD record;
	DWORD events;
	BOOL keydown;
#endif

	check_count("term_key");

#ifdef UNIX
	/*
	Because of the call to nodelay in term_construct, getch will return ERR
	immediately instead of waiting if the user hasn't pressed a key.
	*/
	c = getch();
	return c == ERR ? '\0' : c;
#endif

#ifdef MICROSOFT
	/*
	Read and discard all the hitherto unread events (if any), until the
	first keydown event is encountered.  Then return the key that was
	pressed down.  If there was no keydown event, discard all the events and
	return '\0'.
	*/
	do {
		if (!PeekConsoleInput(hStdIn, &record, 1, &events) ||
			events <= 0) {
			return '\0';
		}

		keydown = record.EventType == KEY_EVENT &&
			record.Event.KeyEvent.bKeyDown;

		ReadConsoleInput(hStdIn, &record, 1, &events);
	} while (!keydown);

	return record.Event.KeyEvent.uChar.AsciiChar;
#endif

#ifdef BORLAND
	return kbhit() ? getch() : '\0';
#endif
}

/* Wait a fraction of a second. 1000 milliseconds == 1 second. */

void term_wait(int milliseconds)
{
	check_count("term_wait");

#ifdef UNIX
	if (napms(milliseconds) != OK) {
		fprintf(stderr, "napms didn't return OK\n");
	}
#endif

#ifdef MICROSOFT
	Sleep(milliseconds);
#endif

#ifdef BORLAND
#if 0
	delay(milliseconds);
#endif
	sleep(milliseconds / 1000);
#endif
}

void term_beep(void)
{
	check_count("term_beep");

#ifdef UNIX
	if (beep() != OK) {	/* see also flash */
		fprintf(stderr, "beep didn't return OK\n");
	}

	if (refresh() == ERR) {
		fprintf(stderr, "refresh returned ERR\n");
	}
#endif

#ifdef MICROSOFT
	if (Beep(880, 125) == 0) {   /* cycles/second, milliseconds duration */
		message("term_beep");
	}
#endif

#ifdef BORLAND
#if 0
	sound(880);   /* turn on the sound (cycles per second) */
	delay(125);   /* milliseconds */
	nosound();    /* turn off the sound */
#endif
	fputc('\a', stderr);   /* alarm */
#endif
}

static void check_count(const char *name)
{
	if (count != 1) {
		if (count > 0 && strcmp(name, "term_destruct") != 0) {
			term_destruct();
		}

		fprintf(stderr,
			"%s: must have exactly one previous call to "
				"term_construct\n", name);
#ifdef MICROSOFT
		system("PAUSE");
#endif
		exit(EXIT_FAILURE);
	}
}

static void check(const char *name, unsigned x, unsigned y)
{
	if (x >= term_xmax()) {
		term_destruct();
		fprintf(stderr, "%s: x == %u, term_xmax == %u\n",
			name, x, term_xmax());
#ifdef MICROSOFT
		system("PAUSE");
#endif
		exit(EXIT_FAILURE);
	}

	if (y >= term_ymax()) {
		term_destruct();
		fprintf(stderr, "%s: y == %u, term_ymax == %u\n",
			name, y, term_ymax());
#ifdef MICROSOFT
		system("PAUSE");
#endif
		exit(EXIT_FAILURE);
	}
}

#ifdef MICROSOFT
void message(const char *name)
{
	const DWORD e = GetLastError();
	LPVOID buffer;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,  /* string containing error message */
		e,
		0,     /* language */
		(LPTSTR)&buffer,
		0,     /* minimum number of characters to allocate */
		NULL   /* additional arguments */
	);

	fprintf(stderr, "%s failed with error %d: %s\n", name, e, buffer);
	LocalFree(buffer);
	system("PAUSE");
	exit(EXIT_FAILURE);
}
#endif
