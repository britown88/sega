#pragma once

#include "segalib\EGA.h"
#include "Strings.h"
#include "mainForm.h"

namespace SEGAStudio {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for ColorPicker
	/// </summary>
	public ref class ColorPicker : public System::Windows::Forms::Form
	{
	public:
      ColorPicker(mainForm ^main, byte pos, byte currentColor)
         :pos(pos), oldColor(currentColor), m_parent(main), m_updated(false)
		{
			InitializeComponent();
         selectedColor = currentColor;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~ColorPicker()
		{
			if (components)
			{
				delete components;
			}
		}
   private: System::Windows::Forms::ImageList^  paletteIcons;
   private: System::Windows::Forms::Panel^  palettePanel;
   private: System::Windows::Forms::StatusStrip^  statusStrip1;
   private: System::Windows::Forms::ToolStripStatusLabel^  statusLabel;


   protected: 
   private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
         this->components = (gcnew System::ComponentModel::Container());
         System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(ColorPicker::typeid));
         this->paletteIcons = (gcnew System::Windows::Forms::ImageList(this->components));
         this->palettePanel = (gcnew System::Windows::Forms::Panel());
         this->statusStrip1 = (gcnew System::Windows::Forms::StatusStrip());
         this->statusLabel = (gcnew System::Windows::Forms::ToolStripStatusLabel());
         this->statusStrip1->SuspendLayout();
         this->SuspendLayout();
         // 
         // paletteIcons
         // 
         this->paletteIcons->ImageStream = (cli::safe_cast<System::Windows::Forms::ImageListStreamer^  >(resources->GetObject(L"paletteIcons.ImageStream")));
         this->paletteIcons->TransparentColor = System::Drawing::Color::Transparent;
         this->paletteIcons->Images->SetKeyName(0, L"undef.png");
         this->paletteIcons->Images->SetKeyName(1, L"x.png");
         // 
         // palettePanel
         // 
         this->palettePanel->Location = System::Drawing::Point(12, 12);
         this->palettePanel->Name = L"palettePanel";
         this->palettePanel->Size = System::Drawing::Size(160, 160);
         this->palettePanel->TabIndex = 1;
         this->palettePanel->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &ColorPicker::palettePanel_Paint);
         this->palettePanel->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &ColorPicker::palettePanel_MouseClick);
         this->palettePanel->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &ColorPicker::palettePanel_MouseMove);
         // 
         // statusStrip1
         // 
         this->statusStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->statusLabel});
         this->statusStrip1->Location = System::Drawing::Point(0, 183);
         this->statusStrip1->Name = L"statusStrip1";
         this->statusStrip1->Size = System::Drawing::Size(180, 22);
         this->statusStrip1->TabIndex = 2;
         this->statusStrip1->Text = L"statusStrip1";
         // 
         // statusLabel
         // 
         this->statusLabel->Name = L"statusLabel";
         this->statusLabel->Size = System::Drawing::Size(0, 17);
         // 
         // ColorPicker
         // 
         this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
         this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
         this->ClientSize = System::Drawing::Size(180, 205);
         this->ControlBox = false;
         this->Controls->Add(this->statusStrip1);
         this->Controls->Add(this->palettePanel);
         this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::Fixed3D;
         this->MaximizeBox = false;
         this->MinimizeBox = false;
         this->MinimumSize = System::Drawing::Size(190, 215);
         this->Name = L"ColorPicker";
         this->ShowIcon = false;
         this->ShowInTaskbar = false;
         this->Deactivate += gcnew System::EventHandler(this, &ColorPicker::ColorPicker_Deactivate);
         this->Load += gcnew System::EventHandler(this, &ColorPicker::ColorPicker_Load);
         this->statusStrip1->ResumeLayout(false);
         this->statusStrip1->PerformLayout();
         this->ResumeLayout(false);
         this->PerformLayout();

      }
#pragma endregion
   private: byte selectedColor, oldColor, pos;
            bool m_updated;
            mainForm ^m_parent;

            void renderPaletteSquare(Graphics ^g, byte value) {
               int imgSize = 20;
               int rowLen = 8;
               
               int ci = getEGAColor(value);
               byte c[4];
               memcpy(c, &ci, 4);
               auto b = gcnew SolidBrush(Color::FromArgb(c[0], c[1], c[2]));
               g->FillRectangle(b, (value%8)*imgSize, (value/8)*imgSize, imgSize, imgSize);


            }

private: System::Void ColorPicker_Load(System::Object^  sender, System::EventArgs^  e) {
            }
private: System::Void palettePanel_Paint(System::Object^  sender, System::Windows::Forms::PaintEventArgs^  e) {
            for(int i = 0; i < EGA_COLORS; ++i) {
               renderPaletteSquare(e->Graphics, i);
            }
         
         }
private: System::Void palettePanel_MouseClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
            int x = e->X/20;
            int y = e->Y/20;

            if(x >= 0 && x < 8 && y >= 0 && y < 8) {
               m_parent->updatePalette(pos, y*8+x);
               m_updated = true;
               Close();
            }
         }
private: System::Void palettePanel_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
            int x = e->X/20;
            int y = e->Y/20;

            if(x >= 0 && x < 8 && y >= 0 && y < 8) {
               String ^str = "";
               byte value = y*8+x;

               if(value != selectedColor){
                  selectedColor = value;
                  m_parent->updatePalette(pos, value);
               }

               if(value == EGA_COLOR_UNDEFINED){
                   str += "Undefined: Chosen by conversion.";
               }
               else if(value == EGA_COLOR_UNUSED){
                  str += "Unused: Decrease available colors in a convert.";
               }
               else {
                  unsigned int ega = getEGAColor(value);
                  byte c[4] = {0};
                  memcpy(c, &ega, 3);
                  
                  String ^hex = "";
                  for(int i = 0; i < 3; ++i)
                     hex += String::Format("{0:x2}", c[i])->ToUpper();


                  str += "" + value + " - " + hex;

               }

               statusLabel->Text = str;
            }
         }
private: System::Void ColorPicker_Deactivate(System::Object^  sender, System::EventArgs^  e) {
            if(!m_updated){
               m_parent->updatePalette(pos, oldColor);
               Close();
            }
         }
};
}
