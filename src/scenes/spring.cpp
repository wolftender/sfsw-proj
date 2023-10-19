#include <iostream>
#include <fstream>
#include <ios>
#include <chrono>

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "gui.hpp"
#include "mathparse.hpp"

#include "scenes/spring.hpp"

namespace mini {
	constexpr float FLOAT_MIN = std::numeric_limits<float>::min();

	spring_scene::spring_scene(application_base& app) : scene_base(app),
		m_time(0.0f),
		m_k0(0.7f),
		m_c0(10.0f),
		m_m0(1.0f),
		m_x0(3.0f), m_dx0(0.0f), m_ddx0(0.0f),
		m_x(0.0f), m_dx(0.0f), m_ddx(0.0f),
		m_last_vp_width(0),
		m_last_vp_height(0),
		m_distance(6.0f),
		m_spring_length(3.0f),
		m_h0(1.0f / 60.0f),
		m_paused(false),
		m_w_expression("0"),
		m_h_expression("sin(t)+cos(t)") {

		// initialize functions
		m_fw = mk_const(0.0f);
		m_fh = mk_const(0.0f);

		// setup context variables
		app.get_context().set_clear_color({0.95f, 0.95f, 0.95f});

		// initialize renderable objects
		auto line_shader = app.get_store().get_shader("line");
		if (line_shader) {
			m_spring_curve = m_make_helix_curve(line_shader);
			m_mass_object = m_make_wire_simplex(line_shader);
		}

		auto xy_grid_shader = app.get_store().get_shader("grid_xy");
		if (xy_grid_shader) {
			m_grid = std::make_shared<grid_object>(xy_grid_shader);
		}

		m_start_simulation();
	}

	std::shared_ptr<curve> spring_scene::m_make_wire_simplex(std::shared_ptr<shader_program> shader) const {
		std::vector<glm::vec3> points = {
			{ 0.5f, 0.5f, 0.5f }, 
			{ 0.5f, 0.5f, -0.5f },
			{ -0.5f, 0.5f, -0.5f },
			{ -0.5f, 0.5f, 0.5f },
			{ 0.5f, 0.5f, 0.5f },
			{ 0.0f, -0.5f, 0.0f },
			{ -0.5f, 0.5f, -0.5f },
			{ 0.5f, 0.5f, -0.5f },
			{ 0.0f, -0.5f, 0.0f },
			{ -0.5f, 0.5f, 0.5f }
		};

		auto object = std::make_shared<curve>(shader, points);
		object->set_color({ 0.0f, 0.3f, 0.4f, 1.0f });

		return object;
	}

	std::shared_ptr<curve> spring_scene::m_make_helix_curve(std::shared_ptr<shader_program> line_shader) const {
		std::vector<glm::vec3> helix_points;

		constexpr float pi = glm::pi<float>();
		const float radius = 0.5f;
		const float step = pi / 30.0f;
		const int num_steps = 500;

		const float vert_step = 1.0f / (step * num_steps);

		helix_points.reserve(num_steps + 1);

		for (int i = 0; i < num_steps; ++i) {
			float t = i * step;

			glm::vec3 point = {
				radius * glm::cos(t),
				t * vert_step,
				radius * glm::sin(t)
			};

			helix_points.push_back(point);
		}

		helix_points.push_back({0.0f, 1.0f, 0.0f});

		auto object = std::make_shared<curve>(line_shader, helix_points);
		object->set_color({0.0f, 0.0f, 0.0f, 1.0f});

		return object;
	}

	void spring_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.45f, nullptr, &dockspace_id);
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Spring", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
		ImGui::DockBuilderDockWindow("Trajectory", dock_id_right);
		ImGui::DockBuilderDockWindow("Simulation Graph", dock_id_bottom);
	}

	inline std::tm localtime_xp(std::time_t timer) {
		std::tm bt{};
#if defined(__unix__)
		localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
		localtime_s(&bt, &timer);
#else
		static std::mutex mtx;
		std::lock_guard<std::mutex> lock(mtx);
		bt = *std::localtime(&timer);
#endif
		return bt;
	}

	void spring_scene::m_export_data() {
		const auto now = std::time(nullptr);
		const std::tm tm = localtime_xp(now);

		const std::string file_name = std::format("spring-{}-{}-{}-{}-{}.txt", 
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

		std::ofstream stream(file_name);

		if (stream) {
			for (auto i = 0; i < m_num_data_points; ++i) {
				stream 
				<< m_t_data[i] << " "
				<< m_f_data[i] << " "
				<< m_g_data[i] << " "
				<< m_h_data[i] << " "
				<< m_x_data[i] << " "
				<< m_v_data[i] << std::endl;
			}
		}
	}

	void spring_scene::m_start_simulation() {
		// try parse expressions and return if fails
		try {
			math_lexer lexer(m_w_expression);
			math_parser parser(lexer.tokenize());

			m_fw = parser.parse();
			m_is_w_error = false;
		} catch (const std::exception& e) {
			m_w_error = e.what();
			m_is_w_error = true;

			return;
		}

		try {
			math_lexer lexer(m_h_expression);
			math_parser parser(lexer.tokenize());

			m_fh = parser.parse();
			m_is_h_error = false;
		} catch (const std::exception& e) {
			m_h_error = e.what();
			m_is_h_error = true;

			return;
		}

		m_t_data.clear();
		m_f_data.clear();
		m_g_data.clear();
		m_h_data.clear();
		m_x_data.clear();
		m_v_data.clear();

		m_t_data.resize(MAX_DATA_POINTS);
		m_f_data.resize(MAX_DATA_POINTS);
		m_g_data.resize(MAX_DATA_POINTS);
		m_h_data.resize(MAX_DATA_POINTS);
		m_x_data.resize(MAX_DATA_POINTS);
		m_v_data.resize(MAX_DATA_POINTS);

		m_num_data_points = 0;

		m_mass = m_m0;
		m_spring_coefficient = m_c0;
		m_friction_coefficient = m_k0;
		m_step = m_h0;

		const float m = m_mass;
		const float c = m_spring_coefficient;
		const float k = m_friction_coefficient;

		float w = m_fw->value(0.0f);
		float h = m_fh->value(0.0f);

		m_time = 0.0f;
		m_x = m_x0;
		m_dx = m_dx0;
		m_ddx = (c * (w - m_x) - k * m_dx + h) / m;

		m_push_data_point(m_time, c * (w - m_x0), -k * m_dx0, h, m_x, m_dx);
	}

	void spring_scene::integrate(float delta_time) {
		if (m_paused) {
			return;
		}

		// window was dragged probably
		if (delta_time > 0.1f) {
			delta_time = 0.1f;
		}

		float t0 = m_time;
		m_step_timer += delta_time;
		
		while (m_step_timer > m_step) {
			const float step = m_step;
			const float m = m_mass;
			const float c = m_spring_coefficient;
			const float k = m_friction_coefficient;

			float w = m_fw->value(t0);
			float h = m_fh->value(t0);

			const float dw = m_fw->derivative(t0);
			const float dh = m_fh->derivative(t0);

			float x0 = m_x;
			float dx0 = m_dx;
			float ddx0 = m_ddx;

			// euler method
			const float ddx1 = ddx0 + step * (c * (dw - dx0) - k * ddx0 + dh) / m;
			const float dx1 = dx0 + step * ddx0;
			const float x1 = x0 + step * dx0;

			// set new values
			m_ddx = ddx1;
			m_dx = dx1;
			m_x = x1;

			t0 = t0 + m_step;
			m_step_timer -= m_step;

			// only push one point per frame (otherwise the buffer is too small)
			if (m_step_timer < m_step) {
				m_push_data_point(m_time, c * (w - x0), -k * dx0, h, m_x, m_dx);
			}
		}

		// advance time step
		m_time += delta_time;
	}

	void spring_scene::render(app_context& context) {
		// setup scene
		m_distance = glm::clamp(m_distance, 1.0f, 15.0f);
		m_spring_length = glm::clamp(m_spring_length, 1.0f, 10.0f);

		auto& camera = get_app().get_context().get_camera();
		camera.set_position({0.0f, 0.0f, -m_distance});
		camera.set_target({0.0f, 0.0f, 0.0f});

		constexpr float pi = glm::pi<float>();

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			grid_model = glm::rotate(grid_model, pi * 0.5f, {1.0f, 0.0f, 0.0f});

			context.draw(m_grid, grid_model);
		}

		float w = m_fw->value(m_time);

		if (m_spring_curve) {
			float l = m_spring_length;

			auto spring_model = glm::mat4x4(1.0f);
			spring_model = glm::translate(spring_model, {0.0f, -w - l, 0.0f});
			spring_model = glm::scale(spring_model, {1.0f, l + m_x, 1.0f});

			context.draw(m_spring_curve, spring_model);
		}

		if (m_mass_object) {
			auto mass_model = glm::mat4x4(1.0f);
			mass_model = glm::translate(mass_model, {0.0f, -w + m_x + 0.5f, 0.0f});

			context.draw(m_mass_object, mass_model);
		}
	}

	void spring_scene::gui() {
		m_gui_graph();
		m_gui_settings();
		m_gui_viewport();
		m_gui_trajectory();
	}

	void spring_scene::menu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Export Data", "Ctrl + Shift + E", nullptr, true)) {
				m_export_data();
			}

			ImGui::EndMenu();
		}
	}

	void spring_scene::on_scroll(double offset_x, double offset_y) {
		m_distance -= offset_y * 0.2f;
		m_distance = glm::clamp(m_distance, 1.0f, 15.0f);
	}

	void spring_scene::m_gui_graph() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Graph", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		auto width = max.x - min.x;
		auto height = max.y - min.y;

		constexpr float min_range_x = 20.0f;
		constexpr float min_range_y = 5.0f;

		// render plots
		if (ImPlot::BeginPlot("f(t)", ImVec2(width * 0.325f, height - 15.0f), ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs)) {
			if (m_t_data[m_num_data_points - 1] - m_t_data[0] < min_range_x) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, m_t_data[0], m_t_data[0] + min_range_x, ImPlotCond_Always);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("f(t)", m_t_data.data(), m_f_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::SameLine();
		if (ImPlot::BeginPlot("g(t)", ImVec2(width * 0.325f, height - 15.0f), ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs)) {
			if (m_t_data[m_num_data_points - 1] - m_t_data[0] < min_range_x) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, m_t_data[0], m_t_data[0] + min_range_x, ImPlotCond_Always);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("g(t)", m_t_data.data(), m_g_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::SameLine();
		if (ImPlot::BeginPlot("h(t)", ImVec2(width * 0.325f, height - 15.0f), ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs)) {
			if (m_t_data[m_num_data_points - 1] - m_t_data[0] < min_range_x) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, m_t_data[0], m_t_data[0] + min_range_x, ImPlotCond_Always);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("h(t)", m_t_data.data(), m_h_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void spring_scene::m_gui_trajectory() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Trajectory", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		auto width = max.x - min.x;
		auto height = max.y - min.y;

		constexpr float min_range_x = 20.0f;
		constexpr float min_range_y = 20.0f;

		// render plots
		if (ImPlot::BeginPlot("x(v)", ImVec2(width, height), ImPlotFlags_NoBoxSelect)) {
			ImPlot::SetupAxis(ImAxis_X1, "position", ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxis(ImAxis_Y1, "velocity", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("x(v)", m_x_data.data(), m_v_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void spring_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::CollapsingHeader("Starting Values", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("x0 = ");
			ImGui::InputFloat("##spring_sim_x0", &m_x0);
			
			gui::prefix_label("dx0 = ");
			ImGui::InputFloat("##spring_sim_dx0", &m_dx0);

			gui::prefix_label("k = ");
			ImGui::InputFloat("##spring_sim_k0", &m_k0);

			gui::prefix_label("c = ");
			ImGui::InputFloat("##spring_sim_c0", &m_c0);

			gui::prefix_label("m = ");
			ImGui::InputFloat("##spring_sim_m0", &m_m0);

			gui::prefix_label("h = ");
			ImGui::InputFloat("##spring_sim_step", &m_h0);
			gui::clamp(m_h0, 0.001f, 0.1f);

			gui::prefix_label("w(t) = ");
			ImGui::InputText("##spring_w_func", &m_w_expression);

			if (m_is_w_error) {
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::Text(m_w_error.c_str());
				ImGui::PopStyleColor();
			}

			gui::prefix_label("h(t) = ");
			ImGui::InputText("##spring_h_func", &m_h_expression);

			if (m_is_h_error) {
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::Text(m_h_error.c_str());
				ImGui::PopStyleColor();
			}
		}

		if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label ("Pause Simulation: ", 250.0f);
			ImGui::Checkbox ("##spring_sim_paused", &m_paused);

			if (ImGui::Button("Reset Simulation")) {
				m_start_simulation();
			}

			ImGui::NewLine ();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void spring_scene::m_gui_viewport() {
		auto& context = get_app().get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2 (320, 240));
		ImGui::Begin("Spring", NULL);
		ImGui::SetWindowPos(ImVec2 (30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2 (640, 480), ImGuiCond_Once);

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

	void spring_scene::m_push_data_point(float t, float f, float g, float h, float x, float v) {
		if (m_num_data_points < MAX_DATA_POINTS) {
			m_t_data[m_num_data_points] = t;
			m_f_data[m_num_data_points] = f;
			m_g_data[m_num_data_points] = g;
			m_h_data[m_num_data_points] = h;
			m_x_data[m_num_data_points] = x;
			m_v_data[m_num_data_points] = v;

			m_num_data_points++;
		} else {

			for (std::size_t i = 0; i < m_num_data_points - 1; ++i) {
				m_t_data[i] = m_t_data[i + 1];
				m_f_data[i] = m_f_data[i + 1];
				m_g_data[i] = m_g_data[i + 1];
				m_h_data[i] = m_h_data[i + 1];
				m_x_data[i] = m_x_data[i + 1];
				m_v_data[i] = m_v_data[i + 1];
			}

			m_t_data[m_num_data_points - 1] = t;
			m_f_data[m_num_data_points - 1] = f;
			m_g_data[m_num_data_points - 1] = g;
			m_h_data[m_num_data_points - 1] = h;
			m_x_data[m_num_data_points - 1] = x;
			m_v_data[m_num_data_points - 1] = v;
		}
	}
}