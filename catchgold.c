/**
 * catchgold v0.2
 *
 * Copyleft 2003, Andrew Gwozdziewycz <git@apgwoz.com>  
 * released under the GNU General Public License 
 *
 * CHANGELOG
 *-----------
 * v0.2 -
 *   -Redesigned the enemy ai. Basically it's impossible. 
 *   -Implemented weapons. If you are fortunate enough to get a weapon
 *      the agents chasing you get blown away (hit with huge fans)
 *      and sent randomly to different places. This brings back agents
 *      who decided to run together. Weapons spawn randomly and by default
 *      only one Weapon will spawn at a given time. 
 *   -Using random and srandom for randoms rather than rand and srand.
 *   -Eliminated getrand(int) function by creating a macro (getrand(x))
 *     
 *
 * v0.1 - 
 *   -Initial release, fully functional game.
 *
 */

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <ncurses.h>

#define BOARD_PLAIN ' '
#define BOARD_GOLD '$'
#define BOARD_ENEMY '%'
#define BOARD_TELEPORT 'T'
#define BOARD_WEAPON 'W'
#define PLAYER_INITIAL_TELEPORTS 1
#define MAX_GOLD 3 
#define MAX_ENEMIES 2
#define MAX_TELEPORTS 1
#define MAX_LIVES 3
WINDOW *win;

#ifndef min
#define min(x,y) ((x < y) ? x: y)
#endif

#ifndef getrand
#define getrand(x) (random() % x)
#endif

#define _STRUCT_ITEM_
struct item {
   int x;
   int y; 
};

void print_board(void);
void check_board(int, int);
void update_gold(void);
void update_teleports(void);
void update_weapons(void);
void update_enemies(void);
void create_enemies(void);
void do_death_noise(void);

void loop(void);
void gameover();
void teleport(void);

static int player_score = 0;
static int player_lives = 0;
static int player_char = '<';

#ifdef PLAYER_INITIAL_TELEPORTS
static int player_teleports = PLAYER_INITIAL_TELEPORTS;
#else
static int player_teleports = 1;
#endif

static int player_weapon = 0;
static int board_teleports = 0;
static int board_weapons = 0;
static int board_gold = 0;
static int game_speed = 1000;
static int width = 60;
static int height = 24;
static char board[60][24];
static char last;
static int curx;
static int cury;

static struct item enemies[MAX_ENEMIES];

int main()
{
   struct timeval s;
   initscr();
   cbreak();
   noecho();
   win = newwin(LINES, COLS, 0, 0);
   wrefresh(win);
   wtimeout(win, game_speed); /* block on input for 1 second max */
   keypad(win, TRUE);

   /* seed our randoms */
   gettimeofday(&s, NULL);
   srandom(s.tv_usec);

   player_lives = MAX_LIVES;
   board_teleports = 0;
   teleport(); 

   loop();

   delwin(win);
   endwin();
   exit(0);
}

void loop(void)
{
   register int x;
   register int y;

   for (y = 0; y < height; y++)  {
      for (x = 0; x < width; x++) {
         board[x][y] = BOARD_PLAIN;
      }
   }

   create_enemies();
   update_teleports();
   update_gold();
   print_board();

   while ((x = wgetch(win)) != 'q' && player_lives > 0) {
      update_teleports();
      update_gold();
      update_weapons();
      switch(x) {
          case KEY_LEFT:
          case 'h':
             if ((curx - 1) >= 0) {
               last = board[curx][cury];
               board[curx][cury] = BOARD_PLAIN;
               curx--; 
               check_board(curx, cury);
               board[curx][cury] = '<';
               player_char = '<';
             }
             break;
          case KEY_RIGHT:
          case 'l':
             if ((curx + 1) < width) {
               last = board[curx][cury];
               board[curx][cury] = BOARD_PLAIN;
               curx++; 
               check_board(curx, cury);
               board[curx][cury] = '>';
               player_char = '>';
             }
             break;
          case KEY_DOWN:
          case 'j':
             if ((cury + 1) < height) {
               last = board[curx][cury];
               board[curx][cury] = BOARD_PLAIN;
               cury++; 
               check_board(curx, cury);
               board[curx][cury] = 'v';
               player_char = 'v';
             }
             break;
          case KEY_UP:
          case 'k':
             if ((cury - 1) >= 0) {
               last = board[curx][cury];
               board[curx][cury] = BOARD_PLAIN;
               cury--;
               check_board(curx, cury);
               board[curx][cury] = '^';
               player_char = '^';
             }
             break;
          case 't':
          case 'T':
             if (player_teleports > 0) {
               player_teleports--; 
               teleport();
             }
             break;
          default:
             break;
      }
      print_board();
      update_enemies(); 
      check_board(curx,cury);
   }
} 

void print_board(void)
{
   int i,j;
   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         mvwprintw(win, i, j, "%c", board[j][i]);
      }
      mvwprintw(win, i, j, "|");
   }
    
   for (i = 0; i < MAX_ENEMIES; i++) {
      last = board[enemies[i].x][enemies[i].y];
      mvwprintw(win, enemies[i].y, enemies[i].x, "%c", BOARD_ENEMY);
      board[enemies[i].x][enemies[i].y] = last;
   }

   mvwprintw(win, 5, 65, "Score: ");
   mvwprintw(win, 6, 65, "%d", player_score); 
   mvwprintw(win, 8, 65, "Lives: ");
   mvwprintw(win, 9, 65, "%d", player_lives); 
   mvwprintw(win, 11, 65, "Teleports:");
   mvwprintw(win, 12, 65, "%d", player_teleports);
   mvwprintw(win, 13, 65, "Weapon? %c", (player_weapon) ? 'Y': 'N');
   wrefresh(win);
}

void check_board(int x, int y)
{
   int i;
   switch(board[x][y]) {
      case BOARD_GOLD:
         player_score += 10;
         --board_gold;
         return;
      case BOARD_TELEPORT:
         player_teleports++;
         --board_teleports;
         return;
      case BOARD_WEAPON:
         player_weapon = 1;
         board_weapons = 0;
         return;
      default:
         break;
   }
   /* do enemy collision check */
   for (i = 0; i < MAX_ENEMIES; i++) {
      if (enemies[i].x == curx &&
          enemies[i].y == cury) {
        if (!player_weapon) {
          player_lives--; 
          /* reset character here */
          board[enemies[i].x][enemies[i].y] = BOARD_PLAIN;
          player_teleports = PLAYER_INITIAL_TELEPORTS;
          teleport();
          do_death_noise();
          create_enemies();
        }
        else { /* HAS A WEAPON! STAND BACK! */
          player_weapon = 0;
          board[curx][cury] = player_char;
          create_enemies();
        }
        break;
      }
   }
   if (player_lives == 0) {
     gameover();
   }
}

void create_enemies(void)
{
   int i;
   for (i = 0; i < MAX_ENEMIES; i++) {
      enemies[i].x = getrand(width);
      enemies[i].y = getrand(height);
      if (curx == enemies[i].x && 
          cury == enemies[i].y) {
        --i; /* redo this one */
      }
      usleep(4);
   }
}

void update_enemies(void)
{
   int diffx, diffy;
   int i;
   for (i = 0; i < MAX_ENEMIES; i++) {
      diffx = (int)abs( enemies[i].x - curx);
      diffy = (int)abs( enemies[i].y - cury);
      if (diffx > diffy) {
        if (enemies[i].x < curx) {
          enemies[i].x++;
        }
        else {
          enemies[i].x--;
        }
      }
      else {
        if (enemies[i].y < cury) {
          enemies[i].y++;
        }
        else {
          enemies[i].y--;
        }
      }
   }
}

/**************************************
***************************************
void update_enemies(void)
{
   int i;
   int diffy, diffx;
   for (i = 0; i < MAX_ENEMIES; i++) {
     diffx = enemies[i].x - curx;
     diffy = enemies[i].y - cury;
     if (enemies[i].x < curx && diffy != 0) {
        enemies[i].x++;
     }
     else if (enemies[i].x > curx && diffy != 0) {
       enemies[i].x--;
     }
     else if (enemies[i].y < cury && diffx != 0) {
       enemies[i].y++;
     }
     else if (enemies[i].y > cury && diffx != 0) {
       enemies[i].y--;
     }
     else if (diffy == 0) {
       if (enemies[i].x < curx) {
         enemies[i].x++;
       }
       else {
         enemies[i].x--; 
       }
     }
     else if (diffx == 0) {
       if (enemies[i].y < cury) {
         enemies[i].y++;
       }
       else {
         enemies[i].y--;
       }
     }
   }
}
***************************************
**************************************/


void teleport(void)
{
   board[curx][cury] = BOARD_PLAIN;
   curx = getrand(width);
   cury = getrand(height);
}

void update_teleports(void)
{
   int x, y;
   if (board_teleports < MAX_TELEPORTS) { 
     x = getrand(width);
     y = getrand(height); 
     if (board[x][y] == BOARD_PLAIN) {
       board[x][y] = BOARD_TELEPORT;
       board_teleports++;
     }
   }
}   

void update_gold(void)
{
   int x, y;
   if (board_gold < MAX_GOLD) {
     x = getrand(width);
     y = getrand(height); 
     if (board[x][y] == BOARD_PLAIN) {
       board[x][y] = BOARD_GOLD;
       board_gold++;
     }
   } 
}


void update_weapons(void)
{
   int i;
   int x, y;
   if (board_weapons == 0) {
     i = getrand(time(NULL));
   
     if ((i % 60) == 0) {
       x = getrand(width);
       y = getrand(height);
       if (board[x][y] == BOARD_PLAIN) {
         board_weapons = 1;
         board[x][y] = BOARD_WEAPON;
       }
     }
   }
}

void gameover(void) 
{
   print_board();
   mvwprintw(win, height/2, 24, "GAME OVER");
   wrefresh(win);
   sleep(4);   
}

void do_death_noise(void)
{
   /* may or may not work.... i just hope it does */
   printf("%c[10;200]", 0x1B);
   printf("%c[11;10]", 0x1B);
   printf("\a");
}

