#pragma once
#include "scene.hpp"
#include "viewport.hpp"
#include "grid.hpp"
#include "gizmo.hpp"

#include <glm/gtc/quaternion.hpp>

namespace mini {
	struct quaternion {
		float w, x, y, z;

		quaternion() :
			w(1.0f), x(0.0f), y(0.0f), z(0.0f) { }
		quaternion(float w, float x, float y, float z) :
			w(w), x(x), y(y), z(z) { }
	};

	quaternion operator*(const float s, const quaternion& q);
	quaternion operator*(const quaternion& q, const float s);
	quaternion operator*(const quaternion& q1, const quaternion& q2);
	quaternion operator+(const quaternion& q1, const quaternion& q2);
	quaternion operator/(const quaternion& q, const float s);

	quaternion conjugate(const quaternion& q);
	float norm(const quaternion& q);
	quaternion normalize(const quaternion& q);
	quaternion angle_axis(float angle, const glm::vec3& axis);
	glm::mat4x4 quat_to_matrix(const quaternion& q);
	quaternion quat_lerp(const quaternion& q1, const quaternion& q2, float t);
	quaternion quat_slerp(const quaternion& q1, const quaternion& q2, float t);

	class slerp_scene : public scene_base {
		private:
			struct simulation_settings_t {
				glm::vec3 start_position;
				glm::vec3 start_rotation_e;
				quaternion start_rotation_q;

				glm::vec3 end_position;
				glm::vec3 end_rotation_e;
				quaternion end_rotation_q;

				bool start_quat_mode, end_quat_mode;
				bool loop, slerp;
				int num_frames;

				simulation_settings_t() :
					start_position(0.0f, 0.0f, 0.0f),
					start_rotation_e(0.0f, 0.0f, 0.0f),
					start_rotation_q(1.0f, 0.0f, 0.0f, 0.0f),
					end_position(0.0f, 0.0f, 0.0f),
					end_rotation_e(0.0f, 0.0f, 0.0f),
					end_rotation_q(1.0f, 0.0f, 0.0f, 0.0f),
					start_quat_mode(false),
					end_quat_mode(false),
					loop(false),
					slerp(false),
					num_frames(10) { }
			};

			struct simulation_state_t {
				simulation_settings_t settings;
				float elapsed;
				bool animate;

				simulation_state_t(simulation_settings_t settings, bool animate);

				void update(float delta_time);

				glm::mat4x4 get_transform_e(float t);
				glm::mat4x4 get_transform_q(float t);
			};

			app_context& m_context1;
			app_context m_context2;

			viewport_window m_viewport1;
			viewport_window m_viewport2;

			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<gizmo> m_gizmo;

			simulation_settings_t m_settings;
			simulation_state_t m_state;

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