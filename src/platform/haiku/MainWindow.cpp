#include "MainWindow.h"


extern char contentDir[255];
extern void handleKeyPress(const char* keyBytes, int index);
extern void handleModifierKeyPress(int32 keys, int index);


namespace Game {
	extern void quickSave();
	extern void quickLoad(bool forced = false);
}


static const uint32 kMsgGameStart = 'gmst';
static const uint32 kMsgGameOpen = 'gmop';
static const uint32 kMsgQuickSave = 'gmqs';
static const uint32 kMsgQuickLoad = 'gmql';
static const uint32 kMsgGameFullscreen = 'gmrr';
static const uint32 kMsgOpenContentDir = 'opcd';


MainWindow::MainWindow()
	:
	BDirectWindow(BRect(100, 100, 740, 580), "OpenLara", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	fLvlName[0] = '\0';
	strcpy(fContentDir, contentDir);
	fIsFullscreen = false;

	fMenuBar = _BuildMenu();
	AddChild(fMenuBar);

	BRect menuBounds = fMenuBar->Bounds();
	fGameViewTop = menuBounds.bottom;
	ResizeBy(0, fGameViewTop);

	BRect gameViewBounds = Bounds();
	gameViewBounds.top = fGameViewTop;
	fGameView = new GameView(gameViewBounds);
	AddChild(fGameView);

	fOpenPanel = new BFilePanel(B_OPEN_PANEL);
	fOpenPanel->SetTarget(this);
	fOpenPanel->SetPanelDirectory(contentDir);
}


MainWindow::~MainWindow()
{
}


void
MainWindow::_StartGame(const char *lvlName)
{
	if (fGameView->IsRunning())
		return;

	fMenuStart->SetEnabled(false);
	fMenuOpen->SetEnabled(false);
	fMenuQuickSave->SetEnabled(true);
	fMenuQuickLoad->SetEnabled(true);

	fGameView->StartGame(lvlName);
}


void
MainWindow::_StopGame()
{
	fGameView->StopGame();
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_KEY_DOWN:
		{
			const char *keyBytes;
			message->FindString("bytes", &keyBytes);
			handleKeyPress(keyBytes, 1);
			break;
		}

		case B_KEY_UP:
		{
			const char *keyBytes;
			message->FindString("bytes", &keyBytes);
			handleKeyPress(keyBytes, 0);
			break;
		}

		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			int32 keys;
			message->FindInt32("modifiers", &keys);
			handleModifierKeyPress(keys, 1);
			break;

		case kMsgGameStart:
			_StartGame();
			break;

		case kMsgGameOpen:
			fOpenPanel->Show();
			break;

		case kMsgOpenContentDir:
		{
			entry_ref ref;
			BEntry entry(fContentDir);
			if (entry.Exists() && entry.GetRef(&ref) == B_OK)
				be_roster->Launch(&ref);

			break;
		}

		case kMsgQuickSave:
			Game::quickSave();
			break;

		case kMsgQuickLoad:
			Game::quickLoad();
			break;

		case kMsgGameFullscreen:
			_ToggleFullscreen();
			break;

		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BEntry entry(&ref, false);
				if (entry.InitCheck() == B_OK) {
					BPath path;
					entry.GetPath(&path);

					const char* pathStr = path.Path();

					if (strlen(pathStr) < 255)
						strcpy(fLvlName, pathStr);

					_StartGame(fLvlName);
				}
			}

			break;
		}

		default:
			BDirectWindow::MessageReceived(message);
	}
}


bool
MainWindow::QuitRequested()
{
	if (fGameView->IsRunning())
		_StopGame();

	return true;
}


BMenuBar*
MainWindow::_BuildMenu()
{
	BMenuBar* menuBar = new BMenuBar(Bounds(), "menuBar");
	BMenu* gameMenu = new BMenu("Game");

	fMenuStart = new BMenuItem("Start", new BMessage(kMsgGameStart));
	gameMenu->AddItem(fMenuStart);

	fMenuOpen = new BMenuItem("Open...", new BMessage(kMsgGameOpen));
	gameMenu->AddItem(fMenuOpen);

	gameMenu->AddSeparatorItem();

	BMenuItem* item = new BMenuItem("Open content dir...", new BMessage(kMsgOpenContentDir));
	gameMenu->AddItem(item);

	gameMenu->AddSeparatorItem();

	fMenuQuickSave = new BMenuItem("Quick Save", new BMessage(kMsgQuickSave));
	fMenuQuickSave->SetEnabled(false);
	gameMenu->AddItem(fMenuQuickSave);

	fMenuQuickLoad = new BMenuItem("Quick Load", new BMessage(kMsgQuickLoad));
	fMenuQuickLoad->SetEnabled(false);
	gameMenu->AddItem(fMenuQuickLoad);

	gameMenu->AddSeparatorItem();

	item = new BMenuItem("Fullscreen", new BMessage(kMsgGameFullscreen), B_RETURN);
	gameMenu->AddItem(item);

	gameMenu->AddSeparatorItem();

	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	gameMenu->AddItem(item);

	menuBar->AddItem(gameMenu);

	return menuBar;
}


void
MainWindow::_ToggleFullscreen()
{
	fIsFullscreen = !fIsFullscreen;
	_SetMenuEnabled(!fIsFullscreen);
	SetFullScreen(fIsFullscreen);
}


void
MainWindow::_SetMenuEnabled(bool enabled)
{
	if (enabled) {
		fMenuBar->Show();
		fGameView->MoveBy(0, fGameViewTop);
		fGameView->ResizeBy(0, -fGameViewTop);
	} else {
		fMenuBar->Hide();
		fGameView->MoveBy(0, -fGameViewTop);
		fGameView->ResizeBy(0, fGameViewTop);
	}
}
