#pragma once
#include "scene.hpp"
#include "grid.hpp"
#include "segments.hpp"

namespace mini {
	class ik_scene : public scene_base {
		private:
			enum class mouse_mode_t {
				start_config,
				end_config,
				obstacle
			};

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
			int m_mouse_tool_id;
			bool m_alt_solution;

			robot_configuration_t m_start_config;
			robot_configuration_t m_current_config;
			robot_configuration_t m_end_config;

			int m_last_vp_width, m_last_vp_height;
			bool m_mouse_in_viewport, m_viewport_focus;
		    bool m_is_start_ok, m_is_end_ok;

			offset_t m_vp_mouse_offset;

			glm::vec2 m_start_point, m_end_point;

			// objects
			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<segments_array> m_robot_arm_start;
			std::shared_ptr<segments_array> m_robot_arm_end;
			std::shared_ptr<segments_array> m_robot_arm_curr;

			std::unique_ptr<camera> m_old_camera;
			mouse_mode_t m_mouse_mode;

		public:
			ik_scene(application_base& app);
			ik_scene(const ik_scene&) = delete;
			ik_scene& operator=(const ik_scene&) = delete;

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;
			virtual void menu() override;
			virtual void on_mouse_button(int button, int action, int mods) override;

		private:
			void m_gui_settings();
			void m_gui_viewport();
			
			void m_handle_mouse_click(float x, float y);
			void m_handle_mouse_release(float x, float y);

			std::shared_ptr<segments_array> m_build_robot_arm(
				std::shared_ptr<shader_program> line_shader) const;

			void m_configure_robot_arm(
				std::shared_ptr<segments_array>& arm, 
				const robot_configuration_t& config);

			void m_length_changed();
			void m_solve_start_ik();
			void m_solve_end_ik();
			bool m_solve_arm_ik(robot_configuration_t& config, float x, float y) const;
	};
}