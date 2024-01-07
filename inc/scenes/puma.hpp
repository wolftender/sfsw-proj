#pragma once
#include "scene.hpp"
#include "viewport.hpp"
#include "billboard.hpp"
#include "grid.hpp"
#include "gizmo.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "segments.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mini {
	class puma_scene : public scene_base {
		private:
			static constexpr glm::mat4x4 CONVERT_MTX = {
				-1.0f,  0.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 1.0f, 0.0f,
				 0.0f, -1.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 0.0f, 1.0f
			};

			enum class ik_mode_t {
				default_solution,
				alter_solution,
				closest_distance
			};

			struct puma_config_t {
				float l1, l2, l3;
				float q1, q2, q3, q4, q5, q6;

				puma_config_t() {
					l1 = 3.5f;
					l2 = 2.5f;
					l3 = 2.0f;
					q3 = 3.0f;
					q1 = q2 = q4 = q5 = q6 = 0.0f;
				}
			};

			struct puma_solution_meta_t {
				glm::vec3 p1, p2, p3, p4;
				puma_solution_meta_t() : 
					p1{ 0.0f, 0.0f, 0.0f },
					p2{ 0.0f, 0.0f, 0.0f },
					p3{ 0.0f, 0.0f, 0.0f },
					p4{ 0.0f, 0.0f, 0.0f } { }
			};

			struct puma_target_t {
				glm::vec3 position;
				glm::quat rotation;
				glm::vec3 euler_angles;
				bool quat_mode;

				puma_target_t() : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), 
					euler_angles{0.0f, 0.0f, 0.0f}, quat_mode(false) { }

				puma_target_t(const glm::vec3& position, const glm::quat& rotation) :
					position(position), rotation(rotation), euler_angles(glm::eulerAngles(rotation)), 
					quat_mode(false) { }

				glm::mat4x4 build_matrix_raw() const {
					glm::mat4x4 transform(1.0f);
					transform = glm::translate(transform, position);
					transform = transform * glm::mat4_cast(rotation);

					return transform;
				}

				glm::mat4x4 build_matrix() const {
					return CONVERT_MTX * build_matrix_raw();
				}
			};

			app_context& m_context1;
			app_context m_context2;

			viewport_window m_viewport1;
			viewport_window m_viewport2;

			puma_config_t m_config1;
			puma_config_t m_config2;

			puma_solution_meta_t m_meta1;
			puma_solution_meta_t m_meta2;

			glm::vec3 m_effector1;
			glm::vec3 m_effector2;

			puma_target_t m_puma_start;
			puma_target_t m_puma_end;

			// data for interpolation by angles
			puma_config_t m_start_config;
			puma_config_t m_end_config;

			puma_solution_meta_t m_start_meta;
			puma_solution_meta_t m_end_meta;

			// data for interpolation by configuration
			puma_target_t m_current_target;

			std::shared_ptr<grid_object> m_grid;

			std::shared_ptr<triangle_mesh> m_effector_mesh;
			std::shared_ptr<triangle_mesh> m_arm_mesh;
			std::shared_ptr<triangle_mesh> m_joint_mesh;
			std::shared_ptr<triangle_mesh> m_debug_mesh;

			std::shared_ptr<model_object> m_effector_model_x;
			std::shared_ptr<model_object> m_effector_model_y;
			std::shared_ptr<model_object> m_effector_model_z;
			std::shared_ptr<model_object> m_arm_model;
			std::shared_ptr<model_object> m_joint_model;
			std::shared_ptr<model_object> m_debug_model;
			std::shared_ptr<billboard_object> m_point_object;
			std::shared_ptr<segments_array> m_debug_lines1;
			std::shared_ptr<segments_array> m_debug_lines2;

			float m_distance;
			bool m_manual_control;
			bool m_follow_effector;
			bool m_loop_animation;
			bool m_flashlight;
			bool m_debug_points;

			bool m_begin_changed;
			bool m_end_changed;

			float m_anim_time;
			float m_anim_speed;
			bool m_anim_active;
			bool m_anim_paused;

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
			std::shared_ptr<segments_array> m_make_debug_mesh(std::shared_ptr<shader_program> line_shader);

			void m_solve_ik(puma_config_t& config, puma_solution_meta_t& meta, 
				const puma_target_t& target, ik_mode_t mode) const;
		
			void m_draw_puma(app_context& context, const puma_config_t& config, glm::vec3& effector_pos) const;
			void m_draw_frame(app_context& context, const glm::mat4x4& transform) const;
			void m_draw_debug(app_context& context, const glm::vec3& position) const;

			void m_draw_debug_mesh(app_context& context, std::shared_ptr<segments_array> mesh, 
				puma_solution_meta_t& meta) const;

			void m_setup_light(app_context& context) const;
			void m_gui_settings();
	};
}