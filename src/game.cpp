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

#define LWIDTH 0.002
#define STARW 0.008

int **board;
int xfields, yfields, rq_sock, sock;
int player, turn;
double dx, dy;
int mainmenu;
bool online = true;
int previewX, previewY;

Mesh mesh;
TurnSequence turnSequence;

bool place_stone(int x, int y, int side, bool dryRun = false);
bool check_liberties(int x, int y, int side);
int handle_flags(bool removePiece);
bool is_legal_move(int x, int y);
bool preview_stone(int x, int y);
void right_menu(int element);
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
	for (int x = 3; x < xfields; x += 6) {
		for (int y = 3; y < xfields; y += 6) {
			glRectangle(lineColour, -0.9-STARW+x*dx, -0.9-STARW+y*dy, -0.9+STARW+x*dx, -0.9+STARW+y*dy);
		}
	}
}

void game_init (int argc, std::string argv[])
{
	mainmenu = glutCreateMenu(right_menu);
	player = turn = 0;
	xfields = yfields = 19;
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

	return std::pair<int, int>(int(xd * 10.0 + 1.5) - 1, int(yd * 10.0 + 1.5) - 1);
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

void game_idle() {
	if (!online)
		return;
	sPlayerAction action;
	get_command(sock, &action);
	switch (action.command) {
		case CmdPut:
			board[action.y][action.x] = 2 - player;
			place_stone(action.x, action.y, 2 - player);
			next_turn();
			glutPostRedisplay();
			break;
		case CmdRemove:
			board[action.y][action.x] = 0;
			glutPostRedisplay();
			break;
		default:
			break;
	}
}

void game_mouse(int b, int z, int x, int y) {
	if(!z || (online && turn != player) || previewX < 0) {
		return;
	}

	auto [iX, iY] = screen_to_board_coordinates(x, y);

	if (iX != previewX || iY != previewY) {
		preview_stone(iX, iY);
		glutPostRedisplay();
		return;
	}

	sPlayerAction action;

	//printf("%g %g\n", xd,yd);
	if(iX < xfields && iY < yfields && iX>=0 && iY>=0) {
		switch(b)
		{
			case GLUT_LEFT_BUTTON:
				if (board[iY][iX] != 0) {
					return;
				}
				board[iY][iX] = turn + 1;
				if (place_stone(iX, iY, turn+1)) {
					board[iY][iX] = 0;
					return;
				}
				action.command = CmdPut;
				action.x = iX;
				action.y = iY;
				next_turn();
				if (online)
					send_command(sock, action);
				break;

			case GLUT_RIGHT_BUTTON:
				break;
				board[iY][iX] = 0;
				action.command = CmdRemove;
				action.x = iX;
				action.y = iY;
				if (online)
					send_command(sock, action);
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

bool preview_stone(int x, int y) {
	if (x < xfields && y < yfields && x >= 0 && y >= 0) {
		if (!is_legal_move(x, y)) {
			x = -1;
			y = -1;
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
	for (Move & move : turnSequence.stonesPlayed) {
		board[move.y][move.x] = move.stoneColour;
	}

	bool isLegal = !place_stone(x, y, turnSequence.nextStoneColour, true);

	for (Move & move : turnSequence.stonesPlayed) {
		board[move.y][move.x] = 0;
	}

	return isLegal;
}

bool place_stone(int x, int y, int side, bool dryRun){
	int nx, ny;
	static int removedX = -1, removedY = -1;
	//fprintf(stderr, "%d %d %d %d", removedX, removedY, x, y);
	if (removedX == x && removedY == y) {
		return true;
	}
	int hremove = 0;
	bool cf;
	for (int i = 0; i < 4; i++) {
		nx = x+(i==1)-(i==0);
		ny = y+(i==3)-(i==2);
		cf = check_liberties(nx, ny, 3-side);
		hremove += !cf*handle_flags(!dryRun && !cf);
		if (!dryRun && hremove == 1 && removedX == -1) {
			removedX = nx;
			removedY = ny;
		}
	}
	cf = check_liberties(x, y, side);
	int tmp = handle_flags(!cf);
	if (!dryRun && (tmp > 1 || hremove != 1)) {
		removedX = -1;
		removedY = -1;
	}
	//fprintf(stderr, " %d %d\n", tmp, hremove);
	return !cf;
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
