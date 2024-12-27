#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int main(void) {
	// int ch;
	// initscr();
	// noecho();
	// cbreak();
	// refresh();
	// WINDOW *win = newpad(20, 30);
	// box(win, 0, 0);
	// prefresh(win, 0, 0, 0, 0, 19, 29);
	// doupdate();
	// while ((ch = getch())) {
		// printw("%c ", ch);
		// refresh();
	// }
	// endwin();
	// return 0;
	struct termios oldt, newt;
	char c;

	// Switch to raw mode
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	printf("Press Control-Enter (or any key):\n");
	while (read(STDIN_FILENO, &c, 1) > 0)
	{
	    printf("Read byte: 0x%02X\n", c);
	    if (c == '\n') // Stop on Enter
	        break;
	}

	// Restore terminal mode
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return 0;
}
