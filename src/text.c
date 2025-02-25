#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
int x;
int y;
int rows;
int cols;
char **lines;
char *error_log_path;
char *currentfile;
int lineindex = 0;
int nlines;
int XBOUNDS;
int YBOUND;
void changepage(int x) {
     if (x == 1) {
          if (lineindex >= nlines) {
               lineindex = nlines - 1;
               return;
          } else {
               for (int i = 1; i < XBOUNDS && lineindex + i < nlines; i++) {
                    move(i, 0);
                    clrtoeol();
                    attron(COLOR_PAIR(2));
                    move(i, YBOUND - 1);
                    // printw("|");
                    move(i, 0);
                    printw(" %3d", lineindex + i + 1);
                    attroff(COLOR_PAIR(2));
                    move(i, YBOUND);
                    clrtoeol();
                    printw("%s", lines[lineindex + i]);
                    refresh();
               }
          }
     } else {
          if (lineindex < 0) {
               lineindex = 0;
               return;
          } else {
               lineindex--;
               for (int i = 1; i < XBOUNDS && lineindex + i >= 0; i++) {
                    move(i, 0);
                    clrtoeol();
                    // attron(A_BOLD);
                    attron(COLOR_PAIR(2));
                    move(i, YBOUND - 1);
                    // printw("|");
                    move(i, 0);

                    printw(" %3d", lineindex + i + 1);
                    attroff(COLOR_PAIR(2));
                    // attroff(A_BOLD);

                    move(i, YBOUND);
                    clrtoeol();
                    printw("%s", lines[lineindex + i]);
                    refresh();
               }
          }
     }
}
void logger(char *message) {
     time_t rawtime;
     struct tm *timeinfo;

     time(&rawtime);
     timeinfo = localtime(&rawtime);

     FILE *error_log = fopen("error_log.txt", "a");
     if (error_log == NULL) {
          printw("ERROR OPENING LOG");
     }
     if (error_log != NULL) {
          fprintf(error_log, "%s ", asctime(timeinfo));
          fprintf(error_log, "%s", message);
     }
     fclose(error_log);
}

void init() {
     error_log_path = "error_log.txt";
     // currentfile = "error_log.txt";
     nlines = 0;
     lines = NULL;
     initscr();
     getmaxyx(stdscr, rows, cols);
     XBOUNDS = rows - 2;
     YBOUND = 5;
     if (curs_set(1) == ERR) { // make cursor rectangle
          logger("not supported");
          move(1, YBOUND);
          printw("not supported");
     }
     start_color();
     init_pair(1, COLOR_BLACK, COLOR_GREEN);
     init_pair(2, COLOR_GREEN, COLOR_BLACK);
     cbreak();
     noecho();
     clear();
     refresh();
     move(rows - rows, (cols / 2) - 4);
     attron(COLOR_PAIR(1));
     printw("REAL-VIM");
     attron(COLOR_PAIR(2));
     x = 1;
     y = YBOUND;
     for (int i = 1; i < rows - 1; i++) {
          move(i, 0);
          printw(" %3d", i);
          move(i, YBOUND - 1);
          // printw("|");
          refresh();
     }
     attroff(COLOR_PAIR(2));
     move(XBOUNDS, 0);
     attron(COLOR_PAIR(1));
     attron(A_BOLD);
     printw("VISUAL");
     move(XBOUNDS, 6);
     for (int i = 0; i < cols - 6; i++)
          addch(' ');
     attroff(A_BOLD);
     attroff(COLOR_PAIR(1));
     move(x, y);
     refresh();
}

void terminate() {
     endwin();
     free(lines);
}

void writer() {
     FILE *output = fopen(currentfile, "w");
     if (output == NULL) {
          logger("error opening");
          return;
     }
     for (int i = 0; i < nlines; i++) {
          if (fprintf(output, "%s\n", lines[i]) < 0) {
               logger("error writing");
               return;
          }
     }
     fclose(output);
}

void check_cursor() {
     if (y == cols - 1) {
          y = YBOUND;
          x++;
          move(x, y);
     } else if (y <= YBOUND) {
          y = YBOUND;
          move(x, y);
     }
     if (x <= 0) {
          x = 1;
          changepage(2);
          move(x, y);
     }
     if (x >= XBOUNDS) {
          x = rows - 3;
          changepage(1);
          move(x, y);
     }
}

void read_file(char path[]) {
     move(XBOUNDS, cols - strlen(path));
     attron(COLOR_PAIR(1));
     attron(A_BOLD);
     printw("%s", path);
     attroff(A_BOLD);
     attroff(COLOR_PAIR(1));
     refresh();
     move(1, YBOUND);
     x = 1;
     y = YBOUND;
     FILE *file = fopen(path, "r");
     if (!file) {
          logger("error opening input file");
          refresh();
          return;
     }
     logger("entered file");
     fseek(file, 0, SEEK_END);
     long size = ftell(file);
     rewind(file);
     char *text = malloc(size + 1);
     if (text == NULL) {
          printw("memory allocation failed");
          fclose(file);
          return;
     }
     fread(text, 1, size, file);
     text[size] = '\0';
     char *token;
     token = strtok(text, "\n");
     while (token) {
          if (x < XBOUNDS) {
               printw("%s\n", token);
               move(++x, YBOUND);
          }
          nlines++;
          lines = (char **)realloc(lines, nlines * sizeof(char *));
          if (lines == NULL) {
               logger("realloc failed");
               return;
          }
          lines[nlines - 1] = (char *)malloc((cols - YBOUND) * sizeof(char));
          if (lines[nlines - 1] == NULL) {
               logger("malloc fail");
               return;
          }
          strcpy(lines[nlines - 1], token);
          token = strtok(NULL, "\n");
     }
     move(1, YBOUND);
     refresh();
}

void display(char c) {
     move(x, y++);
     check_cursor();
     printw("%c", c);
     refresh();
}

int command_mode() {
     move(XBOUNDS, 0);
     attron(COLOR_PAIR(1));
     attron(A_BOLD);
     printw("COMMAND");
     attroff(A_BOLD);
     attroff(COLOR_PAIR(1));
     move(x, y);
     refresh();
     int cx = rows - 1;
     int cy = 1;
     move(cx, cy);
     char input[10] = {0};
     echo();
     getstr(input);
     for (int j = 0; j < 10; j++) {
          if (input[j] == 'q') {
               return 1;
          }
     }
     noecho();
     move(XBOUNDS, 0);
     attron(COLOR_PAIR(1));
     attron(A_BOLD);
     printw("VISUAL ");
     attroff(A_BOLD);
     attroff(COLOR_PAIR(1));
     move(x, y);
     refresh();
     move(rows - 1, 0);
     clrtoeol();
     move(x, y);
     refresh();
     return 0;
}

void insert_mode() {
     curs_set(1);
     move(XBOUNDS, 0);
     attron(COLOR_PAIR(1));
     attron(A_BOLD);
     printw("INSERT");
     attroff(A_BOLD);
     attroff(COLOR_PAIR(1));
     move(x, y);
     refresh();
     timeout(5000);
     char c = getch();
     while (c != 27) {
          if (c == 127) {
               move(x, --y);
               check_cursor();
               delch();
               refresh();
          } else if (c != ERR) {
               move(x, y++);
               check_cursor();
               refresh();
               insch(c);
          }
          timeout(5000);
          c = getch();
     }
     curs_set(2);
     move(XBOUNDS, 0);
     attron(COLOR_PAIR(1));
     attron(A_BOLD);
     printw("VISUAL");
     attroff(A_BOLD);
     attroff(COLOR_PAIR(1));
     move(x, y);
     refresh();
     noecho();
}

void move_cursor(char c) {
     switch (c) {
     case 'a':
          move(x, --y);
          break;
     case 'w':
          move(--x, y);
          break;
     case 'd':
          move(x, ++y);
          break;
     case 's':
          move(++x, y);
          break;
     }
     check_cursor();
     refresh();
}
int main(int argc, char *argv[]) {
     // rows = atoi(argv[1]);
     //  cols = atoi(argv[2]);
     //  writer();
     init();
     currentfile = argv[1];
     read_file(currentfile);
     x = 1;
     y = YBOUND;
     move(1, YBOUND);
     char c;
     read(STDIN_FILENO, &c, 1);
     while (1) {
          if (c == 'i')
               insert_mode();
          else if (c == ':') {
               move(rows - 1, 0);
               printw(":");
               refresh();
               if (command_mode() == 1) {
                    terminate();
                    return 0;
               }
          } else if (c >= 49 && c <= 57)
               changepage(c - '0');
          else
               move_cursor(c);
          refresh();
          timeout(5000);
          read(STDIN_FILENO, &c, 1);
     }
}
