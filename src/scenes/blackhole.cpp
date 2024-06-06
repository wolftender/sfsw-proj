#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "scenes/blackhole.hpp"

namespace mini {
	black_hole_scene::black_hole_scene(application_base& app) : 
		scene_base(app), 
		m_last_vp_width(0), 
		m_last_vp_height(0) {

		const std::array<std::string, 6> cubemap_files = {
			"textures/cubemap/px.png",
			"textures/cubemap/nx.png",
			"textures/cubemap/py.png",
			"textures/cubemap/ny.png",
			"textures/cubemap/pz.png",
			"textures/cubemap/nz.png"
		};

		m_cubemap = cubemap::load_from_files(cubemap_files);

		auto bh_shader = app.get_store().get_shader("blackhole");
		if (!bh_shader) {
			throw std::runtime_error("black hole shader is not present!");
		}
		
		m_screenquad = std::make_shared<black_hole_quad>(bh_shader, m_cubemap);
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
		m_time += delta_time;
	}

	void black_hole_scene::render(app_context& context) {
		glm::mat4x4 world(1.0f);
		world = glm::rotate(world, m_time, glm::vec3 {0.0f, 1.0f, 0.0f});

		context.draw(m_screenquad, world);
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