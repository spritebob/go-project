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
void game_mouse_click_move(int x, int y);

enum Command { CmdNothing = 0, CmdPut, CmdPass };

struct Move {
	Move(int x, int y, int stoneColour)
		: x(x), y(y), stoneColour(stoneColour)
	{ ; }

	int x, y, stoneColour;
};

struct TurnSequence {
	void instertStone(int x, int y);

	int nextStoneColour = 0;
	std::vector<Move> stonesPlayed;
	const bool alternating = true;
	const int stonesToPlay = 2;
};

struct sPlayerAction {
	Command command;
	std::vector<Move> moves;
};

#endif
