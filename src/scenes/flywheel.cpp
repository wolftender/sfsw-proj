#include "gui.hpp"
#include "camera.hpp"
#include "scenes/flywheel.hpp"

namespace mini {
	flywheel_scene::time_series_t::time_series_t(std::size_t samples) : num_samples(samples), index(0UL) {
		time.resize(num_samples);
		data.resize(num_samples);
	}

	void flywheel_scene::time_series_t::store(float t, float x) {
		if (index < num_samples) {
			time[index] = t;
			data[index] = x;
			index++;
		} else {
			for (auto i = 0UL; i < num_samples - 1; ++i) {
				time[i] = time[i + 1];
				data[i] = data[i + 1];
			}

			time[num_samples - 1] = t;
			data[num_samples - 1] = x;
		}
	}

	void flywheel_scene::time_series_t::clear() {
		index = 0UL;
	}

	void flywheel_scene::simulation_state_t::integrate(float delta_time) {
		time = time + delta_time * flywheel_speed;
		time_total = time_total + delta_time;

		constexpr float pi = glm::pi<float>();
		constexpr float dpi = 2.0f * pi;
		if (time > dpi) {
			time -= dpi;
		}
		
		const float R = wheel_radius;
		origin_pos = glm::vec2 {
			R * glm::cos(time) - R,
			R * glm::sin(time)
		};

		float ysq = origin_pos.y * origin_pos.y;
		float dsq = stick_length * stick_length;
		float xsq = dsq - ysq;

		mass_pos = glm::vec2 {
			origin_pos.x + glm::sqrt(xsq),
			0.0f
		};
	}

	flywheel_scene::flywheel_scene(application_base & app) : 
		scene_base(app),
		m_pos_series(NUM_DATA_POINTS),
		m_speed_series(NUM_DATA_POINTS),
		m_accel_series(NUM_DATA_POINTS),
		m_hodograph(NUM_DATA_POINTS),
		m_last_vp_width(0),
		m_last_vp_height(0),
		m_mouse_in_viewport(false),
		m_viewport_focus(false),
		m_vp_mouse_offset{0, 0},
		m_rd{},
		m_generator(m_rd()),
		m_distr{0.0f, m_state.eps} {
		
		app.get_context().set_clear_color({ 0.95f, 0.95f, 0.95f });

		auto xy_grid_shader = app.get_store().get_shader("grid_xy");
		auto line_shader = get_app().get_store().get_shader("line");
		
		if (xy_grid_shader) {
			m_grid = std::make_shared<grid_object>(xy_grid_shader);
		}
		
		if (line_shader) {
			m_wheel_curve = m_make_wheel_curve(line_shader);
			m_square_curve = m_make_square_curve(line_shader);
					
			m_stick_curve = std::make_shared<segments_array>(line_shader, 2);
			m_stick_curve->add_segment(0, 1);
			m_stick_curve->set_line_width(3.0f);
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
		auto dock_id_bottom_right = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.8f, nullptr, &dock_id_right);

		ImGui::DockBuilderDockWindow("Flywheel", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
		ImGui::DockBuilderDockWindow("Simulation Data", dock_id_bottom_right);
		ImGui::DockBuilderDockWindow("Trajectory", dock_id_bottom_right);
	}
	
	void flywheel_scene::integrate(float delta_time) {
		if (delta_time > 0.1f) {
			delta_time = 0.1f;
		}

		m_state.integrate(delta_time);
		
		// update curves based on current state
		m_stick_curve->update_point(0, glm::vec3(m_state.origin_pos, 0.0f));
		m_stick_curve->update_point(1, glm::vec3(m_state.mass_pos, 0.0f));
		m_stick_curve->rebuild_buffers();

		// store data from this frame
		if (m_state.error) {
			m_pos_series.store(m_state.time_total, m_state.mass_pos.x + m_distr(m_generator));
		} else {
			m_pos_series.store(m_state.time_total, m_state.mass_pos.x);
		}

		if (m_pos_series.index > 1) {
			auto curr = m_pos_series.index - 1;
			auto prev = m_pos_series.index - 2;

			float dt = m_pos_series.time[curr] - m_pos_series.time[prev];
			float dx = m_pos_series.data[curr] - m_pos_series.data[prev];

			m_speed_series.store(m_pos_series.time[prev], dx / dt);
			m_hodograph.store(m_speed_series.data[m_speed_series.index - 1], m_pos_series.data[curr]);
		}

		if (m_pos_series.index > 2) {
			auto next = m_pos_series.index - 1;
			auto curr = m_pos_series.index - 2;
			auto prev = m_pos_series.index - 3;

			float t1 = m_pos_series.time[prev];
			float t2 = m_pos_series.time[curr];
			float t3 = m_pos_series.time[next];

			float y1 = m_pos_series.data[prev];
			float y2 = m_pos_series.data[curr];
			float y3 = m_pos_series.data[next];

			float d = 0.0f;
			d += 2.0f * y1 / ((t2 - t1) * (t3 - t1));
			d -= 2.0f * y2 / ((t3 - t2) * (t2 - t1));
			d += 2.0f * y3 / ((t3 - t2) * (t3 - t1));

			m_accel_series.store(t2, d);
		}
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
			glm::vec3 mt = glm::vec3(m_state.mass_pos, 0.0f);
			mt.x += 0.875f;

			mass_model = glm::translate(mass_model, mt);
			mass_model = glm::scale(mass_model, glm::vec3{1.75f, 1.0f, 1.0f});
			
			context.draw(m_wheel_curve, wheel_model);
			context.draw(m_square_curve, mass_model);
			context.draw(m_stick_curve, glm::mat4x4(1.0f));
		}
	}
	
	void flywheel_scene::gui() {
		m_gui_settings();
		m_gui_viewport();
		m_gui_graphs();
		m_gui_curve_graph();
	}
	
	void flywheel_scene::on_scroll(double offset_x, double offset_y) {
		
	}
	
	void flywheel_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}
	
	void flywheel_scene::m_gui_curve_graph() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Trajectory", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		auto width = max.x - min.x;
		auto height = max.y - min.y;

		//constexpr float min_range_x = 20.0f;
		//constexpr float min_range_y = 20.0f;

		// render plots
		if (ImPlot::BeginPlot("x(v)", ImVec2(width, height), ImPlotFlags_NoBoxSelect)) {
			ImPlot::SetupAxis(ImAxis_X1, "position", ImPlotAxisFlags_None);\
			ImPlot::SetupAxis(ImAxis_Y1, "velocity", ImPlotAxisFlags_None);

			ImPlot::SetupAxisLimits(ImAxis_X1, -10.0f, 10.0f, ImPlotCond_Once);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -10.0f, 10.0f, ImPlotCond_Once);

			ImPlot::PlotLine("x(v)", m_hodograph.time.data(), m_hodograph.data.data(), m_hodograph.index);
			ImPlot::EndPlot();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
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

		gui::prefix_label("Speed: ", 250.0f);
		ImGui::DragFloat("##fwh_speed", &m_state.flywheel_speed, 0.1f, 0.1f, 10.0f);

		gui::prefix_label("Epsilon: ", 250.0f);

		auto L = m_state.stick_length;
		if (ImGui::DragFloat("##fwh_epsilon", &m_state.eps, 0.00001f * L, 0.00001f * L, 0.01f * L)) {
			m_distr = std::normal_distribution{ 0.0f, m_state.eps };
		}

		gui::prefix_label("Error: ", 250.0f);
		ImGui::Checkbox("##fwh_error", &m_state.error);

		if (ImGui::Button("Reset")) {
			m_pos_series.clear();
			m_speed_series.clear();
			m_accel_series.clear();
			m_hodograph.clear();

			m_state.time = 0.0f;
			m_state.time_total = 0.0f;
		}
		
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

	void flywheel_scene::m_gui_graphs() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Data", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		auto width = max.x - min.x;
		auto height = max.y - min.y;

		constexpr float min_range_x = 20.0f;
		auto size = ImVec2(-1.0f, 200.0f);

		m_plot_series(m_pos_series, "x(t)", size, min_range_x);
		m_plot_series(m_speed_series, "x'(t)", size, min_range_x);
		m_plot_series(m_accel_series, "x''(t)", size, min_range_x);

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void flywheel_scene::m_plot_series(const time_series_t& series, const std::string & name, const ImVec2& size, 
		const float min_range_x) {

		if (ImPlot::BeginPlot(name.c_str(), size, ImPlotFlags_None)) {
			if (series.index > 0 && series.time[series.index - 1] - series.time[0] < min_range_x) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, series.time[0], series.time[0] + min_range_x, ImPlotCond_Always);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine(name.c_str(), series.time.data(), series.data.data(), static_cast<int>(series.index));
			ImPlot::EndPlot();
		}
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
		object->set_line_width(3.0f);
		
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
		object->set_line_width(3.0f);
		
		return object;
	}
}

