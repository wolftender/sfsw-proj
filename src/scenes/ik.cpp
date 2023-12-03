#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "scenes/ik.hpp"

namespace mini {
	ik_scene::ik_scene(application_base& app) : 
		scene_base(app),
		m_domain_res_x(360),
		m_domain_res_y(360),
		m_arm1_len(30.0f),
		m_arm2_len(30.0f),
		m_last_vp_height(0),
		m_last_vp_width(0) {
		
		app.get_context().set_clear_color({ 0.95f, 0.95f, 0.95f });

		auto xy_grid_shader = app.get_store().get_shader("grid_xy");
		if (xy_grid_shader) {
			m_grid = std::make_shared<grid_object>(xy_grid_shader);
		}
	}

	void ik_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Inverse Kinematics", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}

	void ik_scene::integrate(float delta_time) {
	}

	void ik_scene::render(app_context& context) {
		auto& camera = get_app().get_context().get_camera();
		camera.set_position({ 0.0f, 0.0f, -10.0f });
		camera.set_target({ 0.0f, 0.0f, 0.0f });

		constexpr float pi = glm::pi<float>();

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			grid_model = glm::rotate(grid_model, pi * 0.5f, { 1.0f, 0.0f, 0.0f });

			context.draw(m_grid, grid_model);
		}
	}

	void ik_scene::gui() {
		m_gui_settings();
		m_gui_viewport();
	}

	void ik_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	void ik_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Arm 1 Len. : ", 250.0f);
			ImGui::InputFloat("##ik_arm1_len", &m_arm1_len);

			gui::prefix_label("Arm 2 Len. : ", 250.0f);
			ImGui::InputFloat("##ik_arm2_len", &m_arm2_len);

			ImGui::NewLine();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void ik_scene::m_gui_viewport() {
		auto& context = get_app().get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(320, 240));
		ImGui::Begin("Inverse Kinematics", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(640, 480), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto window_pos = ImGui::GetWindowPos();

		int width = static_cast<int>(max.x - min.x);
		int height = static_cast<int>(max.y - min.y);

		if ((width != m_last_vp_width || height != m_last_vp_height) && width > 8 && height > 8) {
			video_mode_t mode(width, height);

			context.set_video_mode(mode);

			m_last_vp_width = width;
			m_last_vp_height = height;
		} else {
			ImGui::ImageButton(
				reinterpret_cast<ImTextureID>(context.get_front_buffer()),
				ImVec2(width, height), ImVec2(0, 0), ImVec2(1, 1), 0);
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}