#pragma once
#include "segalib\EGA.h"
#include "FileInfo.h"
#include "Strings.h"
#include "segashared\CheckedMemory.h"

namespace SEGAStudio {

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Drawing::Imaging;

/// <summary>
/// Summary for mainForm
/// </summary>
public ref class mainForm : public System::Windows::Forms::Form
{
public:  mainForm(void)
	      {
		      InitializeComponent();
		      //
		      //TODO: Add the constructor code here
		      //
	      }

protected:
         /// <summary>
         /// Clean up any resources being used.
         /// </summary>
         ~mainForm()
         {
            if (components)
            {
               delete components;
            }
         }
private: System::Windows::Forms::TreeView^  fileView;
private: System::Windows::Forms::Panel^  imageCOntainer;
private: System::Windows::Forms::Panel^  imagePanel;
private: System::Windows::Forms::ImageList^  fileViewIcons;
private: System::Windows::Forms::GroupBox^  groupBox1;
private: System::Windows::Forms::Button^  btnSaveEGA;
private: System::Windows::Forms::GroupBox^  groupBox2;
private: System::Windows::Forms::Label^  label1;
private: System::Windows::Forms::Button^  btnSavePalette;
private: System::Windows::Forms::TextBox^  txtPalette;
private: System::Windows::Forms::Panel^  palettePanel;
private: System::Windows::Forms::Label^  label3;
private: System::Windows::Forms::Button^  btnSavePNG;
private: System::Windows::Forms::TextBox^  txtPNG;
private: System::Windows::Forms::Label^  label2;
private: System::Windows::Forms::TextBox^  txtEGA;            
private: System::Windows::Forms::Button^  fileRefresh;
private: System::ComponentModel::IContainer^  components;
private: System::Windows::Forms::ImageList^  paletteIcons;
private: System::Windows::Forms::Button^  btnUndefPalette;

private: System::Windows::Forms::Button^  btnUnusedPalette;
private: System::Windows::Forms::TextBox^  txtCurrent;
private: System::Windows::Forms::StatusStrip^  status;
private: System::Windows::Forms::ToolStripStatusLabel^  statusLabel;


private:
         FileInfo ^m_currentItem;
         String ^m_currentDrawnFile; 
         Bitmap ^m_drawnBitmap;
         array<byte> ^m_palette;

         bool m_paletteDragging;
         byte m_dragValue;

   #pragma region Windows Form Designer generated code
		   /// <summary>
		   /// Required method for Designer support - do not modify
		   /// the contents of this method with the code editor.
		   /// </summary>
		   void InitializeComponent(void)
		   {
            this->components = (gcnew System::ComponentModel::Container());
            System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(mainForm::typeid));
            this->fileView = (gcnew System::Windows::Forms::TreeView());
            this->fileViewIcons = (gcnew System::Windows::Forms::ImageList(this->components));
            this->imageCOntainer = (gcnew System::Windows::Forms::Panel());
            this->imagePanel = (gcnew System::Windows::Forms::Panel());
            this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
            this->label3 = (gcnew System::Windows::Forms::Label());
            this->btnSavePNG = (gcnew System::Windows::Forms::Button());
            this->txtPNG = (gcnew System::Windows::Forms::TextBox());
            this->label2 = (gcnew System::Windows::Forms::Label());
            this->btnSaveEGA = (gcnew System::Windows::Forms::Button());
            this->txtEGA = (gcnew System::Windows::Forms::TextBox());
            this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
            this->btnUnusedPalette = (gcnew System::Windows::Forms::Button());
            this->btnUndefPalette = (gcnew System::Windows::Forms::Button());
            this->label1 = (gcnew System::Windows::Forms::Label());
            this->btnSavePalette = (gcnew System::Windows::Forms::Button());
            this->txtPalette = (gcnew System::Windows::Forms::TextBox());
            this->palettePanel = (gcnew System::Windows::Forms::Panel());
            this->fileRefresh = (gcnew System::Windows::Forms::Button());
            this->paletteIcons = (gcnew System::Windows::Forms::ImageList(this->components));
            this->txtCurrent = (gcnew System::Windows::Forms::TextBox());
            this->status = (gcnew System::Windows::Forms::StatusStrip());
            this->statusLabel = (gcnew System::Windows::Forms::ToolStripStatusLabel());
            this->imageCOntainer->SuspendLayout();
            this->groupBox1->SuspendLayout();
            this->groupBox2->SuspendLayout();
            this->status->SuspendLayout();
            this->SuspendLayout();
            // 
            // fileView
            // 
            this->fileView->AllowDrop = true;
            this->fileView->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
               | System::Windows::Forms::AnchorStyles::Left));
            this->fileView->FullRowSelect = true;
            this->fileView->HideSelection = false;
            this->fileView->ImageIndex = 0;
            this->fileView->ImageList = this->fileViewIcons;
            this->fileView->Location = System::Drawing::Point(12, 12);
            this->fileView->Name = L"fileView";
            this->fileView->SelectedImageIndex = 0;
            this->fileView->Size = System::Drawing::Size(187, 462);
            this->fileView->TabIndex = 0;
            this->fileView->ItemDrag += gcnew System::Windows::Forms::ItemDragEventHandler(this, &mainForm::fileView_ItemDrag);
            this->fileView->NodeMouseClick += gcnew System::Windows::Forms::TreeNodeMouseClickEventHandler(this, &mainForm::fileView_NodeMouseClick);
            this->fileView->DragDrop += gcnew System::Windows::Forms::DragEventHandler(this, &mainForm::fileView_DragDrop);
            this->fileView->DragEnter += gcnew System::Windows::Forms::DragEventHandler(this, &mainForm::fileView_DragEnter);
            // 
            // fileViewIcons
            // 
            this->fileViewIcons->ImageStream = (cli::safe_cast<System::Windows::Forms::ImageListStreamer^  >(resources->GetObject(L"fileViewIcons.ImageStream")));
            this->fileViewIcons->TransparentColor = System::Drawing::Color::Transparent;
            this->fileViewIcons->Images->SetKeyName(0, L"folder");
            this->fileViewIcons->Images->SetKeyName(1, L"image");
            this->fileViewIcons->Images->SetKeyName(2, L"ega");
            this->fileViewIcons->Images->SetKeyName(3, L"palette");
            // 
            // imageCOntainer
            // 
            this->imageCOntainer->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
               | System::Windows::Forms::AnchorStyles::Left) 
               | System::Windows::Forms::AnchorStyles::Right));
            this->imageCOntainer->AutoScroll = true;
            this->imageCOntainer->BorderStyle = System::Windows::Forms::BorderStyle::Fixed3D;
            this->imageCOntainer->Controls->Add(this->imagePanel);
            this->imageCOntainer->Location = System::Drawing::Point(205, 39);
            this->imageCOntainer->Name = L"imageCOntainer";
            this->imageCOntainer->Size = System::Drawing::Size(651, 379);
            this->imageCOntainer->TabIndex = 1;
            // 
            // imagePanel
            // 
            this->imagePanel->Location = System::Drawing::Point(4, 4);
            this->imagePanel->Name = L"imagePanel";
            this->imagePanel->Size = System::Drawing::Size(111, 57);
            this->imagePanel->TabIndex = 0;
            this->imagePanel->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &mainForm::imagePanel_Paint);
            // 
            // groupBox1
            // 
            this->groupBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->groupBox1->Controls->Add(this->label3);
            this->groupBox1->Controls->Add(this->btnSavePNG);
            this->groupBox1->Controls->Add(this->txtPNG);
            this->groupBox1->Controls->Add(this->label2);
            this->groupBox1->Controls->Add(this->btnSaveEGA);
            this->groupBox1->Controls->Add(this->txtEGA);
            this->groupBox1->Location = System::Drawing::Point(553, 424);
            this->groupBox1->Name = L"groupBox1";
            this->groupBox1->Size = System::Drawing::Size(303, 79);
            this->groupBox1->TabIndex = 2;
            this->groupBox1->TabStop = false;
            this->groupBox1->Text = L"Operations";
            // 
            // label3
            // 
            this->label3->AutoSize = true;
            this->label3->Location = System::Drawing::Point(192, 48);
            this->label3->Name = L"label3";
            this->label3->Size = System::Drawing::Size(28, 13);
            this->label3->TabIndex = 9;
            this->label3->Text = L".png";
            // 
            // btnSavePNG
            // 
            this->btnSavePNG->Enabled = false;
            this->btnSavePNG->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"btnSavePNG.Image")));
            this->btnSavePNG->Location = System::Drawing::Point(222, 43);
            this->btnSavePNG->Name = L"btnSavePNG";
            this->btnSavePNG->Size = System::Drawing::Size(75, 23);
            this->btnSavePNG->TabIndex = 7;
            this->btnSavePNG->Text = L"Save";
            this->btnSavePNG->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->btnSavePNG->TextImageRelation = System::Windows::Forms::TextImageRelation::ImageBeforeText;
            this->btnSavePNG->UseVisualStyleBackColor = true;
            this->btnSavePNG->Click += gcnew System::EventHandler(this, &mainForm::btnSavePNG_Click);
            // 
            // txtPNG
            // 
            this->txtPNG->Enabled = false;
            this->txtPNG->Location = System::Drawing::Point(6, 45);
            this->txtPNG->Name = L"txtPNG";
            this->txtPNG->Size = System::Drawing::Size(180, 20);
            this->txtPNG->TabIndex = 8;
            // 
            // label2
            // 
            this->label2->AutoSize = true;
            this->label2->Location = System::Drawing::Point(192, 22);
            this->label2->Name = L"label2";
            this->label2->Size = System::Drawing::Size(28, 13);
            this->label2->TabIndex = 6;
            this->label2->Text = L".ega";
            // 
            // btnSaveEGA
            // 
            this->btnSaveEGA->Enabled = false;
            this->btnSaveEGA->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"btnSaveEGA.Image")));
            this->btnSaveEGA->Location = System::Drawing::Point(222, 17);
            this->btnSaveEGA->Name = L"btnSaveEGA";
            this->btnSaveEGA->Size = System::Drawing::Size(75, 23);
            this->btnSaveEGA->TabIndex = 0;
            this->btnSaveEGA->Text = L"Convert";
            this->btnSaveEGA->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->btnSaveEGA->TextImageRelation = System::Windows::Forms::TextImageRelation::ImageBeforeText;
            this->btnSaveEGA->UseVisualStyleBackColor = true;
            this->btnSaveEGA->Click += gcnew System::EventHandler(this, &mainForm::btnSaveEGA_Click);
            // 
            // txtEGA
            // 
            this->txtEGA->Enabled = false;
            this->txtEGA->Location = System::Drawing::Point(6, 19);
            this->txtEGA->Name = L"txtEGA";
            this->txtEGA->Size = System::Drawing::Size(180, 20);
            this->txtEGA->TabIndex = 4;
            // 
            // groupBox2
            // 
            this->groupBox2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->groupBox2->Controls->Add(this->btnUnusedPalette);
            this->groupBox2->Controls->Add(this->btnUndefPalette);
            this->groupBox2->Controls->Add(this->label1);
            this->groupBox2->Controls->Add(this->btnSavePalette);
            this->groupBox2->Controls->Add(this->txtPalette);
            this->groupBox2->Controls->Add(this->palettePanel);
            this->groupBox2->Location = System::Drawing::Point(205, 424);
            this->groupBox2->Name = L"groupBox2";
            this->groupBox2->Size = System::Drawing::Size(335, 79);
            this->groupBox2->TabIndex = 3;
            this->groupBox2->TabStop = false;
            this->groupBox2->Text = L"Palette";
            // 
            // btnUnusedPalette
            // 
            this->btnUnusedPalette->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"btnUnusedPalette.Image")));
            this->btnUnusedPalette->Location = System::Drawing::Point(303, 44);
            this->btnUnusedPalette->Name = L"btnUnusedPalette";
            this->btnUnusedPalette->Size = System::Drawing::Size(23, 23);
            this->btnUnusedPalette->TabIndex = 5;
            this->btnUnusedPalette->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->btnUnusedPalette->UseVisualStyleBackColor = true;
            this->btnUnusedPalette->Click += gcnew System::EventHandler(this, &mainForm::btnUnusedPalette_Click);
            // 
            // btnUndefPalette
            // 
            this->btnUndefPalette->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"btnUndefPalette.Image")));
            this->btnUndefPalette->Location = System::Drawing::Point(274, 44);
            this->btnUndefPalette->Name = L"btnUndefPalette";
            this->btnUndefPalette->Size = System::Drawing::Size(23, 23);
            this->btnUndefPalette->TabIndex = 4;
            this->btnUndefPalette->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->btnUndefPalette->UseVisualStyleBackColor = true;
            this->btnUndefPalette->Click += gcnew System::EventHandler(this, &mainForm::btnUndefPalette_Click);
            // 
            // label1
            // 
            this->label1->AutoSize = true;
            this->label1->Location = System::Drawing::Point(163, 49);
            this->label1->Name = L"label1";
            this->label1->Size = System::Drawing::Size(24, 13);
            this->label1->TabIndex = 3;
            this->label1->Text = L".pal";
            // 
            // btnSavePalette
            // 
            this->btnSavePalette->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"btnSavePalette.Image")));
            this->btnSavePalette->Location = System::Drawing::Point(193, 44);
            this->btnSavePalette->Name = L"btnSavePalette";
            this->btnSavePalette->Size = System::Drawing::Size(75, 23);
            this->btnSavePalette->TabIndex = 2;
            this->btnSavePalette->Text = L"Save";
            this->btnSavePalette->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->btnSavePalette->TextImageRelation = System::Windows::Forms::TextImageRelation::ImageBeforeText;
            this->btnSavePalette->UseVisualStyleBackColor = true;
            this->btnSavePalette->Click += gcnew System::EventHandler(this, &mainForm::btnSavePalette_Click);
            // 
            // txtPalette
            // 
            this->txtPalette->Location = System::Drawing::Point(6, 46);
            this->txtPalette->Name = L"txtPalette";
            this->txtPalette->Size = System::Drawing::Size(151, 20);
            this->txtPalette->TabIndex = 1;
            // 
            // palettePanel
            // 
            this->palettePanel->Location = System::Drawing::Point(6, 19);
            this->palettePanel->Name = L"palettePanel";
            this->palettePanel->Size = System::Drawing::Size(320, 20);
            this->palettePanel->TabIndex = 0;
            this->palettePanel->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &mainForm::palettePanel_Paint);
            this->palettePanel->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::palettePanel_MouseClick);
            this->palettePanel->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::palettePanel_MouseDown);
            this->palettePanel->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::palettePanel_MouseMove);
            this->palettePanel->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::palettePanel_MouseUp);
            // 
            // fileRefresh
            // 
            this->fileRefresh->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->fileRefresh->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"fileRefresh.Image")));
            this->fileRefresh->Location = System::Drawing::Point(12, 480);
            this->fileRefresh->Name = L"fileRefresh";
            this->fileRefresh->Size = System::Drawing::Size(187, 23);
            this->fileRefresh->TabIndex = 4;
            this->fileRefresh->Text = L"Refresh";
            this->fileRefresh->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->fileRefresh->TextImageRelation = System::Windows::Forms::TextImageRelation::ImageBeforeText;
            this->fileRefresh->UseVisualStyleBackColor = true;
            this->fileRefresh->Click += gcnew System::EventHandler(this, &mainForm::fileRefresh_Click);
            // 
            // paletteIcons
            // 
            this->paletteIcons->ImageStream = (cli::safe_cast<System::Windows::Forms::ImageListStreamer^  >(resources->GetObject(L"paletteIcons.ImageStream")));
            this->paletteIcons->TransparentColor = System::Drawing::Color::Transparent;
            this->paletteIcons->Images->SetKeyName(0, L"undef.png");
            this->paletteIcons->Images->SetKeyName(1, L"x.png");
            // 
            // txtCurrent
            // 
            this->txtCurrent->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left) 
               | System::Windows::Forms::AnchorStyles::Right));
            this->txtCurrent->Location = System::Drawing::Point(206, 13);
            this->txtCurrent->Name = L"txtCurrent";
            this->txtCurrent->ReadOnly = true;
            this->txtCurrent->Size = System::Drawing::Size(650, 20);
            this->txtCurrent->TabIndex = 5;
            // 
            // status
            // 
            this->status->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->statusLabel});
            this->status->Location = System::Drawing::Point(0, 506);
            this->status->Name = L"status";
            this->status->Size = System::Drawing::Size(868, 22);
            this->status->TabIndex = 6;
            this->status->Text = L"statusStrip1";
            // 
            // statusLabel
            // 
            this->statusLabel->Name = L"statusLabel";
            this->statusLabel->Size = System::Drawing::Size(0, 17);
            // 
            // mainForm
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(868, 528);
            this->Controls->Add(this->status);
            this->Controls->Add(this->txtCurrent);
            this->Controls->Add(this->fileRefresh);
            this->Controls->Add(this->groupBox2);
            this->Controls->Add(this->groupBox1);
            this->Controls->Add(this->imageCOntainer);
            this->Controls->Add(this->fileView);
            this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
            this->MinimumSize = System::Drawing::Size(884, 400);
            this->Name = L"mainForm";
            this->Text = L"sEGA Studio";
            this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &mainForm::mainForm_FormClosed);
            this->Load += gcnew System::EventHandler(this, &mainForm::mainForm_Load);
            this->imageCOntainer->ResumeLayout(false);
            this->groupBox1->ResumeLayout(false);
            this->groupBox1->PerformLayout();
            this->groupBox2->ResumeLayout(false);
            this->groupBox2->PerformLayout();
            this->status->ResumeLayout(false);
            this->status->PerformLayout();
            this->ResumeLayout(false);
            this->PerformLayout();

         }
   #pragma endregion
         void refreshFiles() {
            auto selected = getSelectedFile();
            fileView->Nodes->Clear();

            auto cwd = IO::Directory::GetCurrentDirectory();
            fillDirNode(cwd, fileView->Nodes, selected);
         }
         void refreshFiles(FileInfo ^selected) {
            fileView->Nodes->Clear();

            auto cwd = IO::Directory::GetCurrentDirectory();
            fillDirNode(cwd, fileView->Nodes, selected);
         }
         FileInfo ^getSelectedFile() {

            if(fileView->SelectedNode) {
               if((FileInfo^)fileView->SelectedNode->Tag) {
                  return (FileInfo^)fileView->SelectedNode->Tag;
               }
            }

            return nullptr;
         }
         void fillDirNode(System::String^ path, TreeNodeCollection ^parent, FileInfo ^reselect)  {

            for each (auto folder in IO::Directory::GetDirectories(path))
            {
               TreeNode ^fileNode = gcnew TreeNode(IO::Path::GetFileName(folder), 0, 0);
               fileNode->Tag = gcnew FileInfo(FileTypes::Folder, folder);
               parent->Add(fileNode);
               fillDirNode(folder, fileNode->Nodes, reselect);
                  
            }

            for each (auto file in IO::Directory::GetFiles(path,  "*.png"))
            {
               TreeNode ^fileNode = gcnew TreeNode(IO::Path::GetFileName(file), 1, 1);
               fileNode->Tag = gcnew FileInfo(FileTypes::PNG, file);
               parent->Add(fileNode);

               if(reselect && reselect->path == file){                     
                  auto current = fileNode->Parent;
                  while(current){
                     current->Expand();
                     current = current->Parent;
                  }

                  fileView->SelectedNode = fileNode;
               }

                  
            }
            for each (auto file in IO::Directory::GetFiles(path,  "*.ega"))
            {
               TreeNode ^fileNode = gcnew TreeNode(IO::Path::GetFileName(file), 2, 2);
               fileNode->Tag = gcnew FileInfo(FileTypes::EGA, file);
               parent->Add(fileNode);

               if(reselect && reselect->path == file)
                  fileView->SelectedNode = fileNode;
            }
            for each (auto file in IO::Directory::GetFiles(path,  "*.pal"))
            {
               TreeNode ^fileNode = gcnew TreeNode(IO::Path::GetFileName(file), 3, 3);
               fileNode->Tag = gcnew FileInfo(FileTypes::PAL, file);
               parent->Add(fileNode);

               if(reselect && reselect->path == file)
                  fileView->SelectedNode = fileNode;
            }
         }

         void loadEGAFile(FileInfo ^file){

            txtCurrent->Text = file->path;
            repairPalette();

            auto fName = IO::Path::GetFileNameWithoutExtension(file->path);
            txtEGA->Text = fName;
            txtPNG->Text = fName;
            btnSavePNG->Enabled = true;
            btnSaveEGA->Enabled = false;
            btnUndefPalette->Enabled = false;
            btnUnusedPalette->Enabled = false;
            txtEGA->Enabled = false;
            txtPNG->Enabled = true;

            auto pathStr = toNative(file->path);
            auto img = imageDeserialize(pathStr.c_str());
            auto png = pngDataCreateFromImage(img, getNativePalette().colors);            

            auto tempPath = file->path + ".temp~";
            auto tempPathStr = toNative(tempPath);
            m_currentDrawnFile = tempPath;
            pngDataExportPNG(png, tempPathStr.c_str());

            imageDestroy(img);
            pngDataDestroy(png);

            m_currentItem = file;
            imagePanel->Invalidate();
            showStatus("EGA " + fName + " loaded.");
               
         }
         void loadPNGFile(FileInfo ^file){
            txtCurrent->Text = file->path;
            auto fName = IO::Path::GetFileNameWithoutExtension(file->path);
            txtEGA->Text = fName;
            txtPNG->Text = fName;
            btnSavePNG->Enabled = false;
            btnSaveEGA->Enabled = true;
            btnUndefPalette->Enabled = true;
            btnUnusedPalette->Enabled = true;
            txtEGA->Enabled = true;
            txtPNG->Enabled = false;

            m_currentDrawnFile = file->path;   
            m_currentItem = file;
            imagePanel->Invalidate();

            showStatus("PNG " + fName + " loaded.");

         }
         void loadPALFile(FileInfo ^file){
            auto fName = IO::Path::GetFileNameWithoutExtension(file->path);
            txtPalette->Text = fName;

            auto pathStr = toNative(file->path);
            auto pal = paletteDeserialize(pathStr.c_str());
            setPalette(pal.colors);
            showStatus("Palette " + fName + " loaded.");

            if(m_currentItem && m_currentItem->type == FileTypes::EGA) {
               refreshEGA();
            }
         }

         void initPalette() {
            m_palette = gcnew array< byte >(EGA_PALETTE_COLORS);
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               m_palette[i] = EGA_COLOR_UNDEFINED;
            }
         }
         bool paletteIsPartial() {
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               if(m_palette[i] >= EGA_COLORS) {
                  return true;
               }
            }

            return false;
         }
         bool paletteIsEmpty() {
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               if(m_palette[i] != EGA_COLOR_UNUSED) {
                  return false;
               }
            }

            return true;
         }
         void repairPalette(){
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               if(m_palette[i] >= EGA_COLORS) {
                  m_palette[i] = 0;
               }
            }

            palettePanel->Invalidate();
         }

         void setPalette(byte *p){
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               m_palette[i] = p[i];
            }

            palettePanel->Invalidate();
         }

         Palette getNativePalette(){
            Palette r;
            pin_ptr<byte> pinned = &m_palette[0];
            paletteCopy(&r, (Palette*)pinned);
            return r;
         }
         
         void showStatus(String^msg){
            statusLabel->Text = msg;
         }

         bool askFileOverwrite(String ^path){
            if(IO::File::Exists(path)){
               auto ret = MessageBox::Show("File already exists. Overwrite?", "Confirmation", MessageBoxButtons::YesNo, MessageBoxIcon::Warning);
               return ret == System::Windows::Forms::DialogResult::Yes;
            }

            return true;
         }

         void refreshEGA(){
            deleteTempImage();
            loadEGAFile(m_currentItem);
            showStatus("Current EGA refreshed with palette.");
         }
         void EGA2PNG(){
            if(txtPNG->Text->Length == 0){
               return;
            }
            auto toPath = IO::Path::GetDirectoryName(m_currentDrawnFile) +IO::Path::DirectorySeparatorChar + txtPNG->Text + ".png";

            if(!askFileOverwrite(toPath)){
               showStatus("Cancelled.");
               return;
            }

            IO::File::Copy(m_currentDrawnFile, toPath, true);
            deleteTempImage();

            auto info = gcnew FileInfo(FileTypes::PNG, toPath);
            refreshFiles(info);
            loadPNGFile(info);

            showStatus("Convert Success!");

            

         }
         void PNG2EGA(){
            if(txtEGA->Text->Length == 0){
               showStatus("ERROR: No output filename provided.");
               return;
            }

            if(paletteIsEmpty()) {
               showStatus("ERROR: Palette must have at least one usable color.");
               return;
            }

            auto toPath = IO::Path::GetDirectoryName(m_currentDrawnFile) +IO::Path::DirectorySeparatorChar + txtEGA->Text + ".ega";
            auto toStr = toNative(toPath);

            if(!askFileOverwrite(toPath)){
               showStatus("Cancelled.");
               return;               
            }

            auto pathStr = toNative(m_currentDrawnFile);
            PNGData *png = nullptr;

            try{
               png = pngDataCreate(pathStr.c_str());
            }catch(std::exception e){
               MessageBox::Show("Failed to read PNG File. Is it indexed or grayscale??");
               return;
            }

            pngDataRender(png, getNativePalette().colors);
            setPalette(pngDataGetPalette(png));
            txtPalette->Text = IO::Path::GetFileNameWithoutExtension(toPath);

            auto img = pngDataCreateImage(png);

            imageSerialize(img, toStr.c_str());

            imageDestroy(img);
            pngDataDestroy(png);

            auto info = gcnew FileInfo(FileTypes::EGA, toPath);
            refreshFiles(info);
            loadEGAFile(info);

            showStatus("Convert Success!");
         }

         void renderPNG(Graphics ^g, String ^path){

            delete m_drawnBitmap;
            m_drawnBitmap = gcnew Bitmap(path);
            imagePanel->Width = m_drawnBitmap->Width;
            imagePanel->Height = (int)(m_drawnBitmap->Height * 1.37);

            g->DrawImage(m_drawnBitmap, 
               Drawing::Rectangle(0, 0, imagePanel->Width, imagePanel->Height),
               0, 0, m_drawnBitmap->Width, m_drawnBitmap->Height, GraphicsUnit::Pixel);

         }

         void renderPaletteSquare(Graphics ^g, byte pos, byte value) {
            int imgSize = 20;

            if(value >= EGA_COLORS){
               paletteIcons->Draw(g, pos*imgSize, 0, imgSize, imgSize, value - EGA_COLORS);
            }
            else {
               int ci = getEGAColor(value);
               byte c[4];
               memcpy(c, &ci, 4);
               auto b = gcnew SolidBrush(Color::FromArgb(c[0], c[1], c[2]));
               g->FillRectangle(b, pos*imgSize, 0, imgSize, imgSize);
            }

         }
         void deleteTempImage(){
            if(m_currentDrawnFile && IO::Path::GetExtension(m_currentDrawnFile) == ".temp~"){
               delete m_drawnBitmap;
               IO::File::Delete(m_currentDrawnFile);
               m_currentDrawnFile = nullptr;
               m_drawnBitmap = nullptr;
            }
         }

         void colorPicker(byte pos);

public:  void updatePalette(byte pos, byte color) {
            m_palette[pos] = color;
            if(m_currentItem && m_currentItem->type == FileTypes::EGA) {
               refreshEGA();
            }

            palettePanel->Invalidate();

         }

private: System::Void mainForm_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e) {
            printMemoryLeaks();
            deleteTempImage();            
            Application::Exit();
         }
     
private: System::Void mainForm_Load(System::Object^  sender, System::EventArgs^  e) {            
            initPalette();
            refreshFiles();
         }
private: System::Void fileRefresh_Click(System::Object^  sender, System::EventArgs^  e) {
            refreshFiles();
            showStatus("Files refreshed.");
         }
            
private: System::Void imagePanel_Paint(System::Object^  sender, System::Windows::Forms::PaintEventArgs^  e) {
            auto g = e->Graphics; 
            if(m_currentDrawnFile) {
               renderPNG(g, m_currentDrawnFile);
            }               
         }
private: System::Void btnSaveEGA_Click(System::Object^  sender, System::EventArgs^  e) {

            if(m_currentItem) {
               if(m_currentItem->type == FileTypes::PNG) {
                  PNG2EGA();
               }
            }
         }
private: System::Void btnSavePalette_Click(System::Object^  sender, System::EventArgs^  e) {
            if(paletteIsPartial()){
               showStatus("ERROR: You can only save a complete palette.");
               return;               
            }

            if(txtPalette->Text->Length == 0){
               showStatus("ERROR: No filename provided.");
               return;
            }

            auto toPath = IO::Path::GetDirectoryName(m_currentDrawnFile) +IO::Path::DirectorySeparatorChar + txtPalette->Text + ".pal";
            auto toStr = toNative(toPath);

            if(!askFileOverwrite(toPath)){
               showStatus("Cancelled.");
               return;               
            }

            paletteSerialize(getNativePalette().colors, toStr.c_str());
            refreshFiles(gcnew FileInfo(FileTypes::PAL, toPath));

         }
private: System::Void btnSavePNG_Click(System::Object^  sender, System::EventArgs^  e) {
            if(m_currentItem) {
               if(m_currentItem->type == FileTypes::EGA) {
                  EGA2PNG();
               }
            }
         }
private: System::Void palettePanel_Paint(System::Object^  sender, System::Windows::Forms::PaintEventArgs^  e) {
            auto g = e->Graphics;

            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i) {
               renderPaletteSquare(g, i, m_palette[i]);
            }
         }
private: System::Void btnUndefPalette_Click(System::Object^  sender, System::EventArgs^  e) {
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               m_palette[i] = EGA_COLOR_UNDEFINED;
            }
            palettePanel->Invalidate();
            if(m_currentItem && m_currentItem->type == FileTypes::EGA) {
               refreshEGA();
            }
         }
private: System::Void btnUnusedPalette_Click(System::Object^  sender, System::EventArgs^  e) {
            for(byte i = 0; i < EGA_PALETTE_COLORS; ++i){
               m_palette[i] = EGA_COLOR_UNUSED;
            }
            palettePanel->Invalidate();
            if(m_currentItem && m_currentItem->type == FileTypes::EGA) {
               refreshEGA();
            }
         }
private: System::Void palettePanel_MouseClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
            if(e->Button == System::Windows::Forms::MouseButtons::Right){
               if(m_paletteDragging)
                  return;

               if(!m_currentItem || m_currentItem->type == FileTypes::PNG) {
                  auto x = e->X / 20;

                  if(x >= 0 && x < 16) {
                     auto val = m_palette[x];
                     if(val == EGA_COLOR_UNDEFINED)
                        m_palette[x] = EGA_COLOR_UNUSED;
                     else
                        m_palette[x] = EGA_COLOR_UNDEFINED;

                     palettePanel->Invalidate();
                  }
               }
            }
            else if(e->Button == System::Windows::Forms::MouseButtons::Left){
               auto x = e->X / 20;

               if(x >= 0 && x < 16) {
                  colorPicker(x);
               }
            }
         }

private: System::Void fileView_NodeMouseClick(System::Object^  sender, System::Windows::Forms::TreeNodeMouseClickEventArgs^  e) {

            if(e->Node->Tag){
               auto selected = (FileInfo^)e->Node->Tag;
               
               switch(selected->type){
               case FileTypes::Folder:
                  break;
               case FileTypes::PAL:
                  loadPALFile(selected);
                  break;
               case FileTypes::EGA:
                  deleteTempImage();
                  loadEGAFile(selected);
                  break;
               case FileTypes::PNG:
                  deleteTempImage();
                  loadPNGFile(selected);
                  break;
               }
            }
         }
private: System::Void palettePanel_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
            auto x = e->X / 20;

            if(x >= 0 && x < 16) {
               if(m_paletteDragging  && m_palette[x] != m_dragValue){
                  m_palette[x] = m_dragValue;
                  palettePanel->Invalidate();
               }

               auto str = "p"+x+": ";
               byte value = m_palette[x];

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

               showStatus(str);
            }
         }
private: System::Void palettePanel_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
            auto x = e->X / 20;

            if(x >= 0 && x < 16) {
               if(e->Button == System::Windows::Forms::MouseButtons::Right) {
                  if(!m_currentItem || m_currentItem->type == FileTypes::PNG) {
                     if(m_palette[x] != EGA_COLOR_UNDEFINED){                     
                        m_dragValue = EGA_COLOR_UNDEFINED;
                     }
                     else{
                        m_dragValue = EGA_COLOR_UNUSED;
                     }
                     m_paletteDragging = true;
                  }
                  
               }
            }
         }
private: System::Void palettePanel_MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
            if(e->Button == System::Windows::Forms::MouseButtons::Right) {
               if(!m_currentItem || m_currentItem->type == FileTypes::PNG)
                  m_paletteDragging = false;
            }
         }
private: System::Void fileView_DragDrop(System::Object^  sender, System::Windows::Forms::DragEventArgs^  e) {

            if(e->Data->GetDataPresent("System.Windows.Forms.TreeNode", false)){
               auto pt = ((TreeView^)sender)->PointToClient(Point(e->X, e->Y));
				   auto destNode = ((TreeView^)sender)->GetNodeAt(pt);
               auto newNode = (TreeNode^)e->Data->GetData("System.Windows.Forms.TreeNode");

               if(destNode && destNode->Tag && newNode && newNode->Tag){
                  auto info = (FileInfo^)newNode->Tag;
                  auto destInfo = (FileInfo^)destNode->Tag;

                  if(info->type != FileTypes::Folder && destInfo->type == FileTypes::Folder){
                     auto toPath =  
                        destInfo->path + 
                        IO::Path::DirectorySeparatorChar + 
                        IO::Path::GetFileName(info->path);

                     if(askFileOverwrite(toPath)){
                        if(IO::File::Exists(toPath)){
                           IO::File::Delete(toPath);
                        }

                        IO::File::Move(info->path, toPath);
                        auto newInfo = gcnew FileInfo(info->type, toPath);
                        refreshFiles(newInfo);
                     }
                  }
               }
            }
            
         }
private: System::Void fileView_DragEnter(System::Object^  sender, System::Windows::Forms::DragEventArgs^  e) {
            e->Effect = DragDropEffects::Move;
         }
private: System::Void fileView_ItemDrag(System::Object^  sender, System::Windows::Forms::ItemDragEventArgs^  e) {
            DoDragDrop(e->Item, DragDropEffects::Move);
         }
};
}
