#include "mainForm.h"
#include "ColorPicker.h"

void SEGAStudio::mainForm::colorPicker(byte pos){
   auto picker = gcnew ColorPicker(this, pos, m_palette[pos]);               

   auto point = System::Windows::Forms::Cursor::Position;
   point.Offset(0, -picker->Height);

   picker->StartPosition = FormStartPosition::Manual;
   picker->Location = point;
   picker->Show();
}

