#include "gui.hpp"
#include "scenes/puma.hpp"

namespace mini {
	inline float deg_to_rad(float deg) {
		constexpr auto pi = glm::pi<float>();
		return (deg / 180.0f) * pi;
	}

	inline float rad_to_deg(float rad) {
		constexpr auto pi = glm::pi<float>();
		return (rad / pi) * 180.0f;
	}

	puma_scene::puma_scene(application_base& app) : 
		scene_base(app), 
		m_context1(app.get_context()),
		m_context2(video_mode_t(600, 400)), 
		m_distance(10.0f),
		m_viewport1(app, m_context1, "Config Interp."),
		m_viewport2(app, m_context2, "Effector Interp.") {

		m_context1.set_clear_color({ 0.75f, 0.75f, 0.9f });
		m_context2.set_clear_color({ 0.75f, 0.75f, 0.9f });

		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto puma_shader = get_app().get_store().get_shader("puma");

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (puma_shader) {
			m_effector_mesh = m_make_effector_mesh();
			m_arm_mesh = triangle_mesh::make_cylinder(0.3f, 1.0f, 50, 20);
			m_joint_mesh = triangle_mesh::make_cylinder(0.5f, 0.7f, 50, 20);

			m_effector_model_x = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_effector_model_y = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_effector_model_z = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_arm_model = std::make_shared<model_object>(m_arm_mesh, puma_shader);
			m_joint_model = std::make_shared<model_object>(m_joint_mesh, puma_shader);

			m_effector_model_x->set_surface_color({ 1.0f, 0.0f, 0.0f, 1.0f });
			m_effector_model_y->set_surface_color({ 0.0f, 1.0f, 0.0f, 1.0f });
			m_effector_model_z->set_surface_color({ 0.0f, 0.0f, 1.0f, 1.0f });
			m_arm_model->set_surface_color({ 0.0f, 1.0f, 0.0f, 1.0f });
			m_joint_model->set_surface_color({0.0f, 0.0f, 1.0f, 1.0f});
		}

		auto camera = std::make_unique<default_camera>();
		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
	}

	void puma_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id);
		auto dock_id_top_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Config Interp.", dockspace_id);
		ImGui::DockBuilderDockWindow("Effector Interp.", dock_id_top_left);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_bottom);
	}

	void puma_scene::integrate(float delta_time) {
		m_viewport1.update(delta_time);
		m_viewport2.update(delta_time);

		m_viewport1.set_distance(m_distance);
		m_viewport2.set_distance(m_distance);

		if (m_viewport1.is_camera_moved()) {
			m_viewport2.set_cam_pitch(m_viewport1.get_cam_pitch());
			m_viewport2.set_cam_yaw(m_viewport1.get_cam_yaw());
		} else if (m_viewport2.is_camera_moved()) {
			m_viewport1.set_cam_pitch(m_viewport2.get_cam_pitch());
			m_viewport1.set_cam_yaw(m_viewport2.get_cam_yaw());
		}
	}

	inline void setup_light(app_context& context) {
		context.clear_lights();
		auto& light = context.get_light(0);

		light.color = { 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.position = { -5.0f, -5.0f, -5.0f };
		light.att_const = 1.0f;
	}

	inline glm::mat4x4 rotation_mat(const glm::vec3& axis, float angle) {
		constexpr glm::mat4x4 id(1.0f);
		return glm::rotate(id, angle, axis);
	}

	inline glm::mat4x4 translation_mat(const glm::vec3& vec) {
		constexpr glm::mat4x4 id(1.0f);
		return glm::translate(id, vec);
	}

	inline glm::mat4x4 scale_mat(const glm::vec3& vec) {
		constexpr glm::mat4x4 id(1.0f);
		return glm::scale(id, vec);
	}

	constexpr auto AXIS_X = glm::vec3{ 1.0f, 0.0f, 0.0f };
	constexpr auto AXIS_Y = glm::vec3{ 0.0f, 1.0f, 0.0f };
	constexpr auto AXIS_Z = glm::vec3{ 0.0f, 0.0f, 1.0f };
	constexpr auto PI = glm::pi<float>();
	constexpr auto HPI = 0.5f * PI;

	void puma_scene::m_draw_puma(app_context& context, const puma_config_t& config) const {
		std::array<glm::mat4x4, 4> arm_matrix;
		std::array<glm::mat4x4, 5> joint_matrix;
		glm::mat4x4 effector_matrix;

		std::fill(arm_matrix.begin(), arm_matrix.end(), glm::mat4x4(1.0f));
		std::fill(joint_matrix.begin(), joint_matrix.end(), glm::mat4x4(1.0f));
		effector_matrix = glm::mat4x4(1.0f);

		// forward kinematics
		joint_matrix[0] = rotation_mat(AXIS_X, HPI) * rotation_mat(AXIS_Z, -config.q1);
		arm_matrix[0] = rotation_mat(AXIS_X, HPI) * rotation_mat(AXIS_Z, -config.q1);

		joint_matrix[1] = joint_matrix[0] * translation_mat({ 0.0f, 0.0f, config.l1 }) * 
			rotation_mat(AXIS_Y, -HPI + config.q2);
		arm_matrix[1] = arm_matrix[0] * translation_mat({0.0f, 0.0f, config.l1}) * 
			rotation_mat(AXIS_Y, -HPI + config.q2);

		joint_matrix[2] = joint_matrix[1] * translation_mat({0.0f, 0.0f, config.q3 }) * 
			rotation_mat(AXIS_Y, -HPI - config.q4);
		arm_matrix[2] = arm_matrix[1] * translation_mat({ 0.0f, 0.0f, config.q3 }) *
			rotation_mat(AXIS_Y, -HPI - config.q4);

		joint_matrix[3] = joint_matrix[2] * translation_mat({ 0.0f, 0.0f, config.l2 }) * 
			rotation_mat(AXIS_Y, HPI);
		arm_matrix[3] = arm_matrix[2] * translation_mat({ 0.0f, 0.0f, config.l2 }) *
			rotation_mat(AXIS_Y, HPI);

		joint_matrix[4] = joint_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 });

		joint_matrix[1] = joint_matrix[1] * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[2] = joint_matrix[2] * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[3] = joint_matrix[3] * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[4] = joint_matrix[4] * translation_mat({ 0.0f, 0.0f, -0.35f });

		constexpr glm::mat4x4 EFFECTOR_TO_WORLD = {
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		effector_matrix = arm_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 }) * rotation_mat(AXIS_Z, -config.q5) * EFFECTOR_TO_WORLD;

		arm_matrix[0] = arm_matrix[0] * scale_mat({ 1.0f, 1.0f, config.l1 });
		arm_matrix[1] = arm_matrix[1] * scale_mat({ 1.0f, 1.0f, config.q3 });
		arm_matrix[2] = arm_matrix[2] * scale_mat({ 1.0f, 1.0f, config.l2 });
		arm_matrix[3] = arm_matrix[3] * scale_mat({ 1.0f, 1.0f, config.l3 });

		for (const auto& mat : joint_matrix) {
			context.draw(m_joint_model, mat);
		}

		for (const auto& mat : arm_matrix) {
			context.draw(m_arm_model, mat);
		}

		m_draw_frame(context, effector_matrix);
	}

	void puma_scene::m_draw_frame(app_context& context, const glm::mat4x4& transform) const {
		context.draw(m_effector_model_y, transform * translation_mat({ 0.0f, 0.8f, 0.0f }));
		context.draw(m_effector_model_z, transform * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.8f, 0.0f }));
		context.draw(m_effector_model_x, transform * rotation_mat(AXIS_Z, -HPI) * translation_mat({ 0.0f, 0.8f, 0.0f }));
	}

	void puma_scene::render(app_context& context) {
		setup_light(m_context1);
		setup_light(m_context2);

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			m_context1.draw(m_grid, grid_model);
			m_context2.draw(m_grid, grid_model);
		}

		if (m_arm_mesh) {
			m_draw_puma(m_context1, m_config1);
			m_draw_puma(m_context2, m_config2);

			m_draw_frame(m_context1, m_puma_start.build_matrix());
			m_draw_frame(m_context1, m_puma_end.build_matrix());

			m_draw_frame(m_context2, m_puma_start.build_matrix());
			m_draw_frame(m_context2, m_puma_end.build_matrix());
		}

		m_context2.display(false, true);
	}

	void puma_scene::gui() {
		m_gui_settings();
		m_viewport1.display();
		m_viewport2.display();
	}

	inline void angle_clamp(glm::vec3& euler) {
		if (euler.x > 180.0f) {
			euler.x = -180.0f + (euler.x - floorf(euler.x / 180.0f) * 180.0f);
		}

		if (euler.y > 180.0f) {
			euler.y = -180.0f + (euler.y - floorf(euler.y / 180.0f) * 180.0f);
		}

		if (euler.z > 180.0f) {
			euler.z = -180.0f + (euler.z - floorf(euler.z / 180.0f) * 180.0f);
		}

		if (euler.x < -180.0f) {
			euler.x = 180.0f + (euler.x + floorf(fabsf(euler.x) / 180.0f) * 180.0f);
		}

		if (euler.y < -180.0f) {
			euler.y = 180.0f + (euler.y + floorf(fabsf(euler.y) / 180.0f) * 180.0f);
		}

		if (euler.z < -180.0f) {
			euler.z = 180.0f + (euler.z + floorf(fabsf(euler.z) / 180.0f) * 180.0f);
		}
	}

	inline void joint_rotation_editor(const std::string_view id, glm::quat& q, glm::vec3& e, bool& quat_mode) {
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

				glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

				rotation = rotation * glm::angleAxis(deg_to_rad(e[2]), glm::vec3{ 0.0f, 0.0f, 1.0f });
				rotation = rotation * glm::angleAxis(deg_to_rad(e[1]), glm::vec3{ 0.0f, 1.0f, 0.0f });
				rotation = rotation * glm::angleAxis(deg_to_rad(e[0]), glm::vec3{ 1.0f, 0.0f, 0.0f });

				q = rotation;
			}
		}
	}

	void puma_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto width = max.x - min.x;

		ImGui::BeginChild("Start Position", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("Start Position", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::vector_editor_2("start_pos", m_puma_start.position);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Start Rotation", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("Start Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
			joint_rotation_editor("start_rot", m_puma_start.rotation, m_puma_start.euler_angles, m_puma_start.quat_mode);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("End Position", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("End Position", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::vector_editor_2("end_pos", m_puma_end.position);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("End Rotation", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("End Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
			joint_rotation_editor("start_rot", m_puma_end.rotation, m_puma_end.euler_angles, m_puma_end.quat_mode);
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Simulation Control", ImVec2(width * 0.2f, 0));

		if (ImGui::CollapsingHeader("Simulation Control", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SliderFloat("q1", &m_config1.q1, 0.0f, 10.0f);
			ImGui::SliderFloat("q2", &m_config1.q2, 0.0f, 10.0f);
			ImGui::SliderFloat("q3", &m_config1.q3, 0.0f, 10.0f);
			ImGui::SliderFloat("q4", &m_config1.q4, 0.0f, 10.0f);
			ImGui::SliderFloat("q5", &m_config1.q5, 0.0f, 10.0f);
		}

		ImGui::EndChild();

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void puma_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	void puma_scene::on_scroll(double offset_x, double offset_y) {
		bool vp1_focus = m_viewport1.is_viewport_focused() && m_viewport1.is_mouse_in_viewport();
		bool vp2_focus = m_viewport2.is_viewport_focused() && m_viewport2.is_mouse_in_viewport();

		if (vp1_focus || vp2_focus) {
			m_distance = m_distance - (static_cast<float> (offset_y) / 2.0f);
		}

		m_distance = glm::clamp(m_distance, 1.0f, 30.0f);
	}

	std::shared_ptr<triangle_mesh> puma_scene::m_make_effector_mesh() {
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> uvs;

		std::vector<GLuint> indices;

		const int res = 20;
		const float r1 = 0.25f;
		const float r2 = 0.18f;
		const float r3 = 0.18f;
		const float h = 0.5f;

		gizmo::make_gizmo_verts(positions, indices, res, r1, r2, r3, h);

		int num_vertices = positions.size() / 3;
		normals.resize(positions.size());
		uvs.resize(num_vertices * 2);

		std::fill(uvs.begin(), uvs.end(), 0.0f);

		// automatically calculate normals
		std::vector<glm::vec3> sums;
		sums.resize(num_vertices);
		std::fill(sums.begin(), sums.end(), glm::vec3{0.0f, 0.0f, 0.0f});

		int num_faces = indices.size() / 3;

		for (int face = 0; face < num_faces; ++face) {
			int base = face * 3;
			int i1 = indices[base + 0];
			int i2 = indices[base + 1];
			int i3 = indices[base + 2];

			int b1 = 3 * i1;
			int b2 = 3 * i2;
			int b3 = 3 * i3;

			auto p1 = glm::vec3{ positions[b1 + 0], positions[b1 + 1], positions[b1 + 2] };
			auto p2 = glm::vec3{ positions[b2 + 0], positions[b2 + 1], positions[b2 + 2] };
			auto p3 = glm::vec3{ positions[b3 + 0], positions[b3 + 1], positions[b3 + 2] };

			auto n1 = glm::cross(p2 - p1, p3 - p1);
			auto n2 = glm::cross(p1 - p3, p3 - p1);
			auto n3 = glm::cross(p1 - p3, p2 - p3);

			sums[i1] += n1;
			sums[i2] += n2;
			sums[i3] += n3;
		}

		for (int i = 0; i < num_vertices; ++i) {
			sums[i] = glm::normalize(sums[i]);
			normals[3 * i + 0] = sums[i].x;
			normals[3 * i + 1] = sums[i].y;
			normals[3 * i + 2] = sums[i].z;
		}

		return std::make_shared<triangle_mesh>(positions, normals, uvs, indices);
	}
}
