#include "gui.hpp"
#include "scenes/flywheel.hpp"

namespace mini {
	void flywheel_scene::simulation_state_t::integrate(float delta_time) {
		time = time + delta_time;
	}

	flywheel_scene::flywheel_scene(application_base & app) : 
		scene_base(app),
		m_last_vp_width(0),
		m_last_vp_height(0),
		m_mouse_in_viewport(false),
		m_viewport_focus(false),
		m_vp_mouse_offset{0, 0} {
		
		app.get_context().set_clear_color({ 0.95f, 0.95f, 0.95f });

		auto xy_grid_shader = app.get_store().get_shader("grid_xy");
		auto line_shader = get_app().get_store().get_shader("line");
		
		if (xy_grid_shader) {
			m_grid = std::make_shared<grid_object>(xy_grid_shader);
		}
		
		if (line_shader) {
			m_wheel_curve = m_make_wheel_curve(line_shader);
			m_square_curve = m_make_square_curve(line_shader);
			
			std::vector<glm::vec3> curve_points = {
				glm::vec3 {-30.0f, 0.0f, 0.0f},
				glm::vec3 {+30.0f, 0.0f, 0.0f}
			};
			
			m_stick_curve = std::make_shared<curve>(line_shader, curve_points);
			m_stick_curve->set_color({0.0f, 0.0f, 0.0f, 1.0f});
		}
		
		auto camera = std::make_unique<ortho_camera>();
		camera->set_top(-12.0f);
		camera->set_bottom(12.0f);
		camera->set_left(-12.0f);
		camera->set_right(12.0f);

		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
	}
	
	flywheel_scene::~flywheel_scene() { }
	
	void flywheel_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Flywheel", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}
	
	void flywheel_scene::integrate(float delta_time) {
		m_state.integrate(delta_time);
	}
	
	void flywheel_scene::render(app_context& context) {
		auto& camera = get_app().get_context().get_camera();
		camera.set_position({ 0.0f, 0.0f, 20.0f });
		camera.set_target({ 0.0f, 0.0f, 0.0f });

		constexpr float pi = glm::pi<float>();

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			grid_model = glm::rotate(grid_model, pi * 0.5f, { 1.0f, 0.0f, 0.0f });

			context.draw(m_grid, grid_model);
		}
		
		if (m_wheel_curve) {
			auto wheel_model = glm::mat4x4(1.0f);
			float radius = m_state.wheel_radius;
			
			wheel_model = glm::translate(wheel_model, glm::vec3 { -radius, 0.0f, 0.0f });
			wheel_model = glm::scale(wheel_model, glm::vec3 { radius, radius, radius });
			
			auto mass_model = glm::mat4x4(1.0f);
			mass_model = glm::scale(mass_model, glm::vec3{1.75f, 1.0f, 1.0f});
			
			context.draw(m_wheel_curve, wheel_model);
			context.draw(m_square_curve, mass_model);
			context.draw(m_stick_curve, glm::mat4x4(1.0f));
		}
	}
	
	void flywheel_scene::gui() {
		m_gui_settings();
		m_gui_viewport();
	}
	
	void flywheel_scene::on_scroll(double offset_x, double offset_y) {
		
	}
	
	void flywheel_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}
	
	void flywheel_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		gui::prefix_label("Wheel radius: ", 250.0f);
		ImGui::DragFloat("##fwh_radius", &m_state.wheel_radius, 0.1f, 1.0f, 10.0f);
		
		float R = m_state.wheel_radius;
		
		gui::prefix_label("Stick len.: ", 250.0f);
		ImGui::DragFloat("##fwh_rod_len", &m_state.stick_length, 0.1f, 2.0f * R, 4.0f * R);
		
		ImGui::End();
		ImGui::PopStyleVar(1);
	}
	
	void flywheel_scene::m_gui_viewport() {
		auto& context = get_app().get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(320, 240));
		ImGui::Begin("Flywheel", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(640, 480), ImGuiCond_Once);

		if (ImGui::IsWindowFocused()) {
			m_viewport_focus = true;
		}
		else {
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
		}
		else {
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
	
	std::shared_ptr<curve> flywheel_scene::m_make_square_curve(
		std::shared_ptr<shader_program> line_shader) const {
		
		std::vector<glm::vec3> curve_points = {
			glm::vec3 {-0.5f, +0.5f, 0.0f},
			glm::vec3 {+0.5f, +0.5f, 0.0f},
			glm::vec3 {+0.5f, -0.5f, 0.0f},
			glm::vec3 {-0.5f, -0.5f, 0.0f},
			glm::vec3 {-0.5f, +0.5f, 0.0f}
		};
		
		auto object = std::make_shared<curve>(line_shader, curve_points);
		object->set_color({0.0f, 0.0f, 0.0f, 1.0f});
		
		return object;
	}
	
	std::shared_ptr<curve> flywheel_scene::m_make_wheel_curve(
		std::shared_ptr<shader_program> line_shader) const {
		std::vector<glm::vec3> curve_points;
		
		constexpr int steps = 100;
		constexpr float pi = glm::pi<float>();
		constexpr float radius = 1.0f;
		constexpr float step = 2.0f * pi / static_cast<float>(steps);
		
		for (int i = 0; i <= steps; ++i) {
			float t = step * static_cast<float>(i);
			
			glm::vec3 point = {
				radius * glm::cos(t),
				radius * glm::sin(t),
				0.0f
			};
			
			curve_points.push_back(point);
		}
		
		auto object = std::make_shared<curve>(line_shader, curve_points);
		object->set_color({0.0f, 0.0f, 0.0f, 1.0f});
		
		return object;
	}
}

