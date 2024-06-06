#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "scenes/blackhole.hpp"

namespace mini {
	black_hole_scene::black_hole_scene(application_base& app) : 
		scene_base(app), 
		m_last_vp_width(0), 
		m_last_vp_height(0),
		m_selected_map(0),
		m_cam_pitch(0.0f),
		m_cam_yaw(0.0f),
		m_viewport_focus(false) {

		const std::array<std::string, 6> test_cubemap_files = {
			"textures/testcube/px.png",
			"textures/testcube/nx.png",
			"textures/testcube/py.png",
			"textures/testcube/ny.png",
			"textures/testcube/pz.png",
			"textures/testcube/nz.png"
		};

		const std::array<std::string, 6> space_cubemap_files = {
			"textures/cubemap/px.png",
			"textures/cubemap/nx.png",
			"textures/cubemap/py.png",
			"textures/cubemap/ny.png",
			"textures/cubemap/pz.png",
			"textures/cubemap/nz.png"
		};

		m_cubemaps.push_back({ "Milky Way", cubemap::load_from_files(space_cubemap_files) });
		m_cubemaps.push_back({ "Debug", cubemap::load_from_files(test_cubemap_files) });

		auto bh_shader = app.get_store().get_shader("blackhole");
		if (!bh_shader) {
			throw std::runtime_error("black hole shader is not present!");
		}
		
		m_screenquad = std::make_shared<black_hole_quad>(
			bh_shader, m_cubemaps[m_selected_map].second);
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
		if (get_app().is_left_click() && m_viewport_focus) {
			const offset_t& last_pos = get_app().get_last_mouse_offset();
			const offset_t& curr_pos = get_app().get_mouse_offset();

			int d_yaw = curr_pos.x - last_pos.x;
			int d_pitch = curr_pos.y - last_pos.y;

			float f_d_yaw = static_cast<float> (d_yaw);
			float f_d_pitch = static_cast<float> (d_pitch);

			f_d_yaw = f_d_yaw * 15.0f / static_cast<float> (get_app().get_width());
			f_d_pitch = f_d_pitch * 15.0f / static_cast<float> (get_app().get_height());

			m_cam_yaw = m_cam_yaw + f_d_yaw;
			m_cam_pitch = m_cam_pitch + f_d_pitch;
		}
	}

	void black_hole_scene::render(app_context& context) {
		glm::mat4x4 world(1.0f);
		world = glm::rotate(world, m_cam_yaw, glm::vec3 {0.0f, 1.0f, 0.0f});
		world = glm::rotate(world, m_cam_pitch, glm::vec3 {1.0f, 0.0f, 0.0f});

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

		if (ImGui::IsWindowFocused()) {
			m_viewport_focus = true;
		} else {
			m_viewport_focus = false;
		}

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

		auto distance = m_screenquad->get_distance();
		auto star_mass = m_screenquad->get_star_mass();

		gui::prefix_label("Distance: ", 250.0f);
		ImGui::DragFloat("##bh_dist", &distance, 0.1f, 10.0f, 1000.0f);

		gui::prefix_label("Star mass: ", 250.0f);
		ImGui::DragFloat("##bh_mass", &star_mass, 0.1f, 0.1f, 1000.0f);

		gui::prefix_label("Cubemap:", 250.0f);
		if (ImGui::BeginCombo("##bh_cubemap", m_cubemaps[m_selected_map].first.c_str())) {
			for (auto i = 0UL; i < m_cubemaps.size(); ++i) {
				const auto& map_data = m_cubemaps[i];

				if (ImGui::Selectable(map_data.first.c_str())) {
					m_selected_map = i;
					m_screenquad->set_cube_map(map_data.second);
				}
			}

			ImGui::EndCombo();
		}

		m_screenquad->set_distance(distance);
		m_screenquad->set_star_mass(star_mass);

		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}