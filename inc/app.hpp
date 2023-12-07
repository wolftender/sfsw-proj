#pragma once
#include "window.hpp"
#include "context.hpp"
#include "store.hpp"
#include "grid.hpp"
#include "scene.hpp"

namespace mini {
	class application : public application_base {
		private:
			std::unique_ptr<scene_base> m_scene;
			bool m_layout_ready;

			app_context m_context;
			resource_store m_store;

		public:
			application();

			virtual scene_base& get_scene() override;
			virtual app_context& get_context() override;
			virtual const app_context& get_context() const override;
			virtual resource_store& get_store() override;

		protected:
			virtual void t_integrate(float delta_time) override;
			virtual void t_render() override;

			virtual void t_on_character(unsigned int code) override;
			virtual void t_on_cursor_pos(double posx, double posy) override;
			virtual void t_on_mouse_button(int button, int action, int mods) override;
			virtual void t_on_key_event(int key, int scancode, int action, int mods) override;
			virtual void t_on_scroll(double offset_x, double offset_y) override;
			virtual void t_on_resize(int width, int height) override;

		private:
			void m_draw_main_menu();
			void m_draw_main_window();

			void m_load_scene_spring();
			void m_load_scene_top();
			void m_load_scene_rotation();
			void m_load_scene_soft();
			void m_load_scene_ik();
	};
}