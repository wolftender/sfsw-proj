#include "gui.hpp"
#include "scenes/top.hpp"

namespace mini {
	top_scene::top_scene(application_base& app) : scene_base(app),
		m_display_cube(true),
		m_display_plane(true),
		m_display_diagonal(true),
		m_display_path(true),
		m_max_data_points(MAX_DATA_POINTS),
		m_num_data_points(0UL),
		m_last_vp_width(0),
		m_last_vp_height(0) {

		std::fill(m_path_points.begin(), m_path_points.end(), glm::vec3{0.0f, 0.0f, 0.0f});

		auto line_shader = get_app().get_store().get_shader("line");
		auto cube_shader = get_app().get_store().get_shader("cube");
		auto grid_shader = get_app().get_store().get_shader("grid_xz");

		get_app().get_context().set_clear_color({0.75f, 0.75f, 0.9f});
	}

	top_scene::~top_scene() { }

	void top_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Spinning Top", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}

	void top_scene::integrate(float delta_time) {
	}

	void top_scene::render(app_context& context) {
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
		auto& context = get_app().get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(320, 240));
		ImGui::Begin("Spinning Top", NULL);
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
