#pragma once
#include "scene.hpp"
#include "cubemap.hpp"

namespace mini {
	class black_hole_scene : public scene_base {
		private:
			std::shared_ptr<cubemap> m_cubemap;

		public:
			black_hole_scene(application_base& app);
			~black_hole_scene();

			black_hole_scene(const black_hole_scene&) = delete;
			black_hole_scene& operator=(const black_hole_scene&) = delete;

			void layout(ImGuiID dockspace_id) override;
			void integrate(float delta_time) override;
			void render(app_context& context) override;
			void gui() override;
			void menu() override;

		private:
			void m_gui_viewport();
			void m_gui_settings();
	};
}