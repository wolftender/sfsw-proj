#pragma once
#include <imgui.h>

#include "window.hpp"
#include "context.hpp"
#include "store.hpp"

namespace mini {
	class scene_base;

	class application_base : public app_window {
		public:
			using app_window::app_window;

			virtual scene_base& get_scene() = 0;
			virtual app_context& get_context() = 0;
			virtual resource_store& get_store() = 0;
	};

	class scene_base {
		private:
			application_base& m_app;

		public:
			application_base& get_app() {
				return m_app;
			}

			scene_base(application_base& app) : m_app(app) { }

			virtual void layout(ImGuiID dockspace_id) = 0;
			virtual void integrate(float delta_time) = 0;
			virtual void render(app_context & context) = 0;
			virtual void gui() = 0;

			virtual void on_character(unsigned int code) {}
			virtual void on_cursor_pos(double posx, double posy) {}
			virtual void on_mouse_button(int button, int action, int mods) {}
			virtual void on_key_event(int key, int scancode, int action, int mods) {}
			virtual void on_scroll(double offset_x, double offset_y) {}
			virtual void on_resize(int width, int height) {}
	};
}