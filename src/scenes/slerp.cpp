#include "gui.hpp"
#include "scenes/slerp.hpp"

#include <iostream>

namespace mini {
	slerp_scene::slerp_scene(application_base& app) : 
		scene_base(app),
		m_context1(app.get_context()),
		m_context2(video_mode_t(600, 400)),
		m_viewport1(app, m_context1, "Slerp"),
		m_viewport2(app, m_context2, "Lerp") {

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

		ImGui::DockBuilderDockWindow("Slerp", dockspace_id);
		ImGui::DockBuilderDockWindow("Lerp", dock_id_top_left);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_bottom);
	}

	void slerp_scene::integrate(float delta_time) {
		m_viewport1.update(delta_time);
		m_viewport2.update(delta_time);
	}

	void slerp_scene::render(app_context& context) {
		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			m_context1.draw(m_grid, grid_model);
			m_context2.draw(m_grid, grid_model);
		}

		if (m_gizmo) {
			auto gizmo_model = glm::mat4x4(1.0f);
			m_context1.draw(m_gizmo, gizmo_model);
			m_context2.draw(m_gizmo, gizmo_model);
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

	inline void joint_rotation_editor(const std::string_view id, glm::quat& q, glm::vec3& e, bool& quat_mode) {
		std::string id_checkbox = std::format("##_{}_checkbox", id);
		std::string id_quat_x = std::format("##_{}_qx", id);
		std::string id_quat_y = std::format("##_{}_qy", id);
		std::string id_quat_z = std::format("##_{}_qz", id);
		std::string id_quat_w = std::format("##_{}_qw", id);

		gui::prefix_label("Quaternion Input: ", 250.0f);
		if (ImGui::Checkbox(id_checkbox.c_str(), &quat_mode)) {
			if (quat_mode) {
				// convert euler to quat
				glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

				rotation = rotation * glm::angleAxis(e[2], glm::vec3{ 0.0f, 0.0f, 1.0f });
				rotation = rotation * glm::angleAxis(e[1], glm::vec3{ 0.0f, 1.0f, 0.0f });
				rotation = rotation * glm::angleAxis(e[0], glm::vec3{ 1.0f, 0.0f, 0.0f });

				q = rotation;
			} else {
				// convert quat to euler
				e = glm::eulerAngles(q);
			}
		}
			
		if (quat_mode) {
			gui::prefix_label("w: ", 250.0f);
			ImGui::InputFloat(id_quat_w.c_str(), &q.w);

			gui::prefix_label("x: ", 250.0f);
			ImGui::InputFloat(id_quat_x.c_str(), &q.x);

			gui::prefix_label("y: ", 250.0f);
			ImGui::InputFloat(id_quat_y.c_str(), &q.y);

			gui::prefix_label("z: ", 250.0f);
			ImGui::InputFloat(id_quat_z.c_str(), &q.z);
		} else {
			gui::prefix_label("x: ", 250.0f);
			ImGui::InputFloat(id_quat_x.c_str(), &e.x);

			gui::prefix_label("y: ", 250.0f);
			ImGui::InputFloat(id_quat_y.c_str(), &e.y);

			gui::prefix_label("z: ", 250.0f);
			ImGui::InputFloat(id_quat_z.c_str(), &e.z);
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

			m_settings.num_frames = glm::min(10, glm::max(2, m_settings.num_frames));

			if (ImGui::Button("Show Frames")) {
				std::cout << "show animation frames" << std::endl;
			}

			if (ImGui::Button("Run Animation")) {
				std::cout << "run animation" << std::endl;
			}
		}

		ImGui::EndChild();
		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}