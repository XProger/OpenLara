#ifndef GAMEVIEW_H
#define GAMEVIEW_H


#include <GLView.h>


class GameView : public BGLView
{
public:
								GameView(BRect frame);
	virtual						~GameView();
	virtual	void				FrameResized(float width, float height);
			void				StartGame(const char *lvlName);
			void				StopGame();
			bool				IsRunning();

protected:
			void				_CreateGameRenderThread();
	static	int32				_GameRenderThreadEntry(void* pointer);
			void				_GameRenderLoop();

			thread_id			fGameRenderThread;

private:
	const	char*				fLvlName;
};


#endif // GAMEVIEW_H
