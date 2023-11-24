#pragma once
#include "scene.hpp"
#include "viewport.hpp"
#include "grid.hpp"
#include "gizmo.hpp"

namespace mini {
	class slerp_scene : public scene_base {
		private:
			app_context& m_context1;
			app_context m_context2;

			viewport_window m_viewport1;
			viewport_window m_viewport2;

			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<gizmo> m_gizmo;

		public:
			slerp_scene(application_base& app);
			slerp_scene(const slerp_scene&) = delete;
			slerp_scene& operator=(const slerp_scene&) = delete;

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;
			virtual void menu() override;

		private:
			void m_gui_settings();
	};
}