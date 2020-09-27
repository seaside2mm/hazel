#pragma once

#include "Core.h"
#include "Window.h"
<<<<<<< HEAD
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/Event.h"
#include "Hazel/LayerStack.h"

=======
>>>>>>> parent of 8424ddc... EvetnCallback and Close Window
namespace Hazel {

	class HAZEL_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
<<<<<<< HEAD

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline static Application& Get() { return *s_Instance;  }
		inline Window& GetWindow() { return *m_Window;  }
=======
>>>>>>> parent of 8424ddc... EvetnCallback and Close Window
	private:
		std::unique_ptr<Window> m_Window; 
		bool m_Running = true;
		LayerStack m_LayerStack;
	private:
		static Application* s_Instance;
	};

	// To be defined in client
	Application* CreateApplication();
}

