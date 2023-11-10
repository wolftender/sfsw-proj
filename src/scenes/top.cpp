#include "gui.hpp"
#include "scenes/top.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace mini {
	top_scene::top_scene(application_base& app) : scene_base(app),
		m_display_cube(true),
		m_display_plane(true),
		m_display_diagonal(true),
		m_display_path(true),
		m_viewport_focus(false),
		m_mouse_in_viewport(false),
		m_max_data_points(MAX_DATA_POINTS),
		m_num_data_points(0UL),
		m_last_vp_width(0),
		m_last_vp_height(0),
		m_distance(10.0f),
		m_cam_pitch(-0.7f),
		m_cam_yaw(0.0f),
		m_camera_target{0.0f, 0.0f, 0.0f} {

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

	bool top_scene::is_viewport_focused() const {
		return m_viewport_focus;
	}

	bool top_scene::is_mouse_in_viewport() const {
		return m_mouse_in_viewport;
	}

	void top_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Spinning Top", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}

	void top_scene::integrate(float delta_time) {
		// scene interaction
		m_handle_mouse();

		// camera uptading
		gui::clamp(m_distance, 1.0f, 30.0f);

		glm::vec4 cam_pos = { 0.0f, 0.0f, -m_distance, 1.0f };
		glm::mat4x4 cam_rotation(1.0f);

		cam_rotation = glm::translate(cam_rotation, m_camera_target);
		cam_rotation = glm::rotate(cam_rotation, m_cam_yaw, { 0.0f, 1.0f, 0.0f });
		cam_rotation = glm::rotate(cam_rotation, m_cam_pitch, { 1.0f, 0.0f, 0.0f });

		cam_pos = cam_rotation * cam_pos;

		get_app().get_context().get_camera().set_position(cam_pos);
		get_app().get_context().get_camera().set_target(m_camera_target);
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
		if (is_viewport_focused() && is_mouse_in_viewport()) {
			m_distance = m_distance - (static_cast<float> (offset_y) / 2.0f);
		}
	}

	void top_scene::m_handle_mouse() {
		if (get_app().is_left_click() && m_viewport_focus) {
			const offset_t& last_pos = get_app().get_last_mouse_offset();
			const offset_t& curr_pos = get_app().get_mouse_offset();

			int d_yaw = curr_pos.x - last_pos.x;
			int d_pitch = curr_pos.y - last_pos.y;

			float f_d_yaw = static_cast<float> (d_yaw);
			float f_d_pitch = static_cast<float> (d_pitch);

			f_d_yaw = f_d_yaw * 15.0f / static_cast<float> (get_app().get_width());
			f_d_pitch = f_d_pitch * 15.0f / static_cast<float> (get_app().get_height());

			m_cam_yaw = m_cam_yaw - f_d_yaw;
			m_cam_pitch = m_cam_pitch + f_d_pitch;
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

		if (ImGui::IsWindowFocused()) {
			m_viewport_focus = true;
		} else {
			m_viewport_focus = false;
		}

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto window_pos = ImGui::GetWindowPos();

		int width = static_cast<int>(max.x - min.x);
		int height = static_cast<int>(max.y - min.y);

		const offset_t& mouse_offset = get_app().get_mouse_offset();
		m_vp_mouse_offset.x = mouse_offset.x - static_cast<int> (min.x + window_pos.x);
		m_vp_mouse_offset.y = mouse_offset.y - static_cast<int> (min.y + window_pos.y);

		if (m_vp_mouse_offset.x < 0 || m_vp_mouse_offset.x > width || m_vp_mouse_offset.y < 0 || m_vp_mouse_offset.y > height) {
			m_mouse_in_viewport = false;
		} else {
			m_mouse_in_viewport = true;
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
}
