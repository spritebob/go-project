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


#include "networking.h"

#include "game.h"
#include "mesh.h"

#define LWIDTH 0.008

int **board;
int xfields, yfields, rq_sock, sock;
int player, turn;
double dx, dy;
int mainmenu;

Mesh mesh;

bool place_stone(int x, int y, int side);
bool check_freedoms(int x, int y, int side);
int handle_flags(bool removePiece);
void right_menu(int element);

void draw_board() {
	int i;
	glBegin(GL_POLYGON);
	glColor3f(0.8,0.8,0.5); 
	glVertex3f(-1, -1, 0);
	glVertex3f(-1, 1, 0);
	glVertex3f(1, 1, 0);
	glVertex3f(1, -1, 0);
	glEnd();
	for (i = 0; i < xfields; i++) {
		glBegin(GL_POLYGON);
		glColor3f(0.5,0.0,0.0); 
		glVertex3f(0.9+LWIDTH, -0.9-LWIDTH+i*dy, 0);
		glVertex3f(0.9+LWIDTH, -0.9+LWIDTH+i*dy, 0);
		glVertex3f(-0.9-LWIDTH, -0.9+LWIDTH+i*dy, 0);
		glVertex3f(-0.9-LWIDTH, -0.9-LWIDTH+i*dy, 0);
		glEnd();
		glBegin(GL_POLYGON);
		glColor3f(0.5,0.0,0.0); 
		glVertex3f(-0.9-LWIDTH+i*dx, 0.9, 0);
		glVertex3f(-0.9+LWIDTH+i*dx, 0.9, 0);
		glVertex3f(-0.9+LWIDTH+i*dx, -0.9, 0);
		glVertex3f(-0.9-LWIDTH+i*dx, -0.9, 0);
		glEnd();
	}
}

void game_init (int argc, std::string argv[])
{
	mainmenu = glutCreateMenu(right_menu);
	glutAddMenuEntry("Connect", 1);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
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

	if (argc < 2){
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

	printf("Socket: %d\n", sock);
	mesh.load("triangle.obj");

}

void set_board_position(int x,int y)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-0.9+LWIDTH+x*dx, -0.9+LWIDTH+y*dy, 0);
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

	for(int x=0; x<xfields; x++)
		for(int y=0; y<yfields; y++) {
			if(board[y][x]) {
				static float cols[] =
				{0,0,0, 1,1,1};

				set_board_position(x,y);
				int bval = board[y][x]-1;
				mesh.render(cols[bval*3], cols[bval*3+1], cols[bval*3+2]);
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
	sPlayerAction action;
	get_command(sock, &action);
	switch (action.command) {
		case CmdPut:
			turn = !turn;
			board[action.y][action.x] = (!player)+1;
			place_stone(action.x, action.y, (!player)+1);
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
	if(!z || turn != player) {
		return;
	}

	double xd = (double(x)/SWIDTH - 0.5)*2.0 + 0.9;
	double yd = (double(y)/SHEIGHT - 0.5)*2.0 + 0.9;

	int iX = xd/1.8*18.0 +0.5;
	int iY = yd/1.8*18.0 +0.5;

	sPlayerAction action;

	//printf("%g %g\n", xd,yd);
	if(iX < xfields && iY < yfields && iX>=0 && iY>=0) {
		switch(b)
		{
			case GLUT_LEFT_BUTTON:
				if (board[iY][iX] != 0) {
					return;
				}
				board[iY][iX] = player+1;
				if (place_stone(iX, iY, player+1)) {
					board[iY][iX] = 0;
					return;
				}
				action.command = CmdPut;
				action.x = iX;
				action.y = iY;
				turn = !turn;
				send_command(sock, action);
				break;

			case GLUT_RIGHT_BUTTON:
				break;
				board[iY][iX] = 0;
				action.command = CmdRemove;
				action.x = iX;
				action.y = iY;
				send_command(sock, action);
				break;
		}
		glutPostRedisplay();
	}
}

bool place_stone(int x, int y, int side){
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
		cf = check_freedoms(nx, ny, 3-side);
		hremove += !cf*handle_flags(!cf);
		if (hremove == 1 && removedX == -1) {
			removedX = nx;
			removedY = ny;
		}
	}
	cf = check_freedoms(x, y, side);
	int tmp = handle_flags(!cf);
	if (tmp > 1 || hremove != 1) {
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

bool check_freedoms(int x, int y, int side) {
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
			if (check_freedoms(nx, ny, side)) {
				return true;
			}
		}
		return false;
	} else {
		return false;
	}
}
