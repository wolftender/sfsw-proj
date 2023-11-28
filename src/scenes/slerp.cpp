#include "gui.hpp"
#include "scenes/slerp.hpp"

#include <iostream>
#include <glm/gtx/compatibility.hpp>

namespace mini {
	// quaternion implementation
	quaternion operator*(const float s, const quaternion& q) {
		return quaternion{ q.w * s, q.x * s, q.y * s, q.z * s };
	}

	quaternion operator*(const quaternion& q, const float s) {
		return s * q;
	}

	quaternion operator*(const quaternion& q1, const quaternion& q2) {
		auto w1 = q1.w, x1 = q1.x, y1 = q1.y, z1 = q1.z;
		auto w2 = q2.w, x2 = q2.x, y2 = q2.y, z2 = q2.z;

		return quaternion{
			w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
			w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
			w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
			w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2
		};
	}

	quaternion operator+(const quaternion& q1, const quaternion& q2) {
		return quaternion{
			q1.w + q2.w,
			q1.x + q2.x,
			q1.y + q2.y,
			q1.z + q2.z,
		};
	}

	quaternion operator/(const quaternion& q, const float s) {
		return quaternion{ q.w / s, q.x / s, q.y / s, q.z / s };
	}

	quaternion conjugate(const quaternion& q) {
		return quaternion{ q.w, -q.x, -q.y, -q.z };
	}

	float norm(const quaternion& q) {
		return glm::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	}

	quaternion normalize(const quaternion& q) {
		auto n = norm(q);
		return q / n;
	}

	quaternion angle_axis(float angle, const glm::vec3& axis) {
		angle = angle * 0.5f;
		auto s = glm::sin(angle);

		return normalize({
			glm::cos(angle),
			s * axis.x,
			s * axis.y,
			s * axis.z
		});
	}

	glm::mat4x4 quat_to_matrix(const quaternion& q) {
		auto x = q.x, y = q.y, z = q.z, w = q.w;
		auto xx = x * x * 2;
		auto yx = y * x * 2;
		auto yy = y * y * 2;
		auto zx = z * x * 2;
		auto zy = z * y * 2;
		auto zz = z * z * 2;
		auto wx = w * x * 2;
		auto wy = w * y * 2;
		auto wz = w * z * 2;

		return glm::mat4x4{
			1.0f - yy - zz, yx - wz, zx + wy, 0.0f,
			yx + wz, 1.0f - xx - zz, zy - wx, 0.0f,
			zx - wy, zy + wx, 1.0f - xx - yy, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	quaternion quat_lerp(const quaternion& q1, const quaternion& q2, float t) {
		auto ax = q1.x, ay = q1.y, az = q1.z, aw = q1.w;
		auto bx = q2.x, by = q2.y, bz = q2.z, bw = q2.w;
		auto dot = ax * bx + ay * by + az * bz + aw * bw;

		dot = glm::clamp(dot, -1.0f, 1.0f);

		auto z = q2;
		if (dot < 0.0f) {
			dot = -dot;
			z = q2 * (-1.0f);
		}

		return normalize((q1 * (1.0f - t)) + (z * t));
	}

	quaternion quat_slerp(const quaternion& q1, const quaternion& q2, float t) {
		auto ax = q1.x, ay = q1.y, az = q1.z, aw = q1.w;
		auto bx = q2.x, by = q2.y, bz = q2.z, bw = q2.w;
		auto dot = ax * bx + ay * by + az * bz + aw * bw;

		dot = glm::clamp(dot, -1.0f, 1.0f);

		auto z = q2;
		if (dot < 0.0f) {
			dot = -dot;
			z = q2 * (-1.0f);
		}

		//t = t * 0.5f;
		float theta = glm::acos(dot);

		float st = glm::sin(theta);
		float sut = glm::sin(t * theta);
		float sout = glm::sin((1.0f - t) * theta);
		float c1 = sout / st;
		float c2 = sut / st;

		quaternion q{
			c1 * q1.w + c2 * z.w,
			c1 * q1.x + c2 * z.x,
			c1 * q1.y + c2 * z.y,
			c1 * q1.z + c2 * z.z
		};

		return normalize(q);
	}

	float deg_to_rad(float deg) {
		constexpr auto pi = glm::pi<float>();
		return (deg / 180.0f) * pi;
	}

	float rad_to_deg(float rad) {
		constexpr auto pi = glm::pi<float>();
		return (rad / pi) * 180.0f;
	}

	// scene
	slerp_scene::simulation_state_t::simulation_state_t(simulation_settings_t settings, bool animate) :
		settings(settings), animate(animate), elapsed(0.0f) { 
		
		this->settings.start_rotation_e[0] = deg_to_rad(this->settings.start_rotation_e[0]);
		this->settings.start_rotation_e[1] = deg_to_rad(this->settings.start_rotation_e[1]);
		this->settings.start_rotation_e[2] = deg_to_rad(this->settings.start_rotation_e[2]);

		this->settings.end_rotation_e[0] = deg_to_rad(this->settings.end_rotation_e[0]);
		this->settings.end_rotation_e[1] = deg_to_rad(this->settings.end_rotation_e[1]);
		this->settings.end_rotation_e[2] = deg_to_rad(this->settings.end_rotation_e[2]);
	}

	void slerp_scene::simulation_state_t::update(float delta_time) {
		elapsed += 0.3f * delta_time;

		if (elapsed > 1.0f) {
			elapsed = 1.0f;
		}
	}

	glm::mat4x4 slerp_scene::simulation_state_t::get_transform_e(float t) {
		glm::vec3 position = glm::lerp(settings.start_position, settings.end_position, t);
		glm::vec3 euler = glm::lerp(settings.start_rotation_e, settings.end_rotation_e, t);

		quaternion rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

		rotation = rotation * angle_axis(euler[2], glm::vec3{ 0.0f, 0.0f, 1.0f });
		rotation = rotation * angle_axis(euler[1], glm::vec3{ 0.0f, 1.0f, 0.0f });
		rotation = rotation * angle_axis(euler[0], glm::vec3{ 1.0f, 0.0f, 0.0f });

		glm::mat4x4 transform(1.0f);
		transform = glm::translate(transform, position);
		transform = transform * quat_to_matrix(rotation);

		return transform;
	}

	glm::mat4x4 slerp_scene::simulation_state_t::get_transform_q(float t) {
		glm::vec3 position = glm::lerp(settings.start_position, settings.end_position, t);
		quaternion rotation;

		if (settings.slerp) {
			rotation = quat_slerp(settings.start_rotation_q, settings.end_rotation_q, t);
		} else {
			rotation = quat_lerp(settings.start_rotation_q, settings.end_rotation_q, t);
		}

		glm::mat4x4 transform(1.0f);
		transform = glm::translate(transform, position);
		transform = transform * quat_to_matrix(rotation);

		return transform;
	}

	slerp_scene::slerp_scene(application_base& app) : 
		scene_base(app),
		m_state(m_settings, true),
		m_context1(app.get_context()),
		m_context2(video_mode_t(600, 400)),
		m_viewport1(app, m_context1, "Quaternion"),
		m_viewport2(app, m_context2, "Euler Angles") {

		m_context1.set_clear_color({ 0.75f, 0.75f, 0.9f });
		m_context2.set_clear_color({ 0.75f, 0.75f, 0.9f });

		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto gizmo_shader = get_app().get_store().get_shader("gizmo");

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (gizmo_shader) {
			m_gizmo = std::make_shared<gizmo>(gizmo_shader);
		}
	}

	void slerp_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id);
		auto dock_id_top_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Quaternion", dockspace_id);
		ImGui::DockBuilderDockWindow("Euler Angles", dock_id_top_left);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_bottom);
	}

	void slerp_scene::integrate(float delta_time) {
		m_viewport1.update(delta_time);
		m_viewport2.update(delta_time);

		m_state.update(delta_time);
	}

	void slerp_scene::render(app_context& context) {
		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			m_context1.draw(m_grid, grid_model);
			m_context2.draw(m_grid, grid_model);
		}

		if (m_gizmo) {
			if (m_state.animate) {
				auto gizmo_model1 = m_state.get_transform_q(m_state.elapsed);
				auto gizmo_model2 = m_state.get_transform_e(m_state.elapsed);

				m_context1.draw(m_gizmo, gizmo_model1);
				m_context2.draw(m_gizmo, gizmo_model2);
			} else {
				auto num_frames = m_state.settings.num_frames;
				float step = 1.0f / static_cast<float>(num_frames);

				for (int i = 0; i <= num_frames; ++i) {
					float t = glm::min(1.0f, glm::max(0.0f, i * step));

					auto gizmo_model1 = m_state.get_transform_q(t);
					auto gizmo_model2 = m_state.get_transform_e(t);

					m_context1.draw(m_gizmo, gizmo_model1);
					m_context2.draw(m_gizmo, gizmo_model2);
				}
			}
		}

		m_context2.display(false, true);
	}

	void slerp_scene::gui() {
		m_gui_settings();
		m_viewport1.display();
		m_viewport2.display();
	}

	void slerp_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	inline void angle_clamp(glm::vec3& euler) {
		if (euler.x > 180.0f) {
			euler.x = -180.0f + (euler.x - floorf(euler.x / 180.0f)*180.0f);
		}

		if (euler.y > 180.0f) {
			euler.y = -180.0f + (euler.y - floorf(euler.y / 180.0f)*180.0f);
		}

		if (euler.z > 180.0f) {
			euler.z = -180.0f + (euler.z - floorf(euler.z / 180.0f)*180.0f);
		}

		if (euler.x < -180.0f) {
			euler.x = 180.0f + (euler.x + floorf(fabsf(euler.x) / 180.0f)*180.0f);
		}

		if (euler.y < -180.0f) {
			euler.y = 180.0f + (euler.y + floorf(fabsf(euler.y) / 180.0f)*180.0f);
		}

		if (euler.z < -180.0f) {
			euler.z = 180.0f + (euler.z + floorf(fabsf(euler.z) / 180.0f)*180.0f);
		}
	}

	inline void joint_rotation_editor(const std::string_view id, quaternion& q, glm::vec3& e, bool& quat_mode) {
		std::string id_checkbox = std::format("##_{}_checkbox", id);
		std::string id_quat_x = std::format("##_{}_qx", id);
		std::string id_quat_y = std::format("##_{}_qy", id);
		std::string id_quat_z = std::format("##_{}_qz", id);
		std::string id_quat_w = std::format("##_{}_qw", id);

		gui::prefix_label("Quaternion Input: ", 250.0f);
		ImGui::Checkbox(id_checkbox.c_str(), &quat_mode);

		bool changed = false;
			
		if (quat_mode) {
			gui::prefix_label("w: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_w.c_str(), &q.w) || changed;

			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_x.c_str(), &q.x) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_y.c_str(), &q.y) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_z.c_str(), &q.z) || changed;

			// convert quat to euler
			if (changed) {
				e = glm::eulerAngles(glm::quat(q.w, q.x, q.y, q.z));

				e[0] = rad_to_deg(e[0]);
				e[1] = rad_to_deg(e[1]);
				e[2] = rad_to_deg(e[2]);
			}
		} else {
			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_x.c_str(), &e.x) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_y.c_str(), &e.y) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_z.c_str(), &e.z) || changed;

			// convert euler to quat
			if (changed) {
				angle_clamp(e);

				quaternion rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

				rotation = rotation * angle_axis(deg_to_rad(e[2]), glm::vec3{ 0.0f, 0.0f, 1.0f });
				rotation = rotation * angle_axis(deg_to_rad(e[1]), glm::vec3{ 0.0f, 1.0f, 0.0f });
				rotation = rotation * angle_axis(deg_to_rad(e[0]), glm::vec3{ 1.0f, 0.0f, 0.0f });

				q = rotation;
			}
		}
	}

	inline void normalize_quaternion(quaternion& q) {
		auto n = norm(q);
		if (n < 0.01f) {
			q.x = 0.0f;
			q.y = 0.0f;
			q.z = 0.0f;
			q.w = 1.0f;
		} else if (n != 1.0f) {
			q = normalize(q);
		}
	}

	void slerp_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto width = max.x - min.x;

		ImGui::BeginChild("Start Position", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("Start Position", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::vector_editor_2("start_pos", m_settings.start_position);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Start Rotation", ImVec2(width * 0.2f, 0));
		
		if (ImGui::CollapsingHeader("Start Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
			joint_rotation_editor("start_rot", m_settings.start_rotation_q, m_settings.start_rotation_e, m_settings.start_quat_mode);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("End Position", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("End Position", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::vector_editor_2("end_pos", m_settings.end_position);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("End Rotation", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("End Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
			joint_rotation_editor("end_rot", m_settings.end_rotation_q, m_settings.end_rotation_e, m_settings.end_quat_mode);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Simulation Control", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("Simulation Control", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Frames: ", 250.0f);
			ImGui::InputInt("##num_frames", &m_settings.num_frames, 1);

			gui::prefix_label("Use Slerp: ", 250.0f);
			ImGui::Checkbox("##use_slerp", &m_settings.slerp);

			m_settings.num_frames = glm::min(10, glm::max(2, m_settings.num_frames));

			if (ImGui::Button("Show Frames")) {
				normalize_quaternion(m_settings.start_rotation_q);
				normalize_quaternion(m_settings.end_rotation_q);

				m_state = simulation_state_t(m_settings, false);
			}

			if (ImGui::Button("Run Animation")) {
				normalize_quaternion(m_settings.start_rotation_q);
				normalize_quaternion(m_settings.end_rotation_q);

				m_state = simulation_state_t(m_settings, true);
			}
		}

		ImGui::EndChild();
		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}