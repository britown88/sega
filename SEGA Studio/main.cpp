#include <WINDOWS.H>
#include "mainForm.h"

[STAThread] 

// Define our entry-point
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
              LPSTR lpStrCmdString, int nCmdShow)
{
    System::Windows::Forms::Application::EnableVisualStyles();
    System::Windows::Forms::Application::Run( gcnew SEGAStudio::mainForm() );

    return 0;
}