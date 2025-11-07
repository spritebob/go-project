#ifndef MESH_H
#define MESH_H

#include <GL/gl.h>

class Mesh {
	public:
	/**
	 * Loads mesh data from a .obj file.
	 * @param vbo vertex buffer object handle pointer. Should be released manually
	 * later.
	 * @param ebo element buffer object handle pointer. Should be released manually
	 * later.
	 * @param filename filename for .obj file.
	 */
	void load(const char *filename);

	/**
	 * Renders a mesh file.
	 */
	void render(float r,float g, float b, float a = 1.0);
	
	private:
		GLsizei nIndex;
		
		GLuint vbo; ///< Vertex buffer
		GLuint ebo; ///< Element buffer
};

#endif
