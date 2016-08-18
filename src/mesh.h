#ifndef H_MESH
#define H_MESH

#include "core.h"

typedef unsigned short Index;

struct Vertex {
	vec3 coord;
	vec3 normal;
	vec2 texCoord;
};

struct Mesh {
	GLuint	ID[2];
	int		iCount;
	int		vCount;
/*
	Mesh(const char *name) {
		Stream stream(name);
		Index  *indices  = stream.readArray<Index> (stream.read(iCount));
		Vertex *vertices = stream.readArray<Vertex>(stream.read(vCount));

		glGenBuffers(2, ID);
		bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(Index), indices, GL_STATIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, vCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

		delete[] indices;
		delete[] vertices;
	}
*/
	virtual ~Mesh() { 
		glDeleteBuffers(2, ID);
	}

	void bind() {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID[0]);
		glBindBuffer(GL_ARRAY_BUFFER, ID[1]);
	}

	void render() {
		bind();

		glEnableVertexAttribArray(aCoord);
		glEnableVertexAttribArray(aNormal);
		glEnableVertexAttribArray(aTexCoord);


		Vertex *v = NULL;		
		glVertexAttribPointer(aCoord,	 3, GL_FLOAT, false, sizeof(Vertex), &v->coord);
		glVertexAttribPointer(aNormal,	 3, GL_FLOAT, false, sizeof(Vertex), &v->normal);
		glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, false, sizeof(Vertex), &v->texCoord);
		glDrawElements(GL_TRIANGLES, iCount, GL_UNSIGNED_SHORT, NULL);

		glDisableVertexAttribArray(aCoord);
		glDisableVertexAttribArray(aNormal);
		glDisableVertexAttribArray(aTexCoord);
	}
};

#endif