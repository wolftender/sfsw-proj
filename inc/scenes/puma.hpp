#pragma once
#include "scene.hpp"
#include "viewport.hpp"
#include "grid.hpp"
#include "gizmo.hpp"
#include "mesh.hpp"
#include "model.hpp"

#include <glm/gtc/quaternion.hpp>

namespace mini {
	class puma_scene : public scene_base {
		private:
			struct puma_config_t {
				float l1, l2, l3;
				float q1, q2, q3, q4, q5;

				puma_config_t() {
					l1 = 3.5f;
					l2 = 2.5f;
					l3 = 2.0f;
					q3 = 3.0f;
					q1 = q2 = q4 = q5 = 0.0f;
				}
			};

			app_context& m_context1;
			app_context m_context2;

			viewport_window m_viewport1;
			viewport_window m_viewport2;

			puma_config_t m_config1;
			puma_config_t m_config2;

			std::shared_ptr<grid_object> m_grid;

			std::shared_ptr<triangle_mesh> m_effector_mesh;
			std::shared_ptr<triangle_mesh> m_arm_mesh;
			std::shared_ptr<triangle_mesh> m_joint_mesh;

			std::shared_ptr<model_object> m_effector_model_x;
			std::shared_ptr<model_object> m_effector_model_y;
			std::shared_ptr<model_object> m_effector_model_z;
			std::shared_ptr<model_object> m_arm_model;
			std::shared_ptr<model_object> m_joint_model;

			float m_distance;

		public:
			puma_scene(application_base& app);
			puma_scene(const puma_scene&) = delete;
			puma_scene& operator=(const puma_scene&) = delete;

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;
			virtual void menu() override;

			virtual void on_scroll(double offset_x, double offset_y) override;

		private:
			std::shared_ptr<triangle_mesh> m_make_effector_mesh();

			void m_draw_puma(app_context& context, const puma_config_t& config) const;
			void m_gui_settings();
	};
}