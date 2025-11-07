#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
#else /* !_WIN32 */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif /* !_WIN32 */

#include <vector>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <stdio.h>

#include "mesh.h"

extern double dx;

typedef std::vector<std::string> StringVector;
typedef std::vector<GLfloat> FloatVector;
typedef std::vector<GLushort> IndexVector;

void Mesh::load(const char* filename)
{
	std::fstream fin(filename, std::fstream::in);
	std::string line;
	StringVector splitLine;
	
	// Buffer objects
	FloatVector verts;
	IndexVector faces;
	
	if ( fin.fail() ) {
		fprintf (stderr, "Could not open file %s\n", filename);
		exit (EXIT_FAILURE);
	} 
	
	// For each line in the file
	while( !fin.eof() ) {
		std::getline(fin, line);
		boost::split(splitLine, line,
					 boost::is_any_of(" \t\n"),
					 boost::token_compress_on);
		
		// If the line has any contents
		if(splitLine.size()>0) {
				
			switch(splitLine[0][0]) {
				case 'v':
					if(splitLine.size()>=4) {
						// Add each vertex point
						for(int v=1; v<4; v++) {
							verts.push_back( atof(splitLine[v].c_str())*dx*0.45);
						}
					}
				break;
				
				case 'f':
					if(splitLine.size()>=4) {
						for(int f=1; f<4; f++) {
							faces.push_back( atoi(splitLine[f].c_str())-1 );
						}
					}
				break;
				
				default:
					break;
			}
		}
	}
	
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	
	glBindBuffer( GL_ARRAY_BUFFER, vbo);
	glBufferData( GL_ARRAY_BUFFER, verts.size()*sizeof(GLfloat),
				  &verts[0], GL_STATIC_DRAW);
	
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, faces.size()*sizeof(GLushort),
				  &faces[0], GL_STATIC_DRAW);
	
	glBindBuffer( GL_ARRAY_BUFFER, 0);
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);
	
	nIndex = faces.size();
}

void Mesh::render(float r, float g, float b, float a)
{
	glBindBuffer( GL_ARRAY_BUFFER, vbo);
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(r, g, b, a);
	glVertexPointer(3, 	GL_FLOAT, 0, 0);
	glDrawElements( GL_TRIANGLES, nIndex, GL_UNSIGNED_SHORT, 0);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	
	glBindBuffer( GL_ARRAY_BUFFER, 0);
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);
}
