#include "game.h"

DWORD getTime() {
#ifdef DEBUG
	LARGE_INTEGER Freq, Count;
	QueryPerformanceFrequency(&Freq);
	QueryPerformanceCounter(&Count);
	return (DWORD)(Count.QuadPart * 1000L / Freq.QuadPart);
#else
	timeBeginPeriod(0);
	return timeGetTime();
#endif
}

InputKey keyToInputKey(int code) {
	int codes[] = {
		VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_RETURN, VK_ESCAPE, VK_SHIFT, VK_CONTROL, VK_MENU,
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	};

	for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
		if (codes[i] == code)
			return (InputKey)(ikLeft + i);
	return ikNone;
}

InputKey mouseToInputKey(int msg) {
	return	(msg >= WM_LBUTTONDOWN || msg <= WM_LBUTTONDBLCLK) ? ikMouseL :
			(msg >= WM_RBUTTONDOWN || msg <= WM_RBUTTONDBLCLK) ? ikMouseR : ikMouseM;
}

#define JOY_DEAD_ZONE_STICK		0.3f
#define JOY_DEAD_ZONE_TRIGGER	0.01f

bool joyReady;

void joyInit() {
	JOYINFOEX info;
	info.dwSize  = sizeof(info);
	info.dwFlags = JOY_RETURNALL;
	joyReady = joyGetPosEx(0, &info) == JOYERR_NOERROR;
}

void joyFree() {
	joyReady = false;
	memset(&Input::joy, 0, sizeof(Input::joy));
	for (int ik = ikJoyA; ik <= ikJoyDP; ik++)
		Input::down[ik] = false;
}

float joyAxis(int x, int xMin, int xMax) {
	return ((x - xMin) / (float)(xMax - xMin)) * 2.0f - 1.0f;
}

vec2 joyDir(float ax, float ay) {
	vec2 dir = vec2(ax, ay);
	float dist = min(1.0f, dir.length());
	if (dist < JOY_DEAD_ZONE_STICK) dist = 0;

	return dir.normal() * dist;
}

void joyUpdate() {
	if (!joyReady) return;

	JOYINFOEX info;
	info.dwSize  = sizeof(info);
	info.dwFlags = JOY_RETURNALL;

	if (joyGetPosEx(0, &info) == JOYERR_NOERROR) {
		JOYCAPS caps;
		joyGetDevCaps(0, &caps, sizeof(caps));

		Input::setPos(ikJoyL, joyDir(joyAxis(info.dwXpos, caps.wXmin, caps.wXmax), 
									 joyAxis(info.dwYpos, caps.wYmin, caps.wYmax)));

		if ((caps.wCaps & JOYCAPS_HASR) && (caps.wCaps & JOYCAPS_HASU))
			Input::setPos(ikJoyR, joyDir(joyAxis(info.dwUpos, caps.wUmin, caps.wUmax), 
										 joyAxis(info.dwRpos, caps.wRmin, caps.wRmax)));

		if (caps.wCaps & JOYCAPS_HASZ) {
			float z = joyAxis(info.dwZpos, caps.wZmin, caps.wZmax);
			if (fabsf(z) > JOY_DEAD_ZONE_TRIGGER)
				Input::setPos(z > 0.0f ? ikJoyLT : ikJoyRT, vec2(fabsf(z), 0.0f));
		}
		
		if (caps.wCaps & JOYCAPS_HASPOV && info.dwPOV != JOY_POVCENTERED)
			Input::setPos(ikJoyDP, vec2((float)(1 + info.dwPOV / 4500), 0));

		for (int i = 0; i < 10; i++)
			Input::setDown((InputKey)(ikJoyA + i), (info.dwButtons & (1 << i)) > 0);
	} else
		joyFree();
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_ACTIVATE :
			Input::reset();
			break;
		// keyboard
		case WM_KEYDOWN    :
		case WM_KEYUP      :
		case WM_SYSKEYDOWN :
		case WM_SYSKEYUP   :
			Input::setDown(keyToInputKey(wParam), msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
			break;
		// mouse
		case WM_LBUTTONDOWN   :
		case WM_LBUTTONUP     :
		case WM_LBUTTONDBLCLK :
		case WM_RBUTTONDOWN   :
		case WM_RBUTTONUP     :
		case WM_RBUTTONDBLCLK :
		case WM_MBUTTONDOWN   :
		case WM_MBUTTONUP     :
		case WM_MBUTTONDBLCLK : {
			InputKey key = mouseToInputKey(msg);
			Input::setPos(key, vec2((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam)));
			bool down = msg != WM_LBUTTONUP && msg != WM_RBUTTONUP && msg != WM_MBUTTONUP;
			Input::setDown(key, down);
			if (down)
				SetCapture(hWnd);
			else
				ReleaseCapture();
			break;
		}
		case WM_MOUSEMOVE :
			Input::setPos(ikMouseL, vec2((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam)));
			break;
		// gamepad
		case WM_DEVICECHANGE :
			joyInit();
			return 1;
		// touch
		// ...
		case WM_SIZE :
			Core::width  = LOWORD(lParam);
			Core::height = HIWORD(lParam);
			break;
		case WM_DESTROY :
			PostQuitMessage(0);
			break;
		default :
			return DefWindowProc(hWnd, msg, wParam, lParam); 	
	}
	return 0;
}

HGLRC initGL(HDC hDC) {
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize		= sizeof(pfd);
	pfd.nVersion	= 1;
	pfd.dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.cColorBits	= 32;
	pfd.cDepthBits	= 24;

	int format = ChoosePixelFormat(hDC, &pfd);
	SetPixelFormat(hDC, format, &pfd);
	HGLRC hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
	return hRC;
}

void freeGL(HGLRC hRC) {
	wglMakeCurrent(0, 0);
	wglDeleteContext(hRC);
}

int main() {
	RECT r = { 0, 0, 1280, 720 };
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);

	HWND hWnd = CreateWindow("static", "OpenLara", WS_OVERLAPPEDWINDOW, 0, 0, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);

	joyInit();

	HDC hDC = GetDC(hWnd);
	HGLRC hRC = initGL(hDC);
	Game::init();

	SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&WndProc); 
	ShowWindow(hWnd, SW_SHOWDEFAULT); 

	DWORD time, lastTime = getTime();
	
	MSG msg;
	msg.message = WM_PAINT;

	while (msg.message != WM_QUIT) 
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			time = getTime();
			if (time <= lastTime)
				continue;
			
			Core::deltaTime = (time - lastTime) * 0.001f;
			lastTime = time;

			joyUpdate();
			Game::update();
			Game::render();

			SwapBuffers(hDC);
		}
		
	Game::free();
	freeGL(hRC);
	ReleaseDC(hWnd, hDC);

	DestroyWindow(hWnd);
 
	return 0;
}