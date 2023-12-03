#pragma once
#include "scene.hpp"
#include "grid.hpp"

namespace mini {
	class ik_scene : public scene_base {
		private:
			struct robot_configuration_t {
				float theta1;
				float theta2;

				robot_configuration_t() :
					theta1(0.0f),
					theta2(0.0f) { }

				robot_configuration_t(float theta1, float theta2) :
					theta1(theta1),
					theta2(theta2) { }
			};

			struct obstacle_t {
				glm::vec2 position;
				glm::vec2 size;

				obstacle_t(const glm::vec2& position, const glm::vec2& size) :
					position(position),
					size(size) { }

				obstacle_t(float x, float y, float w, float h) :
					position(x, y),
					size(w, h) { }
			};

			uint32_t m_domain_res_x;
			uint32_t m_domain_res_y;

			float m_arm1_len;
			float m_arm2_len;

			robot_configuration_t m_start_config;
			robot_configuration_t m_end_config;

			int m_last_vp_width, m_last_vp_height;

			// objects
			std::shared_ptr<grid_object> m_grid;

		public:
			ik_scene(application_base& app);
			ik_scene(const ik_scene&) = delete;
			ik_scene& operator=(const ik_scene&) = delete;

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;
			virtual void menu() override;

		private:
			void m_gui_settings();
			void m_gui_viewport();
	};
}