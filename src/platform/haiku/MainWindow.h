#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <Path.h>
#include <DirectWindow.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include "GameView.h"


class MainWindow : public BDirectWindow
{
public:
								MainWindow();
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			BMenuBar*			_BuildMenu();
			void				_StartGame(const char *lvlName = nullptr);
			void				_StopGame();
			void				_ToggleFullscreen();
			void				_SetMenuEnabled(bool enabled);

			BMenuBar*			fMenuBar;
			float				fGameViewTop;
			BMenuItem*			fMenuStart;
			BMenuItem*			fMenuOpen;
			BMenuItem*			fMenuQuickSave;
			BMenuItem*			fMenuQuickLoad;
			GameView*			fGameView;
			BFilePanel*			fOpenPanel;
			char				fLvlName[255];
			char				fContentDir[255];
			bool				fIsFullscreen;
};


#endif // MAINWINDOW_H
