#include <GL/glut.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <GL/glext.h>
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
#else
#define closesocket close
#endif


#include "colour.h"
#include "networking.h"

#include "game.h"
#include "mesh.h"

#define LWIDTH 0.0012
#define STARW 0.006

int **board;
int xfields, yfields, rq_sock, sock;
int player, turn;
int caps[] = { 0, 0 };
int passCount = 0;
double dx, dy;
int mainmenu;
bool online = true;
int previewX, previewY;

Mesh mesh;
TurnSequence turnSequence;

void play_stones(std::vector<Move> & stones);
void play_move(Move & move);
bool place_stone(int x, int y, int side, bool dryRun = false);
bool check_liberties(int x, int y, int side);
int handle_flags(bool removePiece);
bool captured_pending_stone();
bool is_legal_move(int x, int y);
bool has_legal_adjacent_move(int x, int y);
bool has_friendly_neighbour(int x, int y);
bool adjacent(int x0, int y0, int x1, int y1);
bool preview_stone(int x, int y);
void right_menu(int element);
void display_caps();
void next_turn();
void reset_turn();
std::pair<int, int> screen_to_board_coordinates(int sx, int sy);

void glRectangle(const Colour & c, float x0, float y0, float x1, float y1) {
	glBegin(GL_POLYGON);
	glColor3f(c.r, c.g, c.b);
	glVertex3f(x0, y0, 0);
	glVertex3f(x0, y1, 0);
	glVertex3f(x1, y1, 0);
	glVertex3f(x1, y0, 0);
	glEnd();
}

void draw_board() {
	int i;
	glRectangle(boardColour, -1, -1, 1, 1);
	for (i = 0; i < xfields; i++) {
		glRectangle(lineColour, -0.9-LWIDTH, -0.9-LWIDTH+i*dy, 0.9+LWIDTH, -0.9+LWIDTH+i*dy);
		glRectangle(lineColour, -0.9-LWIDTH+i*dx, -0.9-LWIDTH, -0.9+LWIDTH+i*dx, 0.9+LWIDTH);
	}

	int delta, orig;
	int mask = 0;
	if (xfields == 19) {
		orig = 3;
		delta = 6;
		glPointSize(5);
	} else if (xfields == 13) {
		orig = 3;
		delta = 3;
		mask = 1;
		glPointSize(5);
	} else {
		orig = 2;
		delta = 4;
		glPointSize(7);
	}

	glBegin(GL_POINTS);
	glColor3f(lineColour.r, lineColour.g, lineColour.b);

	for (int x = orig; x < xfields - 1; x += delta) {
		for (int y = orig; y < xfields - 1; y += delta) {
			if (((x + y) & mask) == 0)
				glVertex3f(-0.9+x*dx, -0.9+y*dy, 0);
		}
	}
	glEnd();
}

void game_init (int argc, std::string argv[])
{
	xfields = yfields = 9;

	mainmenu = glutCreateMenu(right_menu);
	player = turn = 0;
	dx = 1.8/(xfields-1);
	dy = 1.8/(yfields-1);

	board = new int*[yfields];
	for(int y=0;y<yfields; y++) {
		board[y] = new int[xfields];
		for(int x=0; x<xfields; x++)
			board[y][x] = 0;
	}

	// should be substituted by a os_init or win_init function.
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
	{
		fprintf (stderr, "WSAStartup failed.\n");
		exit (EXIT_FAILURE);
	}

	glGenBuffers = (PFNGLGENBUFFERSPROC)
		wglGetProcAddress ("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)
		wglGetProcAddress ("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)
		wglGetProcAddress ("glBufferData");
#endif

	if (argc == 0) {
		printf("Playing hotseat\n");
		online = false;
	}
	else if (argc == 1){
		printf("Hosting...\n");
		player = 0;
		rq_sock = init_server();
		std::cerr << rq_sock;
		sock = accept_or_die(rq_sock);
		glutSetWindowTitle ("Go - Server");
	}
	else if (argc == 2){
		printf("Connecting...\n");
		player = 1;
		sock = init_client(argv[1].data());
		glutSetWindowTitle ("Go - Client");
	}
	else {
		printf ("Usage: %s [ip]\n", argv[0].data());
		exit (EXIT_FAILURE);
	}

	if (online)
		printf("Socket: %d\n", sock);

	mesh.load("stone.obj");
	reset_turn();
}

std::pair<int, int> screen_to_board_coordinates(int sx, int sy) {
	double xd = (double(sx) / SWIDTH - 0.5) * 2.0 + 0.9;
	double yd = (double(sy) / SHEIGHT - 0.5) * 2.0 + 0.9;

	double f = (xfields - 1) / 1.8;
	return std::pair<int, int>(int(xd * f + 1.5) - 1, int(yd * f + 1.5) - 1);
}

void set_board_position(int x,int y)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-0.9+x*dx, -0.9+y*dy, 0);
}

void render_preview() {
	if (previewX >= 0) {
		set_board_position(previewX, previewY);
		Colour & c = turnSequence.nextStoneColour ? whiteColour : blackColour;
		mesh.render(c.r, c.g, c.b, 0.4);
	}
	for (Move & move : turnSequence.stonesPlayed) {
		set_board_position(move.x, move.y);
		Colour & c = move.stoneColour ? whiteColour : blackColour;
		mesh.render(c.r, c.g, c.b, 0.4);
	}
}

void game_display() {
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1,1,1,-1);
	//gluPerspective(60.0, 1, 0.01, 100);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//gluLookAt(1,1,1, 0,0,0, 0,0,1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	draw_board();
	render_preview();

	for(int x=0; x<xfields; x++)
		for(int y=0; y<yfields; y++) {
			if(board[y][x]) {
				set_board_position(x,y);
				int bval = board[y][x]-1;
				Colour & c = bval ? whiteColour : blackColour;
				mesh.render(c.r, c.g, c.b);
			}
		}
	glutSwapBuffers();
}

void game_keyboard(unsigned char key, int x, int y) {
	switch(key) {
		case 27:
			closesocket(sock);
			exit(EXIT_SUCCESS);
		default:
			break;
	}
}

void right_menu(int element) {

}

void play_stones(std::vector<Move> & stones) {
	if (turnSequence.alternating && stones.size() > 1) {
		for (Move & move : stones)
			board[move.y][move.x] = move.stoneColour + 1;
		bool surrounded = !check_liberties(stones.back().x, stones.back().y, stones.back().stoneColour + 1);
		handle_flags(false);
		for (Move & move : stones)
			board[move.y][move.x] = 0;

		if (surrounded) {
			// Opponent stone is surrounded - put it on the board without capturing stones
			auto it = stones.rbegin();
			Move & move = *it++;
			board[move.y][move.x] = move.stoneColour + 1;
			play_move(*it);
		} else {
			for (auto it = stones.rbegin(); it != stones.rend(); ++it)
				play_move(*it);
		}
	} else {
		for (Move & move : stones)
			play_move(move);
	}
}

void play_move(Move & move) {
	board[move.y][move.x] = move.stoneColour + 1;
	place_stone(move.x, move.y, move.stoneColour);
}

void game_idle() {
	if (!online)
		return;
	sPlayerAction action;
	get_command(sock, &action);
	switch (action.command) {
		case CmdPut:
			play_stones(action.moves);
			next_turn();
			glutPostRedisplay();
			break;
		case CmdPass:
			std::cout << "Pass" << std::endl;
			passCount++;
			break;
		default:
			break;
	}
}

void game_mouse(int b, int z, int x, int y) {
	if(!z || (online && turn != player))
		return;

	auto [iX, iY] = screen_to_board_coordinates(x, y);

	if(iX < xfields && iY < yfields && iX>=0 && iY>=0) {
		switch(b)
		{
			case GLUT_LEFT_BUTTON:
				if (iX != previewX || iY != previewY) {
					preview_stone(iX, iY);
					glutPostRedisplay();
					return;
				}

				turnSequence.instertStone(iX, iY);
				previewX = -1;
				previewY = -1;
				if (turnSequence.stonesPlayed.size() < turnSequence.stonesToPlay && has_legal_adjacent_move(iX, iY))
					break;

				play_stones(turnSequence.stonesPlayed);

				if (online) {
					sPlayerAction action;
					action.command = CmdPut;
					action.moves = turnSequence.stonesPlayed;
					send_command(sock, action);
				}
				next_turn();
				break;

			case GLUT_RIGHT_BUTTON:
				if (turnSequence.stonesPlayed.empty())
					return;
				turnSequence.stonesPlayed.clear();
				turnSequence.nextStoneColour = turn;
				break;
		}
		glutPostRedisplay();
	}
}

void game_mouse_move(int x, int y) {
	if (online && turn != player) {
		return;
	}

	auto [iX, iY] = screen_to_board_coordinates(x, y);

	if (preview_stone(iX, iY))
		glutPostRedisplay();
}

void game_mouse_click_move(int x, int y) {
	auto [iX, iY] = screen_to_board_coordinates(x, y);
	if (iX != previewX || iY != previewY) {
		previewX = -1;
		previewY = -1;
		glutPostRedisplay();
	}
}

bool preview_stone(int x, int y) {
	if (x < xfields && y < yfields && x >= 0 && y >= 0) {
		if (x != previewX || y != previewY) {
			if (!is_legal_move(x, y)) {
				x = -1;
				y = -1;
			}
		}
	} else {
		x = -1;
		y = -1;
	}

	if (x != previewX || y != previewY) {
		previewX = x;
		previewY = y;
		return true;
	}
	return false;
}

bool is_legal_move(int x, int y) {
	if (board[y][x])
		return false;

	bool isLegal = turnSequence.stonesPlayed.empty();

	for (Move & move : turnSequence.stonesPlayed) {
		if (x == move.x && y == move.y)
			// Occupied by a pending stone
			return false;
		else if (adjacent(x, y, move.x, move.y))
			isLegal = true;
	}

	if (!isLegal)
		return false;

	for (Move & move : turnSequence.stonesPlayed) {
		board[move.y][move.x] = move.stoneColour + 1;
	}
	board[y][x] = turnSequence.nextStoneColour + 1;

	if (turnSequence.alternating && !turnSequence.stonesPlayed.empty()) {
		Move & move = turnSequence.stonesPlayed.front();
		isLegal = !place_stone(move.x, move.y, move.stoneColour, true);
	} else {
		isLegal = !place_stone(x, y, turnSequence.nextStoneColour, true);
	}

	board[y][x] = 0;
	for (Move & move : turnSequence.stonesPlayed) {
		board[move.y][move.x] = 0;
	}

	return isLegal;
}

bool place_stone(int x, int y, int side, bool dryRun){
	int nx, ny;
	static int koX = -1, koY = -1;
	if (koX == x && koY == y) {
		return true;
	}
	int captures;
	int hremove = 0;
	bool hasLiberty, capturedOpponent;
	for (int i = 0; i < 4; i++) {
		nx = x+(i==1)-(i==0);
		ny = y+(i==3)-(i==2);
		if (nx >= 0 && nx < xfields && ny >= 0 && ny < yfields) {
			hasLiberty = check_liberties(nx, ny, board[ny][nx]);
			capturedOpponent = !hasLiberty && (board[ny][nx] & 3) != side + 1;
			captures = capturedOpponent * handle_flags(!dryRun && capturedOpponent);
			if (captures > 0) {
				hremove += captures;
				if (!dryRun && hremove == 1 && koX == -1) {
					koX = nx;
					koY = ny;
				}
			}
		}

	}
	if (!dryRun && hremove > 0) {
		caps[side] += hremove;
		display_caps();
	}
	hasLiberty = check_liberties(x, y, board[y][x]);
	if (!hasLiberty)
		x = x;
	int tmp = handle_flags(false);
	if (!dryRun && (tmp > 1 || hremove != 1 || has_friendly_neighbour(x, y))) {
		koX = -1;
		koY = -1;
	}

	return hremove == 0 && !hasLiberty;
}

int handle_flags(bool removePiece){
	int r = 0;
	for (int i = 0; i < xfields; i++) {
		for (int j = 0; j < yfields; j++) {
			if (board[i][j] & 4) {
				r++;
				if (removePiece) {
					board[i][j] = 0;
				} else {
					board[i][j] %= 4;
				}
			}
		}
	}
	return r;
}

bool captured_pending_stone() {
	for (Move & move : turnSequence.stonesPlayed)
		if (board[move.y][move.x] & 4)
			return true;
	return false;
}

bool check_liberties(int x, int y, int side) {
	if (x < 0 || x >= xfields || y < 0 || y >= yfields) { 
		return false;
	}
	if (board[y][x] == 0) {
		return true;
	} else if (board[y][x]&4) { // check visited flag
		return false;
	} else if (board[y][x] == side) {
		board[y][x] |= 4; // set visited flag
		int nx, ny;
		for (int i = 0; i < 4; i++) {
			nx = x+(i==1)-(i==0);
			ny = y+(i==3)-(i==2);
			if (check_liberties(nx, ny, side)) {
				return true;
			}
		}
		return false;
	} else {
		return false;
	}
}

bool has_legal_adjacent_move(int x, int y) {
	int nx, ny;
	for (int i = 0; i < 4; i++) {
		nx = x + (i == 1) - (i == 0);
		ny = y + (i == 3) - (i == 2);
		if (nx < 0 || ny < 0 || nx >= xfields || ny >= yfields)
			continue;
		if (is_legal_move(nx, ny))
			return true;
	}
	return false;
}

bool has_friendly_neighbour(int x, int y) {
	int nx, ny;
	for (int i = 0; i < 4; i++) {
		nx = x + (i == 1) - (i == 0);
		ny = y + (i == 3) - (i == 2);
		if (nx < 0 || ny < 0 || nx >= xfields || ny >= yfields)
			continue;
		if (board[ny][nx] == board[y][x])
			return true;
	}
	return false;
}

bool adjacent(int x0, int y0, int x1, int y1) {
	if (x0 == x1) {
		int dY = y1 - y0;
		if (dY == 1 || dY == -1)
			return true;
	}
	else if (y0 == y1) {
		int dX = x1 - x0;
		if (dX == 1 || dX == -1)
			return true;
	}
	return false;
}


void TurnSequence::instertStone(int x, int y) {
	stonesPlayed.emplace_back(x, y, nextStoneColour);
	if (alternating)
		nextStoneColour = 1 - nextStoneColour;
}

void next_turn() {
	turn = 1 - turn;
	turnSequence.nextStoneColour = turn;
	turnSequence.stonesPlayed.clear();
	previewX = -1;
	previewY = -1;
}

void reset_turn() {
	turn = 1;
	next_turn();
}

void display_caps() {
	std::cout << "Caps:" << std::endl;
	std::cout << "Black: " << caps[0] << std::endl;
	std::cout << "White: " << caps[1] << std::endl << std::endl;
}
