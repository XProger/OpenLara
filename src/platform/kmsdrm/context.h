#include <EGL/egl.h>

int physical_width;
int physical_height;

EGLDisplay display;
EGLConfig config;
EGLContext context;
EGLSurface surface;

void init_window();
void init_egl();
void deinit_egl();
void deinit_window();
void swap_window();
