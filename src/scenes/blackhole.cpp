#include "gui.hpp"
#include "scenes/blackhole.hpp"

namespace mini {
	black_hole_scene::black_hole_scene(application_base& app) : scene_base(app) {
		const std::array<std::string, 6> cubemap_files = {
			"textures/cubemap/px.png",
			"textures/cubemap/nx.png",
			"textures/cubemap/py.png",
			"textures/cubemap/ny.png",
			"textures/cubemap/pz.png",
			"textures/cubemap/nz.png"
		};

		m_cubemap = cubemap::load_from_files(cubemap_files);
	}

	black_hole_scene::~black_hole_scene() {
	}

	void black_hole_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
		auto dock_id_bottom_right = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.8f, nullptr, &dock_id_right);

		ImGui::DockBuilderDockWindow("Black Hole Simulation", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}

	void black_hole_scene::integrate(float delta_time) {
	}

	void black_hole_scene::render(app_context& context) {
	}

	void black_hole_scene::gui() {
		m_gui_settings();
		m_gui_viewport();
	}

	void black_hole_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	void black_hole_scene::m_gui_viewport() {
		auto& context = get_app().get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(320, 240));
		ImGui::Begin("Black Hole Simulation", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(640, 480), ImGuiCond_Once);

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void black_hole_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		auto width = max.x - min.x;
		auto height = max.y - min.y;

		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}