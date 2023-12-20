#include <iostream>
#include <array>
#include <ranges>
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
		x(x), 
		dx{0.0f, 0.0f, 0.0f}, 
		ddx{0.0f, 0.0f, 0.0f}, 
		x0{ 0.0f, 0.0f, 0.0f }, 
		dx0{ 0.0f, 0.0f, 0.0f } {}

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
		force_sums.resize(num_masses);
		//x0.resize(num_masses);
		//v0.resize(num_masses);
		k1v.resize(num_masses);
		k2v.resize(num_masses);
		k3v.resize(num_masses);
		k4v.resize(num_masses);
		k1x.resize(num_masses);
		k2x.resize(num_masses);
		k3x.resize(num_masses);
		k4x.resize(num_masses);
	}

	void gel_scene::runge_kutta_solver_t::solve(simulation_state_t& state, float step) {
		// a(x,v,t) = 1/m * (force_sum - kv)

		auto num_masses = state.point_masses.size();
		const float dt = step;
		const float friction = state.settings.spring_friction;
		const float minv = state.mass_inv;

		state.calculate_force_sums(force_sums);
		for (size_t i = 0; i < num_masses; ++i) {
			auto& mass = state.point_masses[i];
			//x0[i] = mass.x;
			//v0[i] = mass.dx;

			k1v[i] = minv * (force_sums[i] - mass.dx * friction) * dt;
			k1x[i] = mass.dx * dt;

			// setup next step
			mass.x = mass.x + k1x[i] * 0.5f;
			mass.dx = mass.dx + k1v[i] * 0.5f;
		}

		state.calculate_force_sums(force_sums);
		for (size_t i = 0; i < num_masses; ++i) {
			auto& mass = state.point_masses[i];

			// a(x + k1x/2, v + k1v/2)
			k2v[i] = minv * (force_sums[i] - mass.dx * friction) * dt;
			k2x[i] = mass.dx * dt;

			mass.x = mass.x + k2x[i] * 0.5f;
			mass.dx = mass.dx + k2v[i] * 0.5f;
		}

		state.calculate_force_sums(force_sums);
		for (size_t i = 0; i < num_masses; ++i) {
			auto& mass = state.point_masses[i];

			// a(x + k1x/2, v + k1v/2)
			k3v[i] = minv * (force_sums[i] - mass.dx * friction) * dt;
			k3x[i] = mass.dx * dt;

			mass.x = mass.x + k3x[i];
			mass.dx = mass.dx + k3v[i];
		}

		state.calculate_force_sums(force_sums);
		for (size_t i = 0; i < num_masses; ++i) {
			auto& mass = state.point_masses[i];
			k4v[i] = minv * (force_sums[i] - mass.dx * friction) * dt;
			k4x[i] = mass.dx * dt;

			mass.dx = mass.dx0 + (k1v[i] + 2.0f * k2v[i] + 2.0f * k3v[i] + k4v[i]) / 6.0f;
			mass.x = mass.x0 + (k1x[i] + 2.0f * k2x[i] + 2.0f * k3x[i] + k4x[i]) / 6.0f;
		}
	}

	gel_scene::simulation_state_t::simulation_state_t(const simulation_settings_t& settings) {
		reset(settings);
	}

	constexpr float COLLISION_EPS = 1e-14;

	template<std::ranges::forward_range _BoundsRange>
	requires std::same_as<std::ranges::range_reference_t<_BoundsRange>, gel_scene::plane_t&>
	inline bool check_collision(
		const float bounce_factor,
		gel_scene::point_mass_t& mass, 
		const _BoundsRange& bounds) {

		auto start = mass.x0;
		auto end = mass.x;
		auto direction = end - start;
		auto length = glm::length(direction);

		if (length < COLLISION_EPS) {
			return false;
		}

		direction = direction / length;

		glm::vec3 _intersection = {0.0f, 0.0f, 0.0f};
		glm::vec3 _normal = {0.0f, 0.0f, 0.0f};
		float _dist = std::numeric_limits<float>::max();

		for (auto& bound : bounds) {
			float num = glm::dot(bound.point - start, bound.normal);
			float den = glm::dot(direction, bound.normal);

			// parallel to the plane
			if (glm::abs(den) < COLLISION_EPS || num <= COLLISION_EPS) {
				continue;
			}

			float d = (num / den);
			if (d > length || d > 0) {
				continue;
			}

			if (d < _dist) {
				_dist = d;
				_intersection = start + direction * d;
				_normal = bound.normal;
			}
		}

		if (_dist < length) {
			if (bounce_factor <= 0.0001f) {
				mass.x = _intersection;
				mass.dx = glm::vec3{ 0.0f, 0.0f, 0.0f };
			} else {
				// reflect position
				float total_dist = length;
				float travel_dist = glm::abs(_dist);
				float reflect_dist = total_dist - travel_dist;

				auto incident_move = direction;
				auto reflect_move = glm::normalize(glm::reflect(incident_move, _normal));

				mass.x0 = mass.x;
				mass.x = _intersection + reflect_move * reflect_dist * bounce_factor;

				// reflect velocity
				float total_speed = glm::length(mass.dx);
				float damped_speed = total_speed * bounce_factor;

				auto velocity_dir = mass.dx / total_speed;
				auto reflected_vel = glm::normalize(glm::reflect(velocity_dir, _normal));

				mass.dx0 = mass.dx;
				mass.dx = reflected_vel * damped_speed;
			}
			
			return true;
		}

		return false;
	}

	constexpr unsigned int MAX_COLLISION_ITER = 10;

	void gel_scene::simulation_state_t::integrate(float delta_time) {
		// window was dragged probably
		if (delta_time > 0.1f) {
			delta_time = 0.1f;
		}

		float t0 = time;
		step_timer += delta_time;

		while (step_timer > settings.integration_step) {
			const float step = settings.integration_step;

			for (auto& mass : point_masses) {
				mass.x0 = mass.x;
				mass.dx0 = mass.dx;
			}

			solver->solve(*this, step);

			// collision checking code
			for (auto& mass : point_masses) {
				unsigned int iter = 0;
				// recursive collision checking, but no more than MAX_COLLISION_ITER times to not
				// crash the simulation
				while (check_collision(settings.bounce_coefficient, mass, bounds) 
					&& iter++ <= MAX_COLLISION_ITER);
			}

			t0 = t0 + settings.integration_step;
			step_timer -= settings.integration_step;
		}

		time = t0;
	}

	void gel_scene::simulation_state_t::reset(const simulation_settings_t& settings) {
		this->settings = settings;

		// initialize bounding planes
		const float max_x = settings.bounds_width * 0.5f;
		const float max_z = settings.bounds_width * 0.5f;
		const float max_y = settings.bounds_height * 0.5f;

		bounds.clear();
		bounds.push_back({ { 0.0f, 0.0f, -max_z }, glm::normalize(glm::vec3{ 0.0f, 0.0f, +max_z }) });
		bounds.push_back({ { 0.0f, 0.0f, +max_z }, glm::normalize(glm::vec3{ 0.0f, 0.0f, -max_z }) });
		bounds.push_back({ { +max_x, 0.0f, 0.0f }, glm::normalize(glm::vec3{ -max_x, 0.0f, 0.0f }) });
		bounds.push_back({ { -max_x, 0.0f, 0.0f }, glm::normalize(glm::vec3{ +max_x, 0.0f, 0.0f }) });
		bounds.push_back({ { 0.0f, +max_y, 0.0f }, glm::normalize(glm::vec3{ 0.0f, -max_y, 0.0f }) });
		bounds.push_back({ { 0.0f, -max_y, 0.0f }, glm::normalize(glm::vec3{ 0.0f, +max_y, 0.0f }) });

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
			float magnitude = c * (dn - l) / (dn + 1e-5);

			auto f12 = magnitude * d12;
			auto f21 = magnitude * d21;

			force_sums[i] += f21;
			force_sums[j] += f12;
		}

		// calculate forces working on edges
		if (world.enable_frame) {
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
		}

		// calculate gravity forces
		if (world.enable_gravity) {
			for (auto& sum : force_sums) {
				sum += glm::vec3{ 0.0f, settings.mass * world.gravity, 0.0f };
			}
		}
	}

	gel_scene::gel_scene(application_base& app) : 
		scene_base(app),
		m_state(m_settings),
		m_viewport(app, "Soft Body"),
		m_frame_euler{0.0f, 0.0f, 0.0f},
		m_solver_method_id(0),
		m_show_springs(false),
		m_show_points(false), 
		m_show_bezier(true),
		m_show_deform(true),
		m_wireframe_mode(false) {

		auto line_shader = get_app().get_store().get_shader("line");
		auto room_shader = get_app().get_store().get_shader("room");
		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto point_shader = get_app().get_store().get_shader("point");
		auto gel_shader = get_app().get_store().get_shader("gelcube");

		auto slime_albedo = get_app().get_store().get_texture("slime_albedo");
		auto slime_normal = get_app().get_store().get_texture("slime_normal");

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

		if (gel_shader) {
			m_soft_object = std::make_shared<bezier_cube>(gel_shader);
			m_soft_object->set_albedo_map(slime_albedo);
			m_soft_object->set_normal_map(slime_normal);
		}

		if (room_shader) {
			m_bounds_object = std::make_shared<cube_object>(room_shader);
			m_bounds_object->set_cull_mode(cube_object::culling_mode_t::front);
		}

		auto camera = std::make_unique<default_camera>();
		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
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
			m_soft_object->update_point(index, m_state.point_masses[index].x);
		}
	}

	void gel_scene::render(app_context& context) {
		context.clear_lights();
		auto& light = context.get_light(0);

		light.color = { 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.position = { -5.0f, -5.0f, -5.0f };
		light.att_const = 1.0f;

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

		if (m_soft_object && m_show_bezier) {
			auto bezier_model = glm::mat4x4(1.0f);

m_soft_object->set_wireframe(m_wireframe_mode);
m_soft_object->refresh_buffer();

context.draw(m_soft_object, bezier_model);
		}

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}

		if (m_bounds_object) {
			float half_width = m_state.settings.bounds_width * 0.5f;
			float half_height = m_state.settings.bounds_height * 0.5f;

			auto bounds_model = glm::scale(glm::mat4(1.0f), glm::vec3{ half_width, half_height, half_width });
			context.draw(m_bounds_object, bounds_model);
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

				rotation = rotation * glm::angleAxis(m_frame_euler[2], glm::vec3{ 0.0f, 0.0f, 1.0f });
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

			gui::prefix_label("Show Wireframe: ", 250.0f);
			ImGui::Checkbox("##gel_show_wirefr", &m_wireframe_mode);
		}

		if (ImGui::CollapsingHeader("World Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Frame Enabled: ", 250.0f);
			ImGui::Checkbox("##gel_frame_on", &m_state.world.enable_frame);

			gui::prefix_label("Gravity Enabled: ", 250.0f);
			ImGui::Checkbox("##gel_gravity_on", &m_state.world.enable_gravity);

			gui::prefix_label("Gravity Force: ", 250.0f);
			ImGui::InputFloat("##gel_gravity_f", &m_state.world.gravity);
		}

		if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Bound Width: ", 250.0f);
			ImGui::InputFloat("##gel_bound_w", &m_settings.bounds_width);

			gui::prefix_label("Bound Height: ", 250.0f);
			ImGui::InputFloat("##gel_bound_h", &m_settings.bounds_height);

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

			gui::prefix_label("Bounce Coeff.: ", 250.0f);
			ImGui::InputFloat("##gel_bounce", &m_settings.bounce_coefficient);
			gui::clamp(m_settings.bounce_coefficient, 0.0f, 1.0f);

			gui::prefix_label("Frame Size: ", 250.0f);
			ImGui::InputFloat("##gel_frame_size", &m_settings.frame_length);

			gui::prefix_label("Int. Step: ", 250.0f);
			ImGui::InputFloat("##gel_int_step", &m_settings.integration_step);

			constexpr const char* solver_types[] = {"Euler Method", "Runge-Kutta Method"};
			gui::prefix_label("Solver Type:", 250.0f);

			if (ImGui::Combo("##gel_solver", &m_solver_method_id, solver_types, 2)) {
				solver_type_t new_mode = solver_type_t::euler;
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