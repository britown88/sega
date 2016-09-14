#include "pch.h"
#include "UWPMain.h"
#include "Common\DirectXHelper.h"

#include "SEGA/App.h"
#include "BTGame\BT.h"
#include "DeviceContext.h"
#include "Renderer.h"

using namespace UWP;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

class UWPMain::EGARenderer{
   std::shared_ptr<DX::DeviceResources> m_deviceResources;
public:
   EGARenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :m_deviceResources(deviceResources) {

   }

   ~EGARenderer() {
   }

   void Render() {
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

   }

   virtual void OnDeviceLost() {

   }
   virtual void OnDeviceRestored() {

   }
};



// Loads and initializes application assets when the application is loaded.
UWPMain::UWPMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources), m_renderer(new EGARenderer(deviceResources))
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
}

UWPMain::~UWPMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

Windows::Foundation::Size UWPMain::getOutputSize() {
   return m_deviceResources->GetOutputSize();
}
void UWPMain::RenderEGA() {

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
}

// Updates the application state once per frame.
void UWPMain::Update() 
{
   
	// Update scene objects.
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);

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
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

// Notifies renderers that device resources need to be released.
void UWPMain::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void UWPMain::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
