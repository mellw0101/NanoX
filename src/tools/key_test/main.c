#include <ncurses.h>

int main(void) {
	int ch;
	initscr();
	noecho();
	cbreak();
	refresh();
	WINDOW *win = newpad(20, 30);
	box(win, 0, 0);
	prefresh(win, 0, 0, 0, 0, 19, 29);
	doupdate();
	while ((ch = getch())) {
		printw("%c ", ch);
		refresh();
	}
	endwin();
	return 0;
}
