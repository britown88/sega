#include "pch.h"
#include "UWPMain.h"
#include "Common\DirectXHelper.h"

#include "SEGA/App.h"
#include "BTGame\BT.h"
#include "DeviceContext.h"
#include "Renderer.h"
#include "segautils/BitTwiddling.h"
#include "segalib/EGA.h"

#include <mutex>

using namespace UWP;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

class UWPMain::EGARenderer{
   std::shared_ptr<DX::DeviceResources> m_deviceResources;
   ModelViewProjectionConstantBuffer m_constantBufferData;

   Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_whiteBrush;
   Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock1> m_stateBlock;
   Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_bmp;
   Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_backBmp;

   std::mutex m;
   D2D1_RECT_F m_rect;
public:
   EGARenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :m_deviceResources(deviceResources) {

      m_deviceResources->GetD2DFactory()->CreateDrawingStateBlock(&m_stateBlock);

      

      CreateDeviceDependentResources();
      CreateWindowSizeDependentResources();
   }

   void CreateDeviceDependentResources() {
      m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush);


      m_bmp = nullptr;
      m_backBmp = nullptr;
      auto dpi = m_deviceResources->GetDpi();

      D2D1_SIZE_U  size = { EGA_RES_WIDTH, EGA_RES_HEIGHT };
      D2D1_BITMAP_PROPERTIES1 properties = { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE }, dpi, dpi, D2D1_BITMAP_OPTIONS_NONE, 0 };

      int r = m_deviceResources->GetD2DDeviceContext()->CreateBitmap(size, nullptr, 0, &properties, &m_bmp);
      r = m_deviceResources->GetD2DDeviceContext()->CreateBitmap(size, nullptr, 0, &properties, &m_backBmp);
      
   }
   void CreateWindowSizeDependentResources() {
      Size outputSize = m_deviceResources->GetOutputSize();
   }
   void ReleaseDeviceDependentResources() {
      m_whiteBrush.Reset();
      m_bmp.Reset();
   }
   void Render() {
      ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();
      Windows::Foundation::Size logicalSize = m_deviceResources->GetLogicalSize();

      context->SaveDrawingState(m_stateBlock.Get());
      context->BeginDraw();

      context->Clear();


      //context->DrawImage()
      m.lock();
      context->DrawBitmap(m_bmp.Get(), &m_rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR );
      m.unlock();

      //context->DrawLine(D2D1::Point2F(0.0f, 0.0f), D2D1::Point2F(100.0f, 100.0f), m_whiteBrush.Get(), 10.0f);

      context->EndDraw();
      context->RestoreDrawingState(m_stateBlock.Get());

   }

   void SwapFrames(byte *pixels, D2D1_RECT_F const &vp) {
      //drawe pixels to back bmp
      static auto rect = D2D1::RectU(0, 0, EGA_RES_WIDTH, EGA_RES_HEIGHT);
      static auto o = D2D1::Point2U();
      m_backBmp->CopyFromMemory(&rect, pixels, EGA_RES_WIDTH * 4);

      m.lock();
      m_bmp->CopyFromBitmap(&o, m_backBmp.Get(), &rect);
      m_rect = vp;
      m.unlock();
   }


};



// Loads and initializes application assets when the application is loaded.
UWPMain::UWPMain(const std::shared_ptr<DX::DeviceResources>& deviceResources, Windows::UI::Core::CoreWindow ^window) :
	m_deviceResources(deviceResources), m_renderer(new EGARenderer(deviceResources)), m_window(window)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	// TODO: Replace this with your app's content initialization.
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
   m_timer.SetFixedTimeStep(false);
   m_window->PointerCursor = nullptr;

   m_dipScale = m_deviceResources->GetDpi() / 96.0f;

}

UWPMain::~UWPMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}



Windows::Foundation::Point UWPMain::getMousePosition() {
   return m_mousePosition;
}

void UWPMain::setMousePosition(Windows::Foundation::Point p) {
   m_mousePosition = p;
}

Windows::Foundation::Size UWPMain::getOutputSize() {
   return m_deviceResources->GetOutputSize();
}
void UWPMain::RenderEGA(unsigned char *pixels, D2D1_RECT_F const &vp) {
   m_renderer->SwapFrames(pixels, vp);
}

void UWPMain::CloseGame() {
   appQuit(appGet());
}
bool UWPMain::GameShouldClose() {
   return (bool)appRunning();
}

void UWPMain::DestroyGame() {
   appDestroy();
}
void UWPMain::StartGame() {
   IDeviceContext *context = createUWPContext(this);
   IRenderer *renderer = createUWPRenderer(context, this);
   VirtualApp *app = btCreate();

   appStart(app, renderer, context);
}

// Updates application state when the window size changes (e.g. device orientation change)
void UWPMain::CreateWindowSizeDependentResources() 
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
   m_renderer->CreateWindowSizeDependentResources();
}

// Updates the application state once per frame.
void UWPMain::Update() 
{
   
	// Update scene objects.
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
		//m_sceneRenderer->Update(m_timer);
		//m_fpsTextRenderer->Update(m_timer);

      appStep();
	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool UWPMain::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	//m_sceneRenderer->Render();
	//m_fpsTextRenderer->Render();
   m_renderer->Render();

   

	return true;
}

// Notifies renderers that device resources need to be released.
void UWPMain::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
   m_renderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void UWPMain::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
   m_renderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
