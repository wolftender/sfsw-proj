#include <stdexcept>
#include <iostream>

#include "window.hpp"

// use imgui library
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>
#include <implot_internal.h>

namespace mini {
	void GLAPIENTRY gl_error_callback (GLenum source, GLenum type, GLuint id,
		GLenum severity, GLsizei length, const GLchar * message, const void * userParam) {
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
			return;
		}

		std::cerr << "opengl error: " << (type == GL_DEBUG_TYPE_ERROR ? "** ERROR **" : "") 
			<< " type = " << type << ", severity = " << severity << ", message = " << message << std::endl;
	}

	void glfw_window_deleter_t::operator()(GLFWwindow * window) {
		glfwDestroyWindow (window);
	}

	uint32_t app_window::get_width () const {
		return m_width;
	}

	uint32_t app_window::get_height () const {
		return m_height;
	}

	const std::string & app_window::get_title () const {
		return m_title;
	}

	const offset_t & app_window::get_mouse_offset () const {
		return m_mouse;
	}

	const offset_t & app_window::get_last_mouse_offset () const {
		return m_last_mouse;
	}

	bool app_window::is_left_click () const {
		return m_left_click;
	}

	bool app_window::is_right_click () const {
		return m_right_click;
	}

	bool app_window::is_middle_click () const {
		return m_middle_click;
	}

	bool app_window::is_key_down (int key) const {
		auto iter = m_pressed_keys.find (key);
		return (iter != m_pressed_keys.end ());
	}

	void app_window::set_width (uint32_t width) {
		m_width = width;
		glfwSetWindowSize (m_window.get (), m_width, m_height);
	}

	void app_window::set_height (uint32_t height) {
		m_height = height;
		glfwSetWindowSize (m_window.get (), m_width, m_height);
	}

	void app_window::set_size (uint32_t width, uint32_t height) {
		m_width = width;
		m_height = height;

		glfwSetWindowSize (m_window.get (), m_width, m_height);
	}

	void app_window::set_title (const std::string & title) {
		m_title = title;
		glfwSetWindowTitle (m_window.get (), m_title.c_str ());
	}

	app_window::app_window (uint32_t width, uint32_t height, const std::string & title) {
		m_width = width;
		m_height = height;
		m_title = title;

		// use opengl 4.3
		glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint (GLFW_RESIZABLE, true);

		GLFWwindow * window = glfwCreateWindow (m_width, m_height, m_title.c_str (), nullptr, nullptr);

		if (!window) {
			throw std::runtime_error ("failed to create glfw window");
		}

		// setup opengl context
		glfwMakeContextCurrent (window);

		if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress)) {
			throw std::runtime_error ("failed to initialize opengl context");
		}

		// this will print errors
		glEnable (GL_DEBUG_OUTPUT);
		glDebugMessageCallback (gl_error_callback, 0);

		m_window.reset (window);

		glfwSwapInterval (1); // vsync
		glfwSetWindowUserPointer (window, reinterpret_cast<void*> (this));

		glfwSetKeyCallback (window, app_window::s_on_key_event);
		glfwSetCharCallback (window, app_window::s_on_character);
		glfwSetCursorPosCallback (window, app_window::s_on_cursor_pos);
		glfwSetMouseButtonCallback (window, app_window::s_on_mouse_button);
		glfwSetWindowSizeCallback (window, app_window::s_on_resize);
		glfwSetScrollCallback (window, app_window::s_on_scroll);

		glfwSetWindowSizeLimits (window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);

		// setup imgui
		m_setup_imgui ();
	}

	app_window::~app_window () { }

	void app_window::message_loop () {
		m_last_frame = std::chrono::steady_clock::now();

		while (!glfwWindowShouldClose (m_window.get ())) {
			glfwPollEvents ();

			// calculate delta time
			auto now = std::chrono::steady_clock::now ();
			float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_frame).count () / 1000.0f;

			m_last_frame = now;

			ImGui_ImplOpenGL3_NewFrame ();
			ImGui_ImplGlfw_NewFrame ();
			ImGui::NewFrame ();

			t_integrate (elapsed);
			t_render ();

			ImGui::Render ();
			ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

			glfwSwapBuffers (m_window.get ());
		}
	}

	void app_window::s_on_key_event (GLFWwindow * window, int key, int scancode, int action, int mods) {
		void * user_ptr = glfwGetWindowUserPointer (window);

		if (!user_ptr) {
			return;
		}

		app_window * target_ptr = reinterpret_cast<app_window *> (user_ptr);
		target_ptr->t_on_key_event (key, scancode, action, mods);
	}

	void app_window::s_on_character (GLFWwindow * window, unsigned int code) {
		void * user_ptr = glfwGetWindowUserPointer (window);

		if (!user_ptr) {
			return;
		}

		app_window * target_ptr = reinterpret_cast<app_window *> (user_ptr);
		target_ptr->t_on_character (code);
	}

	void app_window::s_on_cursor_pos (GLFWwindow * window, double posx, double posy) {
		void * user_ptr = glfwGetWindowUserPointer (window);

		if (!user_ptr) {
			return;
		}

		app_window * target_ptr = reinterpret_cast<app_window *> (user_ptr);
		target_ptr->t_on_cursor_pos (posx, posy);
	}

	void app_window::s_on_mouse_button (GLFWwindow * window, int button, int action, int mods) {
		void * user_ptr = glfwGetWindowUserPointer (window);

		if (!user_ptr) {
			return;
		}

		app_window * target_ptr = reinterpret_cast<app_window *> (user_ptr);
		target_ptr->t_on_mouse_button (button, action, mods);
	}

	void app_window::s_on_resize (GLFWwindow * window, int width, int height) {
		void * user_ptr = glfwGetWindowUserPointer (window);

		if (!user_ptr) {
			return;
		}

		app_window * target_ptr = reinterpret_cast<app_window *> (user_ptr);
		target_ptr->t_on_resize (width, height);
	}

	void app_window::s_on_scroll (GLFWwindow * window, double offset_x, double offset_y) {
		void * user_ptr = glfwGetWindowUserPointer (window);

		if (!user_ptr) {
			return;
		}

		app_window * target_ptr = reinterpret_cast<app_window *> (user_ptr);
		target_ptr->t_on_scroll (offset_x, offset_y);
	}

	void app_window::m_setup_imgui () {
		// setup imgui
		IMGUI_CHECKVERSION ();
		ImGui::DebugCheckVersionAndDataLayout (IMGUI_VERSION, sizeof (ImGuiIO), sizeof (ImGuiStyle), sizeof (ImVec2), sizeof (ImVec4), sizeof (ImDrawVert), sizeof (ImDrawIdx));

		ImGui::CreateContext ();
		ImPlot::CreateContext ();

		ImGuiIO & io = ImGui::GetIO (); (void)io;

		// dont generate ini file
		io.IniFilename = nullptr;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.Fonts->AddFontFromFileTTF ("opensans.ttf", 16);

		constexpr const char * glslVersion = "#version 150";

		ImGui_ImplGlfw_InitForOpenGL (m_window.get (), true);
		ImGui_ImplOpenGL3_Init (glslVersion);

		ImGui::StyleColorsLight ();
		m_setup_imgui_style ();
	}

	void app_window::m_setup_imgui_style () {
	}

	void app_window::t_integrate (float delta_time) { 
		m_last_mouse = m_mouse;
	}

	void app_window::t_render () { }

	void app_window::t_on_key_event (int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			m_pressed_keys.insert (key);
		} else if (action == GLFW_RELEASE) {
			m_pressed_keys.erase (key);
		}
	}

	void app_window::t_on_character (unsigned int code) { }

	void app_window::t_on_cursor_pos (double posx, double posy) {
		m_last_mouse.x = m_mouse.x;
		m_last_mouse.y = m_mouse.y;

		m_mouse.x = static_cast<int> (posx);
		m_mouse.y = static_cast<int> (posy);
	}

	void app_window::t_on_mouse_button (int button, int action, int mods) {
		bool press_state = (action != GLFW_RELEASE);
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				m_left_click = press_state;
				break;

			case GLFW_MOUSE_BUTTON_RIGHT:
				m_right_click = press_state;
				break;

			case GLFW_MOUSE_BUTTON_MIDDLE:
				m_middle_click = press_state;
				break;

			default: break;
		}
	}

	void app_window::t_on_resize (int width, int height) { 
		m_width = width;
		m_height = height;
	}

	void app_window::t_on_scroll (double offset_x, double offset_y) { }
}