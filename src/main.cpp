#include <iostream>
#include <string>

#include <GL/glut.h>

#include "game.h"

void init_gl() {
	glClearColor (1.0, 1.0, 1.0, 1.0); // Set background to white
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(SWIDTH,SHEIGHT);
	glutCreateWindow("Go");
	glutDisplayFunc(game_display);
	glutMouseFunc(game_mouse);
	glutKeyboardFunc(game_keyboard);
	glutIdleFunc(game_idle);
	
	std::string args[2];
	args[0] = argv[0];
	if (argc == 1) {
		std::cout << "Enter host IP / leave blank to host" << std::endl;
		std::getline(std::cin, args[1]);
		if (!args[1].empty())
			argc++;
	}
	else {
		args[1] = argv[1];
	}
	init_gl();
	game_init (argc, args);

	glutMainLoop();
	return 0;
}
