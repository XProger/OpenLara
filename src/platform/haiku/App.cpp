#include "App.h"

const char* kApplicationSignature = "application/x-vnd.XProger-OpenLara";


App::App()
	:
	BApplication(kApplicationSignature)
{
	MainWindow* mainWindow = new MainWindow();
	mainWindow->Show();
}


App::~App()
{
}
