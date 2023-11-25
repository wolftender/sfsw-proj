#include "gui.hpp"
#include "scenes/slerp.hpp"

#include <iostream>
#include <glm/gtx/compatibility.hpp>

namespace mini {
	slerp_scene::simulation_state_t::simulation_state_t(simulation_settings_t settings, bool animate) :
		settings(settings), animate(animate), elapsed(0.0f) { }

	void slerp_scene::simulation_state_t::update(float delta_time) {
		elapsed += 0.3f * delta_time;

		if (elapsed > 1.0f) {
			elapsed = 1.0f;
		}
	}

	glm::mat4x4 slerp_scene::simulation_state_t::get_transform_e(float t) {
		glm::vec3 position = glm::lerp(settings.start_position, settings.end_position, t);
		glm::vec3 euler = glm::lerp(settings.start_rotation_e, settings.end_rotation_e, t);

		glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

		rotation = rotation * glm::angleAxis(euler[2], glm::vec3{ 0.0f, 0.0f, 1.0f });
		rotation = rotation * glm::angleAxis(euler[1], glm::vec3{ 0.0f, 1.0f, 0.0f });
		rotation = rotation * glm::angleAxis(euler[0], glm::vec3{ 1.0f, 0.0f, 0.0f });

		glm::mat4x4 transform(1.0f);
		transform = glm::translate(transform, position);
		transform = transform * glm::mat4_cast(rotation);

		return transform;
	}

	glm::mat4x4 slerp_scene::simulation_state_t::get_transform_q(float t) {
		glm::vec3 position = glm::lerp(settings.start_position, settings.end_position, t);
		glm::quat rotation;

		if (settings.slerp) {
			rotation = glm::slerp(settings.start_rotation_q, settings.end_rotation_q, t);
		} else {
			rotation = glm::lerp(settings.start_rotation_q, settings.end_rotation_q, t);
		}

		glm::mat4x4 transform(1.0f);
		transform = glm::translate(transform, position);
		transform = transform * glm::mat4_cast(rotation);

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
			e = glm::eulerAngles(q);
		} else {
			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_x.c_str(), &e.x) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_y.c_str(), &e.y) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::InputFloat(id_quat_z.c_str(), &e.z) || changed;

			// convert euler to quat
			glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

			rotation = rotation * glm::angleAxis(e[2], glm::vec3{ 0.0f, 0.0f, 1.0f });
			rotation = rotation * glm::angleAxis(e[1], glm::vec3{ 0.0f, 1.0f, 0.0f });
			rotation = rotation * glm::angleAxis(e[0], glm::vec3{ 1.0f, 0.0f, 0.0f });

			q = rotation;
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
				m_state = simulation_state_t(m_settings, false);
			}

			if (ImGui::Button("Run Animation")) {
				m_state = simulation_state_t(m_settings, true);
			}
		}

		ImGui::EndChild();
		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}