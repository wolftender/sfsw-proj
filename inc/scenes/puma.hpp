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
			static constexpr glm::mat4x4 CONVERT_MTX = {
				-1.0f,  0.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 1.0f, 0.0f,
				 0.0f, -1.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 0.0f, 1.0f
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

			puma_target_t m_puma_start;
			puma_target_t m_puma_end;

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

			void m_solve_ik_dbg(app_context& context, puma_config_t& config, const puma_target_t& target) const;
			void m_solve_ik(puma_config_t& config, const puma_target_t& target) const;
		
			void m_draw_puma(app_context& context, const puma_config_t& config) const;
			void m_draw_frame(app_context& context, const glm::mat4x4& transform) const;
			void m_draw_debug(app_context& context, const glm::vec3& position) const;

			void m_gui_settings();
	};
}