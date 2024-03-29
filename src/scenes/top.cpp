#include "gui.hpp"
#include "scenes/top.hpp"

#include <iostream>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>

namespace mini {
	constexpr float SQRT3 = 1.73205080757f;
	constexpr float SQRT3INV = 1.0f / SQRT3;
	constexpr float PI = glm::pi<float>();

	// simulation code starts here
	static inline void cube_inertia_tensor(
		const float diagonal, 
		const float density, 
		float& mass,
		glm::mat3x3& tensor, 
		glm::mat3x3& inverse) {
		// edge width
		const float a = SQRT3INV * diagonal;
		
		mass = density * a * a * a;
		
		tensor = mass * glm::mat3x3{
			2.0f*a*a/3.0f, -a*a/4.0f, -a*a/4.0f,
			-a*a/4.0f, 2.0f*a*a/3.0f, -a*a/4.0f,
			-a*a/4.0f, -a*a/4.0f, 2.0f*a*a/3.0f
		};

		inverse = (6.0f / (11.0f * a * a * mass)) * glm::mat3x3{
			5.0f, 3.0f, 3.0f,
			3.0f, 5.0f, 3.0f,
			3.0f, 3.0f, 5.0f
		};
	}

	top_scene::simulation_state_t::simulation_state_t(
		const simulation_parameters_t& parameters, 
		const world_parameters_t* world_params) :
		world_params(world_params),
		parameters(parameters),
		W(1.0f, 1.0f, 1.0f),
		Q(1.0f, 0.0f, 0.0f, 0.0f),
		time(0.0f), 
		step_timer(0.0f) {

		// initial angular speed
		W = W * parameters.angular_velocity;

		// initial rotation calculation
		float angle = glm::atan(1.0f / glm::sqrt(2.0f));
		float angle_x = angle - PI * 0.5f + parameters.cube_deviation;

		auto start_rotation = Q;

		start_rotation = start_rotation * glm::angleAxis(0.25f * PI, glm::vec3{ 0.0f, -1.0f, 0.0f });
		start_rotation = glm::angleAxis(angle_x, glm::vec3{ 1.0f, 0.0f, 0.0f }) * start_rotation;

		Q = start_rotation;

		// compute inertia tensor based on parameters
		cube_inertia_tensor(
			parameters.diagonal_length, 
			parameters.cube_density, 
			mass, 
			inertia_tensor, 
			inertia_tensor_inv);
	}

	void top_scene::simulation_state_t::integrate(float delta_time) {
		const auto& I = inertia_tensor;
		const auto& Iinv = inertia_tensor_inv;

		constexpr auto diag_local = glm::vec3{1.0f, 1.0f, 1.0f} * SQRT3INV;

		// window was dragged probably
		if (delta_time > 0.1f) {
			delta_time = 0.1f;
		}

		float t0 = time;
		step_timer += delta_time;

		// dW/dt
		const auto f = [&](const float t, const glm::vec3& N, const glm::vec3& W) -> glm::vec3 {
			return Iinv * (N + glm::cross((I * W), W));
		};

		// dQ/dt
		const auto g = [&](const float t, const glm::quat& Q, const glm::vec3& W) -> glm::quat {
			return 0.5f * Q * glm::quat(0.0f, W.x, W.y, W.z);
		};

		while (step_timer > parameters.int_step) {
			const float h = parameters.int_step;

			glm::vec3 N {0.0f, 0.0f, 0.0f};

			if (world_params->gravity_enabled) {
				const glm::vec3& world_up = { 0.0f, -1.0f, 0.0f };
				const glm::vec3& to_center = 0.5f * diag_local * parameters.diagonal_length;
				const glm::vec3& local_up = glm::rotate(glm::conjugate(Q), world_up);
				N = mass * world_params->gravity * glm::cross(-local_up, to_center);
			}

			// first equation IWt = N + (IW)xW
			// denoted Wt = f(t,W)
			{
				auto k1w = f(t0, N, W);
				auto k2w = f(t0 + 0.5f * h, N, W + 0.5f * h * k1w);
				auto k3w = f(t0 + 0.5f * h, N, W + 0.5f * h * k2w);
				auto k4w = f(t0 + h, N, W + h * k3w);

				W = W + h * (k1w + 2.0f * k2w + 2.0f * k3w + k4w) / 6.0f;
			}

			// second equation Qt = Q*W/2
			// denoted Qt = g(t,Q)
			{
				auto k1q = g(t0, Q, W);
				auto k2q = g(t0 + 0.5f * h, Q + 0.5f * h * k1q, W);
				auto k3q = g(t0 + 0.5f * h, Q + 0.5f * h * k2q, W);
				auto k4q = g(t0 + h, Q + h * k3q, W);

				Q = Q + h * (k1q + 2.0f * k2q + 2.0f * k3q + k4q) / 6.0f;
				Q = glm::normalize(Q);
			}

			t0 = t0 + parameters.int_step;
			step_timer -= parameters.int_step;
		}

		time += delta_time;
	}

	top_scene::top_scene(application_base& app) : scene_base(app),
		m_display_cube(true),
		m_display_diagonal(true),
		m_display_grid(true),
		m_display_plane(true),
		m_display_path(true),
		m_world_params(),
		m_start_params(),
		m_state(m_start_params, &m_world_params),
		m_max_data_points(MAX_DATA_POINTS),
		m_viewport(app, "Spinning Top") {

		m_clear_data_points();

		auto line_shader = get_app().get_store().get_shader("line");
		auto cube_shader = get_app().get_store().get_shader("cube");
		auto grid_shader = get_app().get_store().get_shader("grid_xz");

		get_app().get_context().set_clear_color({0.75f, 0.75f, 0.9f});

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (cube_shader) {
			m_cube = std::make_shared<cube_object>(cube_shader);
		}

		if (line_shader) {
			m_diagonal = std::make_shared<curve>(line_shader);
			m_diagonal->append_position({0.0f, 0.0f, 0.0f});
			m_diagonal->append_position({-1.0f, -1.0f, -1.0f});
			m_diagonal->set_color({1.0f, 0.0f, 1.0f, 1.0f});

			m_curve = std::make_shared<curve>(line_shader);
			m_curve->set_color({ 0.890f, 0.657f, 0.0178f, 1.0f });
		}

		auto camera = std::make_unique<default_camera>();
		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
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

		// integrate the simulation state
		m_state.integrate(delta_time);

		// push data point to the curve
		constexpr glm::vec3 diag_vector = -0.5f * glm::vec3{ 1.0f, 1.0f, 1.0f } * SQRT3INV;
		auto diag_position = glm::rotate(m_state.Q, 2.0f * m_state.parameters.diagonal_length * diag_vector);

		m_push_data_point(m_state.time, diag_position);
	}

	void top_scene::render(app_context& context) {
		context.clear_lights();
		auto& light = context.get_light(0);

		light.color = {1.0f, 1.0f, 1.0f};
		light.intensity = 3.0f;
		light.position = {0.0f, -5.0f, 0.0f};
		light.att_const = 1.0f;

		if (m_grid && m_display_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}

		const float edge_len = SQRT3INV * m_state.parameters.diagonal_length;

		if (m_diagonal && m_display_diagonal) {
			auto diagonal_model = glm::mat4x4(1.0f);

			diagonal_model = glm::scale(diagonal_model, glm::vec3{ edge_len, edge_len, edge_len });
			diagonal_model = glm::mat4_cast(m_state.Q) * diagonal_model;

			context.draw(m_diagonal, diagonal_model);
		}

		if (m_cube && m_display_cube) {
			auto cube_model = glm::mat4x4(1.0f);

			cube_model = glm::scale(cube_model, 0.5f * glm::vec3{ edge_len, edge_len, edge_len });
			cube_model = glm::translate(cube_model, { -1.0f, -1.0f, -1.0f });
			cube_model = glm::mat4_cast(m_state.Q) * cube_model;

			context.draw(m_cube, cube_model);
		}

		if (m_curve && m_display_path) {
			context.draw(m_curve, glm::mat4x4(1.0f));
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

	void top_scene::m_export_data() {
		const auto now = std::time(nullptr);
		const std::tm tm = localtime_xp(now);

		const std::string file_name = std::format("top-{}-{}-{}-{}-{}.txt",
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

		std::ofstream stream(file_name);

		if (stream) {
			for (std::size_t i = 0; i < m_path_points.size(); ++i) {
				stream
					<< m_time_points[i] << " "
					<< m_path_points[i].x << " "
					<< m_path_points[i].y << " "
					<< m_path_points[i].z << std::endl;
			}
		}
	}

	void top_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		m_viewport.configure();

		// render controls
		if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Diagonal Len.: ", 250.0f);
			ImGui::InputFloat("##top_diagonal", &m_start_params.diagonal_length);

			gui::prefix_label("Deviation: ", 250.0f);
			ImGui::InputFloat("##top_deviation", &m_start_params.cube_deviation);

			gui::prefix_label("Density: ", 250.0f);
			ImGui::InputFloat("##top_density", &m_start_params.cube_density);

			gui::prefix_label("Int. Step: ", 250.0f);
			ImGui::InputFloat("##top_step", &m_start_params.int_step, 0.0001f, 0.001f, "%.5f");

			gui::prefix_label("Ang. Velocity: ", 250.0f);
			ImGui::InputFloat("##top_angvel", &m_start_params.angular_velocity);

			if (ImGui::Button("Apply Settings")) {
				m_reset_simulation();
			}

			ImGui::NewLine();
		}

		if (ImGui::CollapsingHeader("World Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Gravity Enabled: ", 250.0f);
			ImGui::Checkbox("##top_gravity_on", &m_world_params.gravity_enabled);

			gui::prefix_label("Gravity Force: ", 250.0f);
			ImGui::InputFloat("##top_gravity_f", &m_world_params.gravity);
		}

		if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Show Cube: ", 250.0f);
			ImGui::Checkbox("##top_show_cube", &m_display_cube);

			gui::prefix_label("Show Grid: ", 250.0f);
			ImGui::Checkbox("##top_show_grid", &m_display_grid);

			gui::prefix_label("Show Path: ", 250.0f);
			ImGui::Checkbox("##top_show_path", &m_display_path);

			gui::prefix_label("Show Diagonal: ", 250.0f);
			ImGui::Checkbox("##top_show_diag", &m_display_diagonal);
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void top_scene::m_gui_viewport() {
		m_viewport.display();
	}

	void top_scene::m_reset_simulation() {
		m_state = simulation_state_t(m_start_params, &m_world_params);
		m_clear_data_points();
	}

	void top_scene::m_clear_data_points() {
		m_path_points.clear();
		m_time_points.clear();
		m_path_points.reserve(m_max_data_points);
		m_time_points.reserve(m_max_data_points);

		if (m_curve) {
			m_curve->clear_positions();
		}
	}

	void top_scene::m_push_data_point(const float time, const glm::vec3& point) {
		auto num_points = m_path_points.size();

		if (num_points < m_max_data_points) {
			m_path_points.push_back(point);
			m_time_points.push_back(time);
		} else {
			for (std::size_t i = 0; i < num_points - 1; ++i) {
				m_path_points[i] = m_path_points[i + 1];
				m_time_points[i] = m_time_points[i + 1];
			}

			m_path_points[num_points - 1] = point;
			m_time_points[num_points - 1] = time;
		}

		if (m_curve) {
			m_curve->reset_positions(m_path_points);
		}
	}
}
