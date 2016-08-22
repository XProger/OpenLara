#ifndef H_DEBUG
#define H_DEBUG

#include "core.h"

namespace Debug {

	namespace Draw {

		void begin() {
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf((GLfloat*)&Core::mProj);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glLoadMatrixf((GLfloat*)&Core::mView);

			glLineWidth(3);
			glPointSize(32);

			glUseProgram(0);
		}

		void end() {
			//
		}

		void box(const vec3 &min, const vec3 &max) {
			glBegin(GL_LINES);
				glVertex3f(min.x, min.y, min.z);
				glVertex3f(max.x, min.y, min.z);
				glVertex3f(min.x, max.y, min.z);
				glVertex3f(max.x, max.y, min.z);

				glVertex3f(min.x, min.y, max.z);
				glVertex3f(max.x, min.y, max.z);
				glVertex3f(min.x, max.y, max.z);
				glVertex3f(max.x, max.y, max.z);

				glVertex3f(min.x, min.y, min.z);
				glVertex3f(min.x, min.y, max.z);
				glVertex3f(min.x, max.y, min.z);
				glVertex3f(min.x, max.y, max.z);

				glVertex3f(max.x, min.y, min.z);
				glVertex3f(max.x, min.y, max.z);
				glVertex3f(max.x, max.y, min.z);
				glVertex3f(max.x, max.y, max.z);

				glVertex3f(min.x, min.y, min.z);
				glVertex3f(min.x, max.y, min.z);

				glVertex3f(max.x, min.y, min.z);
				glVertex3f(max.x, max.y, min.z);
				glVertex3f(min.x, min.y, min.z);
				glVertex3f(min.x, max.y, min.z);

				glVertex3f(max.x, min.y, max.z);
				glVertex3f(max.x, max.y, max.z);
				glVertex3f(min.x, min.y, max.z);
				glVertex3f(min.x, max.y, max.z);
			glEnd();
		}

		void sphere(const vec3 &center, const float radius, const vec4 &color) {
			const float k = PI * 2.0f / 18.0f;

			glColor4fv((GLfloat*)&color);
			for (int j = 0; j < 3; j++) {
				glBegin(GL_LINE_STRIP);
				for (int i = 0; i < 19; i++) {
					vec3 p = vec3(sinf(i * k), cosf(i * k), 0.0f) * radius;
					glVertex3f(p[j] + center.x, p[(j + 1) % 3] + center.y, p[(j + 2) % 3] + center.z);
				}
				glEnd();
			}
		}

		void mesh(vec3 *vertices, Index *indices, int iCount) {
			glBegin(GL_LINES);
			for (int i = 0; i < iCount; i += 3) {
				vec3 &a = vertices[indices[i + 0]];
				vec3 &b = vertices[indices[i + 1]];
				vec3 &c = vertices[indices[i + 2]];
				glVertex3fv((GLfloat*)&a);
				glVertex3fv((GLfloat*)&b);

				glVertex3fv((GLfloat*)&b);
				glVertex3fv((GLfloat*)&c);

				glVertex3fv((GLfloat*)&c);
				glVertex3fv((GLfloat*)&a);
			}
			glEnd();
		}

		void axes(float size) {
			glBegin(GL_LINES);
				glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(size,    0,    0);
				glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(   0, size,    0);
				glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(   0,    0, size);
			glEnd();
		}

		void point(const vec3 &p, const vec4 &color) {
			glColor4fv((GLfloat*)&color);
			glBegin(GL_POINTS);
				glVertex3fv((GLfloat*)&p);
			glEnd();
		}
	}
}

#endif