#include "GameView.h"


extern char contentDir[255];


namespace Game {
	extern void init(const char *lvlName = NULL);
	extern bool update();
	extern bool render();
	extern void deinit();
}


namespace Core {
	extern int width;
	extern int height;
	extern bool isQuit;
	extern void waitVBlank();
}


extern void sndInit();
extern void sndFree();
extern void sndFill();


GameView::GameView(BRect frame)
	:
	BGLView(frame, "gameView", B_FOLLOW_ALL, B_WILL_DRAW,
		BGL_RGB | BGL_DOUBLE | BGL_DEPTH)
{
	fLvlName = nullptr;
	fGameRenderThread = 0;
	Core::width = frame.Width();
	Core::height = frame.Height();

	sndInit();
}


GameView::~GameView()
{
	sndFree();
}


void
GameView::StartGame(const char *lvlName)
{
	fLvlName = lvlName;
	if (lvlName != nullptr)
		contentDir[0] = '\0';

	_CreateGameRenderThread();
}


void
GameView::StopGame()
{
	Core::isQuit = true;
	status_t ret;
	wait_for_thread(fGameRenderThread, &ret);
	fGameRenderThread = 0;
}


bool
GameView::IsRunning()
{
	return fGameRenderThread > 0;
}


void
GameView::FrameResized(float width, float height)
{
	Core::width = width;
	Core::height = height;
	BGLView::FrameResized(width, height);
}


void
GameView::_CreateGameRenderThread()
{
	fGameRenderThread = spawn_thread(GameView::_GameRenderThreadEntry, "gameRenderThread", B_NORMAL_PRIORITY, this);
	resume_thread(fGameRenderThread);
}


int32
GameView::_GameRenderThreadEntry(void* pointer)
{
	reinterpret_cast<GameView*>(pointer)->_GameRenderLoop();

	return 0;
}


void
GameView::_GameRenderLoop()
{
	LockGL();
	Game::init(fLvlName);
	UnlockGL();

	while (!Core::isQuit) {
		LockGL();
		if (Game::update()) {
			Game::render();
			Core::waitVBlank();
			SwapBuffers(false);
		}

		UnlockGL();
	}

	LockGL();
	Game::deinit();
	UnlockGL();
}
