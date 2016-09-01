#ifndef H_MESH
#define H_MESH

#include "core.h"

typedef unsigned short Index;

struct Vertex {
    short3  coord;
    short2  texCoord;
    short4  normal;
    ubyte4  color;
};

struct MeshRange {
    int iStart;
    int iCount;
    int vStart;
};

struct Mesh {
    GLuint  ID[2];
    int     iCount;
    int     vCount;

    Mesh(Index *indices, int iCount, Vertex *vertices, int vCount) : iCount(iCount), vCount(vCount) {
        glGenBuffers(2, ID);
        bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(Index), indices, GL_STATIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, vCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    }

    virtual ~Mesh() {
        glDeleteBuffers(2, ID);
    }

    void bind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID[0]);
        glBindBuffer(GL_ARRAY_BUFFER, ID[1]);

        glEnableVertexAttribArray(aCoord);
        glEnableVertexAttribArray(aTexCoord);
        glEnableVertexAttribArray(aNormal);
        glEnableVertexAttribArray(aColor);
    }

    void render(const MeshRange &range) {
        Vertex *v = (Vertex*)(range.vStart * sizeof(Vertex));
        glVertexAttribPointer(aCoord,    3, GL_SHORT,         false, sizeof(Vertex), &v->coord);
        glVertexAttribPointer(aTexCoord, 2, GL_SHORT,         true,  sizeof(Vertex), &v->texCoord);
        glVertexAttribPointer(aNormal,   4, GL_SHORT,         false, sizeof(Vertex), &v->normal);
        glVertexAttribPointer(aColor,    4, GL_UNSIGNED_BYTE, true,  sizeof(Vertex), &v->color);
        glDrawElements(GL_TRIANGLES, range.iCount, GL_UNSIGNED_SHORT, (GLvoid*)(range.iStart * sizeof(Index)));

        Core::stats.dips++;
        Core::stats.tris += range.iCount / 3;
    }
};

#endif