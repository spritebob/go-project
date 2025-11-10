#include <conio.h>
#include <iostream>
#include <string>

#include <GL/glut.h>

#include "game.h"

void init_gl() {
	glClearColor (1.0, 1.0, 1.0, 1.0); // Set background to white
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);

	std::cout << "Play online (y/n)? ";
	char c;
	do {
		c = tolower(getch());
	} while (c != 'y' && c != 'n');
	std::cout << c << std::endl;
	if (c == 'n') argc = 0;

	std::string args[2];
	args[0] = argv[0];
	if (argc == 1) {
		std::cout << "Enter host IP / leave blank to host" << std::endl;
		std::getline(std::cin, args[1]);
		if (!args[1].empty())
			argc++;
	}
	else if (argc == 2) {
		args[1] = argv[1];
	}

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(SWIDTH,SHEIGHT);
	glutCreateWindow("Go");
	glutDisplayFunc(game_display);
	glutMouseFunc(game_mouse);
	glutPassiveMotionFunc(game_mouse_move);
	glutMotionFunc(game_mouse_click_move);
	glutKeyboardFunc(game_keyboard);
	glutIdleFunc(game_idle);

	init_gl();
	game_init (argc, args);

	glutMainLoop();
	return 0;
}
