#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>   // for memset

#define WIDTH 60
#define HEIGHT 15
#define LIFETIME 180 // game time

/* Global variables */
int startx = 0;
int starty = 0;
int clocation = 0;  // current column position
int rlocation = 0;  // current row position
int score = 0;
int line = 0;       // how many lines have been cleared
int quitflag = 0;   // for forcing quit game
int multiclear = 0; // for counting how many rows or columns be cleared in one motion
int sec = 0;        // timer
int printdouble = 0;// print double flag
int printtriple = 0;// print triple flag

/* Function prototypes */
void print_menu(WINDOW *menu_win, char *map);
void disp_timer(WINDOW *menu_win);
void game_initial(char *map);                                // reset board and global variables for new game
void game_execute(WINDOW *menu_win, char *map);
void move_updown(char *map, int column_number, int ud_flag); // flag 1 = up, 0 = down
void move_leftright(char *map, int row_number, int lr_flag); // flag 1 = left, 0 = right
void state_judge(char *map);
void print_result(void);
void highscore_memo(void);                                   // not complete yet
void alarm_handler(int sig);
void timer_start(void);

int main(void)
{
    WINDOW *menu_win;
    char map[36];

    srand((unsigned int)time(0));

    initscr();
	clear();
	noecho();
	cbreak();	/* Line buffering disabled. pass on everything */
	halfdelay(10);
	startx = (80 - WIDTH) / 2;
	starty = (30 - HEIGHT) / 2;

/* Color Parameters */
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_RED);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_BLACK, COLOR_CYAN);
    init_pair(4, COLOR_RED, COLOR_WHITE);

    game_initial(map);

	menu_win = newwin(HEIGHT, WIDTH, starty, startx);
	keypad(menu_win, TRUE);

    bkgd(COLOR_PAIR(2));            // outer window color
    wbkgd(menu_win, COLOR_PAIR(3)); // inner window color

    mvprintw(0, 0, "**INSTRUCTION**");
    mvprintw(1, 0, "1.Use arrow keys to select a row or column, q to terminate the program");
    mvprintw(2, 0, "2.Move elements at current column/row e = up, d = right, x = down, s = left");
    mvprintw(3, 0, "3.When all elements in a row or column are identical, score obtained");
    mvprintw(4, 0, "4.If you clear multiple lines at the same time, higher score obtained");
    mvprintw(6, 0, "Press enter to start the game");

    refresh();

    print_menu(menu_win, map);
    game_execute(menu_win, map);

    clrtoeol();
	refresh();
	endwin();

    if (quitflag == 1)  printf("Terminated!\n");
    else                print_result();

    return 0;
}

void game_initial(char *map)
{
    int i, x, y, space_to_fill = 0;

/* Initialize Game Board */
    for (i = 35; i >= 0; i--)
        map[i] = ' ';

/* Generate 24 elements in 6 * 6 array */
    do {
        x=rand()%36;
        if(map[x] != 'T' && map[x] != 'A') {
        y=rand()%2;
		if(y == 1)  map[x] = 'T';
		else        map[x] = 'A';
        space_to_fill++;
        }
	} while(space_to_fill < 24);

/* Initialize related parameter */
    clocation = 0;
    rlocation = 0;
    quitflag = 0;
    score = 0;
    line = 0;

    return;
}

void print_menu(WINDOW *menu_win, char *map)
{
	int x, y, i;

	x = 2;
	y = 2;
	box(menu_win, 0, 0);

	for(i = 0; i < 36; i++){
        if (i > -1 + (rlocation * 6) && i < 6 + (rlocation * 6)){
            //wattron(menu_win, A_REVERSE);
            wattron(menu_win, COLOR_PAIR(4));
			if (i % 6 != 5) mvwprintw(menu_win, y, x + 2, "%c ", map[i]);
			else            mvwprintw(menu_win, y, x + 2, "%c", map[i]);  // remove extra highlighted space at row end
			wattroff(menu_win, COLOR_PAIR(4));
        }
        else if ((i - clocation) % 6 == 0){
            wattron(menu_win, COLOR_PAIR(4));
			mvwprintw(menu_win, y, x + 2, "%c", map[i]);
            wattroff(menu_win, COLOR_PAIR(4));
            mvwprintw(menu_win, y, x + 3, " "); // Adjust screen
        }
        else{
        mvwprintw(menu_win, y, x + 2, "%c ", map[i]);
        }
        x += 2;
    /* Change row (6 elements per row)*/
        if (((i + 1) % 6) == 0){
        x = 2;
        ++y;
        }
    }

    //mvwprintw(menu_win, y + 1, 1, "current life: %d ", life);
    if (sec == 0) {
    mvwprintw(menu_win, 2, 29, "remaining time: 180");
    }

    mvwprintw(menu_win, 3, 29, "current score: %d", score);

    if (multiclear == 2)
    printdouble = 3;
    else if (multiclear == 3)
    printtriple = 3;

    mvwprintw(menu_win, y + 4, 15, " "); // Move cursor far away for UI

	wrefresh(menu_win);
}

void disp_timer(WINDOW *menu_win)
{
  //mvwprintw(menu_win, 3, 30, "%02i:%02i:%02i", hour, min, sec);
    mvwprintw(menu_win, 2, 29, "remaining time: %03i", 180 - sec);
    if (printdouble != 0){
        mvwprintw(menu_win, 10, 2, "DOUBLE CLEAR! Bonus score 2 points!");
        printdouble--;
    }
    else if (printtriple != 0){
        mvwprintw(menu_win, 10, 2, "Triple CLEAR! Bonus score 5 points!");
        printtriple--;
    }
    wrefresh(menu_win);
}

void alarm_handler(int sig)
{
  sec++;
  sig += 0;
  /* removed due to count seconds only
  if(sec == 60){
    sec = 0;
    min++;
    if(min == 60){
      min = 0;
      hour++;
    }
  }
  */
}

void timer_start()
{
    struct itimerval value;
    signal(SIGALRM, alarm_handler);
    memset(&value, 0, sizeof(value));
    value.it_interval.tv_sec = 1;
    value.it_value.tv_sec    = 1;
    setitimer(ITIMER_REAL, &value, NULL); /* alarm reset after timer countdown */
}

void game_execute(WINDOW* menu_win, char* map)
{
    int c, enter = 0;

    /* press enter to start */
    mvprintw(5, 0, "                              ");
    while (enter != '\n' && enter != '\r'){
        enter = getchar();
    }
    refresh();

    timer_start();

    while (1){

        /* timer */
        wclear(menu_win);
        disp_timer(menu_win);
        print_menu(menu_win, map);

        /* detect key input and do correspond action */
        c = wgetch(menu_win);
		switch(c) {
            case KEY_UP:
                if (rlocation > 0)  rlocation--;
				break;
			case KEY_DOWN:
                if (rlocation < 5)  rlocation++;
				break;
            case KEY_LEFT:
                if (clocation > 0)  clocation--;
				break;
            case KEY_RIGHT:
                if (clocation < 5)  clocation++;
				break;
            case 'd':   move_leftright(map, rlocation, 0);
                        break;
            case 's':   move_leftright(map, rlocation, 1);
                        break;
            case 'e':   move_updown(map, clocation, 1);
                        break;
            case 'x':   move_updown(map, clocation, 0);
                        break;
			case 'q':
				quitflag = 1;
				break;
			default:
				//mvprintw(24, 0, "Charcter pressed is = %3d Hopefully it can be printed as '%c'", c, c);
				refresh();
				break;
		}
		state_judge(map);
		/* print_menu(menu_win, map); // Print new states after state judge */
		if(sec >= LIFETIME){
            print_result();
            break;
        }
        /* User types q to come out of the infinite loop */
		if(quitflag != 0)
			break;
	}
}

void move_updown(char *map, int column_number, int ud_flag)
{
    char bufferqueue[6];
    int i, j = 0;

    for (i = 0; i < 6; i++)
        bufferqueue[i] = ' ';

    /* put elements into queue for moving up */
    if (ud_flag){
        for (i = 0; i < 36; ++i){
            if (i % 6 == column_number && map[i] != ' ')
            bufferqueue[j++] = map[i];
        }
    }
    /* put elements into queue for moving down */
    if (!ud_flag){
        for (i = 35; i >= 0; --i){
            if (i % 6 == column_number && map[i] != ' ')
            bufferqueue[5-j++] = map[i];
        }
    }
    /* put elements in queue back to board */
    for (i = 0, j = 0; i < 36; i++){
        if (i % 6 == column_number){
            map[i] = bufferqueue[j++];
        }
    }

    return;
}

void move_leftright(char *map, int row_number, int lr_flag)
{
    char bufferqueue[6];
    int i, j;               // for loops only

    for (i = 0; i < 6; i++)
    bufferqueue[i] = ' ';

    /* put elements into queue for moving left */
    if (lr_flag){
        for (i = 0; i < 36; ++i){
            if (i / 6 == row_number && map[i] != ' ')
            bufferqueue[j++] = map[i];
        }
    }
    /* put elements into queue for moving right */
    if (!lr_flag){
        for (i = 35; i >= 0; --i){
            if (i / 6 == row_number && map[i] != ' ')
            bufferqueue[5-j++] = map[i];
        }
    }
    /* ut elements in queue back to board */
    for (i = 0, j = 0; i < 36; i++){
        if (i / 6 == row_number){
            map[i] = bufferqueue[j++];
        }
    }

    return;
}

void state_judge(char *map)
{
    int i, j, newspace_to_fill = 0;

    multiclear = 0;

    /* Check row clear state and clean corresponding row */
    for (i = 0; i < 6; i++)
        if (map[i*6+0] == map[i*6+1] && map[i*6+1] == map[i*6+2] && map[i*6+2] == map[i*6+3] &&
            map[i*6+3] == map[i*6+4] && map[i*6+4] == map[i*6+5] && map[i*6+0] != ' '){
            multiclear++;
            for (j = 0; j < 6; j++)
                map[i*6+j] = ' ';
        }
    /* Check column clear state and clean corresponding column */
    for (i = 0; i < 6; i++)
        if (map[i+6*0] == map[i+6*1] && map[i+6*1] == map[i+6*2] && map[i+6*2] == map[i+6*3] &&
            map[i+6*3] == map[i+6*4] && map[i+6*4] == map[i+6*5] && map[i+6*0] != ' '){
            multiclear++;
            for (j = 0; j < 6; j++)
                map[i+6*j] = ' ';
        }

    /* Add Score */
    switch (multiclear){
	    case 1: score++;
                break;
        case 2: score += 4;
                break;
        case 3: score += 8;
                break;
        default: ;
	    }

    line += multiclear;

    /* Supply new elements */
    while (newspace_to_fill < multiclear * 6){
        i = rand() % 36;
        if (map[i] == ' '){

        if (line < 10)                      j = rand() % 2;
        else if (line > 9 && line < 15)     j = rand() % 3;
        else                                j = rand() % 4;

		if (j == 0)      map[i] = 'T';
		else if (j == 1) map[i] = 'A';
		else if (j == 2) map[i] = 'R';
		else             map[i] = 'U';
        newspace_to_fill++;
        }
	}// keep 24 elements in 6 * 6 array


    return;
}

void print_result(void)
{
    printf("\n******************** GAME OVER ********************\n\n");
    printf("You cleared %d lines\n", line);
	printf("Final score: %d", score);
	if (score >= 50) printf(" Level: SS\n");
	else if (score >= 40 && score <= 49) printf(" Level: S\n");
	else if (score >= 35 && score <= 39) printf(" Level: A\n");
	else if (score >= 30 && score <= 34) printf(" Level: B\n");
	else if (score >= 26 && score <= 29) printf(" Level: C\n");
	else if (score >= 22 && score <= 25) printf(" Level: D\n");
	else if (score >= 18 && score <= 21) printf(" Level: E\n");
	else if (score >= 14 && score <= 17) printf(" Level: F\n");
	else if (score >= 10 && score <= 13) printf(" Level: G\n");
	else printf(" Level: Unranked\n");

	highscore_memo();

	return;
}

void highscore_memo(void)
{/*
    FILE *fptr;
    char there_was_error = 0;
    char opened_in_read = 1;

    fptr = fopen("highscore.txt", "rb+");

    if (fptr == NULL) // if file does not exist, create it
    {
        opened_in_read = 0;
        fptr = fopen("highscore.txt", "wb");
        if (fptr == NULL)
            there_was_error = 1;
    }

    if (there_was_error)
    {
        printf("Disc full or no permission\n");
        return EXIT_FAILURE;
    }

    if (opened_in_read)
        printf("The file is opened in read mode, read cache data\n");
    else
        printf("The file is opened in write mode\n");
    return EXIT_SUCCESS;*/
}
