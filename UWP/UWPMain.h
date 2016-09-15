#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Content\SampleFpsTextRenderer.h"

// Renders Direct2D and 3D content on the screen.
namespace UWP
{
	class UWPMain : public DX::IDeviceNotify
	{
	public:
		UWPMain(const std::shared_ptr<DX::DeviceResources>& deviceResources, Windows::UI::Core::CoreWindow ^window);
		~UWPMain();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

      Windows::Foundation::Size getOutputSize();
      Windows::Foundation::Point getMousePosition();
      void setMousePosition(Windows::Foundation::Point p);
      void RenderEGA(unsigned char *pixels, D2D1_RECT_F const &vp);

      void CloseGame();
      bool GameShouldClose();

      void DestroyGame();
      void StartGame();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
		std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

      class EGARenderer;
      std::unique_ptr<EGARenderer> m_renderer;

		// Rendering loop timer.
		DX::StepTimer m_timer;
      Platform::Agile<Windows::UI::Core::CoreWindow> m_window;

      float m_dipScale;
      Windows::Foundation::Point m_mousePosition;
	};
}