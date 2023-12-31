#include "gui.hpp"
#include "scenes/puma.hpp"

#include <glm/gtx/vector_angle.hpp>

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
		m_effector1{ 0.0f, 0.0f, 0.0f },
		m_effector2{ 0.0f, 0.0f, 0.0f },
		m_manual_control(false),
		m_follow_effector(false),
		m_loop_animation(false),
		m_flashlight(false),
		m_debug_points(false),
		m_viewport1(app, m_context1, "Config Interp."),
		m_viewport2(app, m_context2, "Effector Interp.") {

		m_context1.set_clear_color({ 0.75f, 0.75f, 0.9f });
		m_context2.set_clear_color({ 0.75f, 0.75f, 0.9f });

		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto puma_shader = get_app().get_store().get_shader("puma");
		auto point_shader = get_app().get_store().get_shader("point");

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (point_shader) {
			m_point_object = std::make_shared<billboard_object>(point_shader);
			m_point_object->set_color_tint({ 1.0f, 0.0f, 0.0f, 1.0f });
			m_point_object->set_size({ 16.0f, 16.0f });
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

			m_debug_mesh = triangle_mesh::make_cube_mesh();
			m_debug_model = std::make_shared<model_object>(m_debug_mesh, puma_shader);
			m_debug_model->set_surface_color({ 0.0f, 0.0f, 1.0f, 1.0f });
		}

		auto camera = std::make_unique<default_camera>();
		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
	}

	void puma_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.4f, nullptr, &dockspace_id);
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

		if (m_follow_effector) {
			m_viewport1.set_camera_target(m_effector1);
			m_viewport2.set_camera_target(m_effector2);
		}

		if (!m_manual_control) {
			m_solve_ik(m_config1, m_meta1, m_puma_start);
			m_solve_ik(m_config2, m_meta2, m_puma_start);
		} else {
			m_config2 = m_config1;
		}
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

	inline float oriented_angle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& n) {
		return atan2f(glm::dot(glm::cross(a, b), n), glm::dot(a, b));
	}

	void puma_scene::m_solve_ik(puma_config_t& config, puma_solution_meta_t& meta, const puma_target_t& target) const {
		auto p0 = glm::vec3{ 0.0f, 0.0f, 0.0f };
		auto p1 = glm::vec3{ 0.0f, 0.0f, config.l1 };
		auto p4 = target.position;

		auto xdir = glm::vec3(target.build_matrix_raw() * glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f });
		auto ydir = glm::vec3(target.build_matrix_raw() * glm::vec4{ 0.0f, 1.0f, 0.0f, 0.0f });
		auto p3 = target.position - xdir * config.l3;

		auto p_ = p4 - p3;
		auto n = glm::normalize(glm::cross(p1 - p3, p1 - p0));

		auto num = config.l2 * config.l2;
		auto den = 1.0f;
		den += (n.x * n.x) / (n.y * n.y);

		float xv, yv, zv;

		if (p_.z != 0.0f) {
			auto m = (p_.x - p_.y * n.x / n.y) / p_.z;
			den += m * m;

			xv = glm::sqrt(num / den); // + or -
			zv = -m * xv;
			yv = -n.x * xv / n.y;
		} else {
			auto m = (p_.x - p_.y * n.x / n.y);
			xv = 0.0f;
			yv = 0.0f;
			zv = config.l2;
		}

		// v = P2 - P3
		// P2 = v + P3
		auto p2 = glm::vec3{ p3.x + xv, p3.y + yv, p3.z + zv };
		auto v12 = p2 - p1;
		auto v23 = p3 - p2;
		auto v34 = p4 - p3;

		config.q1 = atan2f(p2.y, p2.x);
		config.q2 = atan2f(v12.z, glm::length(glm::vec2{ v12.x, v12.y }));
		config.q3 = glm::distance(p1, p2);
		//config.q4 = atan2f(v23.z, glm::length(glm::vec2{ v23.x, v23.y })) + config.q2 + HPI;

		config.q4 = oriented_angle(glm::normalize(v12), glm::normalize(v23), n) - HPI;

		glm::vec3 fwd2 = rotation_mat(n, config.q4) * glm::vec4(glm::normalize(v12), 0.0f);
		config.q5 = oriented_angle(glm::normalize(fwd2), xdir, -glm::normalize(v23));

		glm::vec3 left4 = rotation_mat(glm::normalize(v23), -config.q5) * glm::vec4(n, 0.0f);
		config.q6 = oriented_angle(left4, ydir, xdir);

		meta.p1 = p1;
		meta.p2 = p2;
		meta.p3 = p3;
		meta.p4 = p4;
	}

	void puma_scene::m_draw_puma(app_context& context, const puma_config_t& config, glm::vec3& effector_pos) const {
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
			rotation_mat(AXIS_Y, -HPI - config.q4) * rotation_mat(AXIS_Z, config.q5);

		joint_matrix[3] = joint_matrix[2] * translation_mat({ 0.0f, 0.0f, config.l2 }) * 
			rotation_mat(AXIS_Z, config.q5) * rotation_mat(AXIS_Y, HPI);
		arm_matrix[3] = arm_matrix[2] * translation_mat({ 0.0f, 0.0f, config.l2 }) *
			rotation_mat(AXIS_Y, HPI);

		joint_matrix[4] = joint_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 });

		joint_matrix[1] = joint_matrix[1] * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[2] = joint_matrix[2] * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[3] = joint_matrix[3] * rotation_mat(AXIS_Y, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[4] = joint_matrix[4] * translation_mat({ 0.0f, 0.0f, -0.35f });

		constexpr glm::mat4x4 EFFECTOR_TO_WORLD = {
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		effector_matrix = arm_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 }) * rotation_mat(AXIS_Z, -config.q6) * EFFECTOR_TO_WORLD;
		effector_pos = arm_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 }) * rotation_mat(AXIS_Z, -config.q6) * EFFECTOR_TO_WORLD * 
			glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };

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

	void puma_scene::m_draw_debug(app_context& context, const glm::vec3& position) const {
		context.draw(m_point_object, CONVERT_MTX * translation_mat(position));
	}

	void puma_scene::m_setup_light(app_context& context) const {
		context.clear_lights();
		auto& light = context.get_light(0);

		light.color = { 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.att_const = 1.0f;

		if (m_flashlight) {
			light.position = context.get_camera().get_position();
		} else {
			light.position = { -5.0f, -5.0f, -5.0f };
		}
	}

	void puma_scene::render(app_context& context) {
		m_setup_light(m_context1);
		m_setup_light(m_context2);

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			m_context1.draw(m_grid, grid_model);
			m_context2.draw(m_grid, grid_model);
		}

		if (m_arm_mesh) {
			m_draw_puma(m_context1, m_config1, m_effector1);
			m_draw_puma(m_context2, m_config2, m_effector2);

			m_draw_frame(m_context1, m_puma_start.build_matrix());
			m_draw_frame(m_context1, m_puma_end.build_matrix());

			m_draw_frame(m_context2, m_puma_start.build_matrix());
			m_draw_frame(m_context2, m_puma_end.build_matrix());
		}

		if (m_debug_points) {
			m_draw_debug(m_context1, m_meta1.p1);
			m_draw_debug(m_context1, m_meta1.p2);
			m_draw_debug(m_context1, m_meta1.p3);
			m_draw_debug(m_context1, m_meta1.p4);

			m_draw_debug(m_context2, m_meta2.p1);
			m_draw_debug(m_context2, m_meta2.p2);
			m_draw_debug(m_context2, m_meta2.p3);
			m_draw_debug(m_context2, m_meta2.p4);
		}

		m_context2.display(false, true);
	}

	template<typename... Args>
	inline void padded_text(const char* fmt, const ImVec2& padding, Args... args) {
		auto cursor_pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor_pos.x + padding.x, cursor_pos.y + padding.y));
		ImGui::Text(fmt, args ...);
	}

	void puma_scene::gui() {
		const auto overlay_handler = [](const puma_config_t& config, const puma_solution_meta_t& meta) {
			if (ImGui::CollapsingHeader("Configuration")) {
				ImGui::SetWindowSize(ImVec2(150, 180));
				padded_text("q1 = %.4f", ImVec2(8.0f, 0.0f), config.q1);
				padded_text("q2 = %.4f", ImVec2(8.0f, 0.0f), config.q2);
				padded_text("q3 = %.4f", ImVec2(8.0f, 0.0f), config.q3);
				padded_text("q4 = %.4f", ImVec2(8.0f, 0.0f), config.q4);
				padded_text("q5 = %.4f", ImVec2(8.0f, 0.0f), config.q5);
				padded_text("q6 = %.4f", ImVec2(8.0f, 0.0f), config.q6);

				ImGui::Dummy(ImVec2(0.0f, 5.0f));
			}
		};

		m_gui_settings();

		m_viewport1.display([&]() {
			overlay_handler(m_config1, m_meta1);
		});

		m_viewport2.display([&]() {
			overlay_handler(m_config2, m_meta2);
		});
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

	bool vector_editor_slider(const std::string& label, glm::vec3& vector) {
		const std::string label_x = "##" + label + "_x";
		const std::string label_y = "##" + label + "_y";
		const std::string label_z = "##" + label + "_z";

		bool changed = false;

		gui::prefix_label("X: ", 100.0f);
		changed = ImGui::DragFloat(label_x.c_str(), &vector[0], 0.1f, -10.0f, 10.0f) || changed;

		gui::prefix_label("Y: ", 100.0f);
		changed = ImGui::DragFloat(label_y.c_str(), &vector[1], 0.1f, -10.0f, 10.0f) || changed;

		gui::prefix_label("Z: ", 100.0f);
		changed = ImGui::DragFloat(label_z.c_str(), &vector[2], 0.1f, -10.0f, 10.0f) || changed;

		return changed;
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
			changed = ImGui::DragFloat(id_quat_w.c_str(), &q.w, 0.01f, 0.0f, 1.0f) || changed;

			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_x.c_str(), &q.x, 0.01f, 0.0f, 1.0f) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_y.c_str(), &q.y, 0.01f, 0.0f, 1.0f) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_z.c_str(), &q.z, 0.01f, 0.0f, 1.0f) || changed;

			// convert quat to euler
			if (changed) {
				q = glm::normalize(q);
				e = glm::eulerAngles(q);

				e[0] = rad_to_deg(e[0]);
				e[1] = rad_to_deg(e[1]);
				e[2] = rad_to_deg(e[2]);
			}
		} else {
			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_x.c_str(), &e.x, 1.0f, -90.0f, 90.0f) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_y.c_str(), &e.y, 1.0f, -90.0f, 90.0f) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_z.c_str(), &e.z, 1.0f, -90.0f, 90.0f) || changed;

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

		ImGui::BeginChild("Start Config", ImVec2(width * 0.33f, 0));

		if (ImGui::CollapsingHeader("Start Config", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Rotation")) {
				joint_rotation_editor("start_rot", m_puma_start.rotation, m_puma_start.euler_angles, m_puma_start.quat_mode);
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Position")) {
				vector_editor_slider("start_pos", m_puma_start.position);
				ImGui::TreePop();
			}

			if (ImGui::Button("Reset", ImVec2(glm::min(width * 0.3f, 150.0f), 0.0f))) {
				m_puma_start = puma_target_t();
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("End Config", ImVec2(width * 0.33f, 0));

		if (ImGui::CollapsingHeader("End Config", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Rotation")) {
				joint_rotation_editor("start_rot", m_puma_end.rotation, m_puma_end.euler_angles, m_puma_end.quat_mode);
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Position")) {
				vector_editor_slider("start_pos", m_puma_end.position);
				ImGui::TreePop();
			}

			if (ImGui::Button("Reset", ImVec2(glm::min(width * 0.3f, 150.0f), 0.0f))) {
				m_puma_end = puma_target_t();
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Simulation Control", ImVec2(width * 0.33f, 0));

		if (ImGui::CollapsingHeader("Simulation Control", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Manual Mode: ", 100.0f);
			ImGui::Checkbox("##puma_manual", &m_manual_control);

			gui::prefix_label("Flashlight: ", 100.0f);
			ImGui::Checkbox("##puma_flashlight", &m_flashlight);

			gui::prefix_label("IK Debug: ", 100.0f);
			ImGui::Checkbox("##puma_debug", &m_debug_points);

			gui::prefix_label("Follow Effector: ", 100.0f);
			ImGui::Checkbox("##puma_cinematic", &m_follow_effector);

			if (m_manual_control) {
				gui::prefix_label("q1: ", 100.0f);
				ImGui::SliderFloat("##puma_q1", &m_config1.q1, -PI, PI);

				gui::prefix_label("q2: ", 100.0f);
				ImGui::SliderFloat("##puma_q2", &m_config1.q2, -PI, PI);

				gui::prefix_label("q3: ", 100.0f);
				ImGui::SliderFloat("##puma_q3", &m_config1.q3, 0.0f, 10.0f);

				gui::prefix_label("q4: ", 100.0f);
				ImGui::SliderFloat("##puma_q4", &m_config1.q4, -PI, PI);

				gui::prefix_label("q5: ", 100.0f);
				ImGui::SliderFloat("##puma_q5", &m_config1.q5, -PI, PI);

				gui::prefix_label("q6: ", 100.0f);
				ImGui::SliderFloat("##puma_q6", &m_config1.q6, -PI, PI);
			} else {
				gui::prefix_label("Loop Animation: ", 100.0f);
				ImGui::Checkbox("##puma_loop", &m_loop_animation);

				ImGui::Button("Start", ImVec2(width * 0.1f, 25.0f));
				ImGui::SameLine();
				ImGui::Button("Pause", ImVec2(width * 0.1f, 25.0f));
				ImGui::SameLine();
				ImGui::Button("Reset", ImVec2(width * 0.1f, 25.0f));
			}
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
