#pragma once
#include "scene.hpp"
#include "viewport.hpp"
#include "grid.hpp"
#include "gizmo.hpp"

#include <glm/gtc/quaternion.hpp>

namespace mini {
	class puma_scene : public scene_base {
		private:
			app_context& m_context1;
			app_context m_context2;

			viewport_window m_viewport1;
			viewport_window m_viewport2;

			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<gizmo> m_gizmo;

		public:
			puma_scene(application_base& app);
			puma_scene(const puma_scene&) = delete;
			puma_scene& operator=(const puma_scene&) = delete;

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;
			virtual void menu() override;

		private:
			void m_gui_settings();
	};
}