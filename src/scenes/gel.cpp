#include <iostream>
#include <array>
#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "scenes/gel.hpp"

namespace mini {
	constexpr std::array<glm::vec3, 8> cube_points = {
		glm::vec3{ -1.0f, -1.0f,  1.0f },
		glm::vec3{ -1.0f, -1.0f, -1.0f },
		glm::vec3{ -1.0f,  1.0f,  1.0f },
		glm::vec3{ -1.0f,  1.0f, -1.0f },
		glm::vec3{  1.0f, -1.0f,  1.0f },
		glm::vec3{  1.0f, -1.0f, -1.0f },
		glm::vec3{  1.0f,  1.0f,  1.0f },
		glm::vec3{  1.0f,  1.0f, -1.0f }
	};

	gel_scene::point_mass_t::point_mass_t(const glm::vec3& x) :
		x(x), dx{0.0f, 0.0f, 0.0f}, ddx{0.0f, 0.0f, 0.0f} {}

	////// EULER METHOD //////

	gel_scene::euler_solver_t::euler_solver_t(std::size_t num_masses) {
		force_sums.resize(num_masses);
	}

	void gel_scene::euler_solver_t::solve(simulation_state_t& state, float step) {
		state.calculate_force_sums(force_sums);

		for (std::size_t mass_id = 0; mass_id < 64; ++mass_id) {
			auto& mass = state.point_masses[mass_id];
			auto& force_sum = force_sums[mass_id];

			mass.x = mass.x + step * mass.dx;
			mass.dx = mass.dx + step * mass.ddx;
			mass.ddx = state.mass_inv * (force_sum - mass.dx * state.settings.spring_friction);
		}
	}

	//////////////////////////
	//////// RK METHOD ///////

	gel_scene::runge_kutta_solver_t::runge_kutta_solver_t(std::size_t num_masses) {
		throw std::runtime_error("not implemented");
	}

	void gel_scene::runge_kutta_solver_t::solve(simulation_state_t& state, float step) {
		throw std::runtime_error("not implemented");
	}

	gel_scene::simulation_state_t::simulation_state_t(const simulation_settings_t& settings) {
		reset(settings);
	}

	void gel_scene::simulation_state_t::integrate(float delta_time) {
		// window was dragged probably
		if (delta_time > 0.1f) {
			delta_time = 0.1f;
		}

		float t0 = time;
		step_timer += delta_time;

		while (step_timer > settings.integration_step) {
			const float step = settings.integration_step;

			solver->solve(*this, step);

			t0 = t0 + settings.integration_step;
			step_timer -= settings.integration_step;
		}

		time = t0;
	}

	void gel_scene::simulation_state_t::reset(const simulation_settings_t& settings) {
		this->settings = settings;

		time = 0.0f;
		step_timer = 0.0f;

		mass_inv = 1.0f / settings.mass;
	
		frame_offset = { 0.0f, 0.0f, 0.0f };
		frame_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

		if (settings.solver_type == solver_type_t::euler) {
			solver = std::make_unique<euler_solver_t>(64);
		} else {
			solver = std::make_unique<runge_kutta_solver_t>(64);
		}

		point_masses.clear();
		point_masses.reserve(64);

		float dim = settings.spring_length * 3.0f;
		float step = settings.spring_length;
		float origin = dim * 0.5f;
		float half_frame = settings.frame_length * 0.5f;

		glm::vec3 corner = {-origin, -origin, -origin};
		glm::vec3 frame_corner = {-half_frame, -half_frame, -half_frame};

		frame_spring_len = glm::distance(corner, frame_corner);

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				for (int k = 0; k < 4; ++k) {
					float x = static_cast<float>(i) * step - origin;
					float y = static_cast<float>(j) * step - origin;
					float z = static_cast<float>(k) * step - origin;

					point_masses.push_back(point_mass_t(glm::vec3{ x,y,z }));
				}
			}
		}

		active_springs.clear();
		springs.clear();
		springs.resize(64*64);

		std::fill(springs.begin(), springs.end(), spring_t{ 0.0f, false });

		// setup springs
		for (int i = 0; i < 64; ++i) {
			int cx = i / 16;
			int cy = (i % 16) / 4;
			int cz = i % 4;

			for (int nx = cx - 1; nx <= cx + 1; ++nx) {
				for (int ny = cy - 1; ny <= cy + 1; ++ny) {
					for (int nz = cz - 1; nz <= cz + 1; ++nz) {
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < 4 && ny < 4 && nz < 4 &&
							(nx == cx || ny == cy || nz == cz)) {
							int j = (nx * 16) + (ny * 4) + nz;
							float dist = glm::distance(point_masses[i].x, point_masses[j].x);
							
							springs[i*64 + j].length = dist;
							springs[i*64 + j].enabled = true;
							springs[j*64 + i] = springs[i*64 + j];

							active_springs.push_back(i*64+j);
						}
					}
				}
			}
		}
	}

	void gel_scene::simulation_state_t::calculate_force_sums(std::vector<glm::vec3>& force_sums) const {
		for (auto& sum : force_sums) {
			sum = { 0.0f, 0.0f, 0.0f };
		}

		// calculate forces working on all the point masses
		for (const auto& spring_id : active_springs) {
			const auto i = spring_id % 64;
			const auto j = spring_id / 64;

			auto& m1 = point_masses[i];
			auto& m2 = point_masses[j];

			glm::vec3 d12 = m1.x - m2.x;
			glm::vec3 d21 = -d12;

			float l = springs[spring_id].length;
			float c = settings.spring_coefficient;

			float dn = glm::length(d12);
			float magnitude = c * (dn - l) / (dn + 0.0001f);

			auto f12 = magnitude * d12;
			auto f21 = magnitude * d21;

			force_sums[i] += f21;
			force_sums[j] += f12;
		}

		// calculate forces working on edges
		auto cube_model = glm::mat4x4(1.0f);
		float s = 0.5f * settings.frame_length;

		cube_model = glm::translate(cube_model, frame_offset);
		cube_model = cube_model * glm::mat4_cast(frame_rotation);
		cube_model = glm::scale(cube_model, { s, s, s });

		for (int x = 0; x <= 1; ++x) {
			for (int y = 0; y <= 1; ++y) {
				for (int z = 0; z <= 1; ++z) {
					glm::vec3 frame_point = cube_model * glm::vec4{
						-1.0f + x * 2.0f,
						-1.0f + y * 2.0f,
						-1.0f + z * 2.0f, 1.0f
					};

					int mx = x * 3;
					int my = y * 3;
					int mz = z * 3;
					int mass_id = (mx * 16) + (my * 4) + mz;

					auto& mass = point_masses[mass_id];
					float l = frame_spring_len;
					float c = settings.frame_coefficient;
					glm::vec3 d = frame_point - mass.x;
					float dn = glm::length(d);
					float magnitude = c * (dn - l) / (dn + 0.0001f);

					force_sums[mass_id] += magnitude * d;
				}
			}
		}

		// calculate gravity forces
		if (settings.enable_gravity) {
			for (auto& sum : force_sums) {
				sum += glm::vec3{ 0.0f, settings.mass * settings.gravity, 0.0f };
			}
		}
	}

	gel_scene::gel_scene(application_base& app) : 
		scene_base(app),
		m_state(m_settings),
		m_viewport(app, "Soft Body"),
		m_frame_euler{0.0f, 0.0f, 0.0f},
		m_solver_method_id(0),
		m_show_springs(true),
		m_show_points(true), 
		m_show_bezier(true),
		m_show_deform(true) {

		auto line_shader = get_app().get_store().get_shader("line");
		auto cube_shader = get_app().get_store().get_shader("cube");
		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto point_shader = get_app().get_store().get_shader("point");

		get_app().get_context().set_clear_color({ 0.75f, 0.75f, 0.9f });

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (line_shader) {
			m_springs_object = std::make_shared<segments_array>(line_shader, 64);
			m_springs_object->set_color({0.0f, 0.0f, 0.0f, 1.0f});
			m_reset_spring_array();

			m_build_cube_object(line_shader);
		}

		if (point_shader) {
			m_point_object = std::make_shared<billboard_object>(point_shader);
			m_point_object->set_color_tint({1.0f, 0.0f, 0.0f, 1.0f});
			m_point_object->set_size({16.0f, 16.0f});
		}
	}

	gel_scene::~gel_scene() { }

	void gel_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Soft Body", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
	}

	void gel_scene::integrate(float delta_time) {
		m_viewport.update(delta_time);
		m_state.integrate(delta_time);
		
		// update point positions on the gpu
		for (std::size_t index = 0; index < m_state.point_masses.size(); ++index) {
			m_springs_object->update_point(index, m_state.point_masses[index].x);
		}
	}

	void gel_scene::render(app_context& context) {
		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}

		if (m_springs_object && m_show_springs) {
			auto springs_model = glm::mat4x4(1.0f);

			m_springs_object->rebuild_buffers();
			context.draw(m_springs_object, springs_model);
		}

		if (m_point_object && m_show_points) {
			for (const auto& point : m_state.point_masses) {
				auto position = point.x;
				auto model_matrix = glm::translate(glm::mat4x4(1.0f), position);

				context.draw(m_point_object, model_matrix);
			}
		}

		if (m_cube_object) {
			auto cube_model = glm::mat4x4(1.0f);
			float s = 0.5f * m_state.settings.frame_length;

			cube_model = glm::translate(cube_model, m_state.frame_offset);
			cube_model = cube_model * glm::mat4_cast(m_state.frame_rotation);
			cube_model = glm::scale(cube_model, { s, s, s });

			context.draw(m_cube_object, cube_model);
		}
	}

	void gel_scene::gui() {
		m_gui_viewport();
		m_gui_settings();
	}

	void gel_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	void gel_scene::on_scroll(double offset_x, double offset_y) {
		if (m_viewport.is_viewport_focused() && m_viewport.is_mouse_in_viewport()) {
			m_viewport.set_distance(m_viewport.get_distance() - (static_cast<float> (offset_y) / 2.0f));
		}
	}

	void gel_scene::m_gui_viewport() {
		m_viewport.display();
	}

	void gel_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		m_viewport.configure();

		if (ImGui::CollapsingHeader("Frame Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Offset X: ", 250.0f);
			ImGui::SliderFloat("##gel_frame_x", &m_state.frame_offset.x, -15.0f, 15.0f);

			gui::prefix_label("Offset Y: ", 250.0f);
			ImGui::SliderFloat("##gel_frame_y", &m_state.frame_offset.y, -15.0f, 15.0f);

			gui::prefix_label("Offset Z: ", 250.0f);
			ImGui::SliderFloat("##gel_frame_z", &m_state.frame_offset.z, -15.0f, 15.0f);

			bool rotation_changed = false;
			constexpr auto half_pi = glm::pi<float>() * 0.5f;

			gui::prefix_label("Rotation X: ", 250.0f);
			rotation_changed = ImGui::SliderFloat("##gel_frame_rx", &m_frame_euler.x, -half_pi, half_pi) || rotation_changed;

			gui::prefix_label("Rotation Y: ", 250.0f);
			rotation_changed = ImGui::SliderFloat("##gel_frame_ry", &m_frame_euler.y, -half_pi, half_pi) || rotation_changed;

			gui::prefix_label("Rotation Z: ", 250.0f);
			rotation_changed = ImGui::SliderFloat("##gel_frame_rz", &m_frame_euler.z, -half_pi, half_pi) || rotation_changed;

			if (rotation_changed) {
				glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

				rotation = rotation * glm::angleAxis(m_frame_euler[2], glm::vec3{0.0f, 0.0f, 1.0f});
				rotation = rotation * glm::angleAxis(m_frame_euler[1], glm::vec3{ 0.0f, 1.0f, 0.0f });
				rotation = rotation * glm::angleAxis(m_frame_euler[0], glm::vec3{ 1.0f, 0.0f, 0.0f });

				m_state.frame_rotation = rotation;
			}
		}

		if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Show Masses: ", 250.0f);
			ImGui::Checkbox("##gel_show_points", &m_show_points);

			gui::prefix_label("Show Springs: ", 250.0f);
			ImGui::Checkbox("##gel_show_springs", &m_show_springs);

			gui::prefix_label("Show Gel Cube: ", 250.0f);
			ImGui::Checkbox("##gel_show_bezier", &m_show_bezier);

			gui::prefix_label("Show Model: ", 250.0f);
			ImGui::Checkbox("##gel_show_deform", &m_show_deform);
		}

		if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Gravity Enabled: ", 250.0f);
			ImGui::Checkbox("##gel_gravity_on", &m_settings.enable_gravity);

			gui::prefix_label("Gravity Force: ", 250.0f);
			ImGui::InputFloat("##gel_gravity_f", &m_settings.gravity);

			gui::prefix_label("Point Mass: ", 250.0f);
			ImGui::InputFloat("##gel_point_mass", &m_settings.mass);

			gui::prefix_label("Spring Length: ", 250.0f);
			ImGui::InputFloat("##gel_spring_len", &m_settings.spring_length);

			gui::prefix_label("Spring Friction: ", 250.0f);
			ImGui::InputFloat("##gel_spring_frict", &m_settings.spring_friction);

			gui::prefix_label("Spring Coefficient: ", 250.0f);
			ImGui::InputFloat("##gel_spring_coeff", &m_settings.spring_coefficient);

			gui::prefix_label("Frame Spring Coeff.: ", 250.0f);
			ImGui::InputFloat("##gel_spring_coeff_f", &m_settings.frame_coefficient);

			gui::prefix_label("Frame Size: ", 250.0f);
			ImGui::InputFloat("##gel_frame_size", &m_settings.frame_length);

			gui::prefix_label("Int. Step: ", 250.0f);
			ImGui::InputFloat("##gel_int_step", &m_settings.integration_step);

			constexpr const char* solver_types[] = {"Euler Method", "Runge-Kutta Method"};
			gui::prefix_label("Solver Type:", 250.0f);

			if (ImGui::Combo("##gel_solver", &m_solver_method_id, solver_types, 2)) {
				solver_type_t new_mode;
				switch (m_solver_method_id) {
					case 0: new_mode = solver_type_t::euler; break;
					case 1: new_mode = solver_type_t::runge_kutta; break;
				}

				m_settings.solver_type = new_mode;
			}

			if (ImGui::Button("Apply Settings")) {
				m_state.reset(m_settings);
				m_reset_spring_array();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void gel_scene::m_build_cube_object(std::shared_ptr<shader_program> line_shader) {
		m_cube_object = std::make_shared<segments_array>(line_shader, 8);

		for (std::size_t i = 0; i < cube_points.size(); ++i) {
			m_cube_object->update_point(i, cube_points[i]);
		}

		m_cube_object->add_segment(0, 1);
		m_cube_object->add_segment(1, 3);
		m_cube_object->add_segment(3, 2);
		m_cube_object->add_segment(2, 0);

		m_cube_object->add_segment(4, 5);
		m_cube_object->add_segment(5, 7);
		m_cube_object->add_segment(7, 6);
		m_cube_object->add_segment(6, 4);

		m_cube_object->add_segment(0, 4);
		m_cube_object->add_segment(1, 5);
		m_cube_object->add_segment(2, 6);
		m_cube_object->add_segment(3, 7);

		m_cube_object->rebuild_buffers();
	}

	void gel_scene::m_reset_spring_array() {
		m_springs_object->clear_segments();
		std::vector<segments_array::segment_t> segments;

		for (std::size_t i = 0; i < 64; ++i) {
			for (std::size_t j = 0; j < i; ++j) {
				auto index = i*64 + j;
				if (m_state.springs[index].enabled) {
					segments.push_back({i,j});
				}
			}
		}

		m_springs_object->add_segments(segments);
	}
}