#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <fcntl.h>

struct drm_fb {
	struct gbm_bo *bo;
	uint32_t fb_id;
};

struct gbm_device {
	struct gbm_device *dev;
	struct gbm_surface *surface;
};

struct drm_device{
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;

	drmModeCrtcPtr orig_crtc;
};

struct gbm_device gbm;
struct drm_device drm;
struct gbm_bo *bo;
struct drm_fb *fb;

drmEventContext eventContext;

extern EGLDisplay display;
extern EGLConfig config;
extern EGLContext context;
extern EGLSurface surface;

void drmPageFlipHandler(int fd, uint frame, uint sec, uint usec, void *data) {
	int *waiting_for_flip = (int *)data;
	*waiting_for_flip = 0;
}

struct drm_fb *drmFBGetFromBO(struct gbm_bo *bo) {
	struct drm_fb *fb = (struct drm_fb*)gbm_bo_get_user_data(bo);
	uint32_t width, height, stride, handle;

	if (fb) {
		return fb;
	}
	
	fb = (struct drm_fb *)calloc(1, sizeof *fb);
	fb->bo = bo;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	stride = gbm_bo_get_stride(bo);
	handle = gbm_bo_get_handle(bo).u32;

	if (drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id)) {
		printf("Could not add drm framebuffer\n");
		free(fb);
		return NULL;
	}

	// We used to pass the destroy callback function here. Now it's done manually in deinitEGL()
	gbm_bo_set_user_data(bo, fb, NULL);
	return fb;
}

void drmPageFlip(void) {
	int waiting_for_flip = 1;
	fd_set fds;

	struct gbm_bo *next_bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drmFBGetFromBO(next_bo);

	if (drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip)) {
		printf ("Failed to queue pageflip\n");
		return;
	}

	while (waiting_for_flip) {
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(drm.fd, &fds);
		select(drm.fd+1, &fds, NULL, NULL, NULL);
		drmHandleEvent(drm.fd, &eventContext);
	}
	
	// release last buffer to render on again
	gbm_surface_release_buffer(gbm.surface, bo);
	bo = next_bo;
}

void swap_window () {
	    eglSwapBuffers(display, surface);
            drmPageFlip();
}

bool initDRM(void) {
	// In plain C, we can just init _eventContext at declare time, but it's now allowed in C++
	eventContext.version = DRM_EVENT_CONTEXT_VERSION;
	eventContext.page_flip_handler = drmPageFlipHandler;

	drmModeConnector *connector;
	drmModeEncoder *encoder;
	uint i, area;

	drm.fd = open("/dev/dri/card0", O_RDWR);

	if (drm.fd < 0) {
		printf ("could not open drm device\n");
		return false;
	}

	drmModeRes *resources = drmModeGetResources(drm.fd);
	if (!resources) {
		printf ("drmModeGetResources failed\n");
		return false;
	}

	// find a connected connector
	for (i = 0; i < (uint)resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			// it's connected, let's use this!
			break;
		}
		drmModeFreeConnector(connector);
		connector = NULL;
	}

	if (!connector) {
		// we could be fancy and listen for hotplug events and wait for
		// a connector..
		printf ("no connected connector found\n");
		return false;
	}
	// find highest resolution mode
	for (i = 0, area = 0; i < (uint)connector->count_modes; i++) {
		drmModeModeInfo *current_mode = &connector->modes[i];
		uint current_area = current_mode->hdisplay * current_mode->vdisplay;
		if (current_area > area) {
			drm.mode = current_mode;
			area = current_area;
		}
	}

	if (!drm.mode) {
		printf ("could not find mode\n");
		return false;
	}

	// find encoder
	for (i = 0; i < (uint)resources->count_encoders; i++) {
		encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
		if (encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(encoder);
		encoder = NULL;
	}

	if (!encoder) {
		printf ("no encoder found\n");
		return false;
	}

	drm.crtc_id = encoder->crtc_id;
	drm.connector_id = connector->connector_id;

	// backup original crtc so we can restore the original video mode on exit.
	drm.orig_crtc = drmModeGetCrtc(drm.fd, encoder->crtc_id);

	return true;
}

bool initGBM() {
	gbm.dev = gbm_create_device(drm.fd);

	gbm.surface = gbm_surface_create(gbm.dev,
			drm.mode->hdisplay, drm.mode->vdisplay,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

	if (!gbm.surface) {
		printf ("failed to create gbm surface\n");
		return -1;
	}
	
	return true;
}

extern int physical_width;
extern int physical_height;

void init_window () {
	if (!initDRM()) {
		printf ("failed to initialize DRM\n");
		return;
	}

	// Create the GBM surface, which contains several buffers. A GBM surface is an "abstraction".
	if (!initGBM()) {
		printf ("failed to initialize GBM\n");
		return;
	}
}


void init_egl() {

	static const EGLint attributeList[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        	EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        	EGL_BLUE_SIZE,       8,
		EGL_GREEN_SIZE,      8,
		EGL_RED_SIZE,        8,
		EGL_ALPHA_SIZE,      8,
		EGL_NONE
	};
	
	// create an EGL rendering context
	static const EGLint contextAttributes[] = {
	    EGL_CONTEXT_CLIENT_VERSION, 2,
	    EGL_NONE
	};

	EGLint numConfig;
	
	display = eglGetDisplay((NativeDisplayType)gbm.dev);
	assert(display != EGL_NO_DISPLAY);

	// initialize the EGL display connection
	EGLBoolean result = eglInitialize(display, NULL, NULL);
	assert(EGL_FALSE != result);
    
	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(display, attributeList, &config, 1, &numConfig);
	assert(EGL_FALSE != result);
   
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
	assert(context != EGL_NO_CONTEXT);

        surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)gbm.surface, NULL);
	assert(surface != EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(display, surface, surface, context);
	assert(EGL_FALSE != result);

	if (eglSwapBuffers(display, surface) != EGL_TRUE) {
		printf("eglSwapBuffers() failed\n");
	}
	
	// NEVER call this without having called eglSwapBuffers before: it will segfault badly.
        // THIS is why you need to call eglSwapBuffers before calling drmModeSetCrtc: because you need
	// to call eglSwapBuffers so you can call gbm_surface_lock_front_buffer so you can call
        // drmModeSetCrtc on the buffer you get.
        bo = gbm_surface_lock_front_buffer(gbm.surface);
        fb = drmFBGetFromBO(bo);

        // set mode physical video mode
        if (drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0, &drm.connector_id, 1, drm.mode)) {
                printf ("failed to set mode\n");
                return;
	}

	physical_width = drm.mode->hdisplay;
	physical_height = drm.mode->vdisplay;

	printf ("EGL init succesfull\n");
}

void deinit_egl() {
	// Release context resources
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(display, surface);
	eglDestroyContext(display, context);
	eglTerminate(display);
}

void deinit_window () {
	// Restore the original videomode/connector/scanoutbuffer combination (the original CRTC, that is). 
	drmModeSetCrtc(drm.fd, drm.orig_crtc->crtc_id, drm.orig_crtc->buffer_id,
		drm.orig_crtc->x, drm.orig_crtc->y,
		&drm.connector_id, 1, &drm.orig_crtc->mode);	

	if (fb->fb_id) {
		drmModeRmFB(drm.fd, fb->fb_id);
	}

	// If we want to re-create the context in the same program, we need to de-init
	// gbm before we init it again.
	gbm_surface_destroy(gbm.surface);
	gbm_device_destroy(gbm.dev);

	free(fb);

	close(drm.fd);
}
