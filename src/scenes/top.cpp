#include "gui.hpp"
#include "scenes/top.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace mini {
	top_scene::top_scene(application_base& app) : scene_base(app),
		m_viewport(app, "Spinning Top"),
		m_display_cube(true),
		m_display_plane(true),
		m_display_diagonal(true),
		m_display_path(true),
		m_max_data_points(MAX_DATA_POINTS),
		m_num_data_points(0UL) {

		std::fill(m_path_points.begin(), m_path_points.end(), glm::vec3{0.0f, 0.0f, 0.0f});

		auto line_shader = get_app().get_store().get_shader("line");
		auto cube_shader = get_app().get_store().get_shader("cube");
		auto grid_shader = get_app().get_store().get_shader("grid_xz");

		get_app().get_context().set_clear_color({0.75f, 0.75f, 0.9f});

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}
	}

	top_scene::~top_scene() { }

	void top_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Spinning Top", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}

	void top_scene::integrate(float delta_time) {
		// scene interaction
		m_viewport.update(delta_time);
	}

	void top_scene::render(app_context& context) {
		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}
	}

	void top_scene::gui() {
		m_gui_settings();
		m_gui_viewport();
	}

	void top_scene::menu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Export Data", "Ctrl + Shift + E", nullptr, true)) {
				m_export_data();
			}

			ImGui::EndMenu();
		}
	}

	void top_scene::on_scroll(double offset_x, double offset_y) {
		if (m_viewport.is_viewport_focused() && m_viewport.is_mouse_in_viewport()) {
			m_viewport.set_distance(m_viewport.get_distance() - (static_cast<float> (offset_y) / 2.0f));
		}
	}

	void top_scene::m_export_data() {
		// nothing to export
	}

	void top_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::CollapsingHeader("i wonder whats for", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Button("dinner");
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void top_scene::m_gui_viewport() {
		m_viewport.display();
	}
}
