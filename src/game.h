/*
 * Defines some game specific functions
 */

#ifndef _GAME_H
#define _GAME_H

#define SWIDTH 700
#define SHEIGHT 700

#include <vector>


void game_init (int argc, std::string argv[]);
void game_display ();
void game_keyboard (unsigned char key, int x, int y);
void game_idle ();
void game_mouse (int b, int z, int x, int y);
void game_mouse_move(int x, int y);

enum Command {CmdNothing=0, CmdPut, CmdRemove};

struct sPlayerAction{
	Command command;
	int x, y;
};

struct Move {
	int x, y, stoneColour;
};

struct TurnSequence {
	int nextStoneColour = 0;
	std::vector<Move> stonesPlayed;
};

#endif
