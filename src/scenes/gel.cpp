#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "scenes/gel.hpp"

namespace mini {
	gel_scene::point_mass_t::point_mass_t(const glm::vec3& x) :
		x(x), dx{0.0f, 0.0f, 0.0f}, ddx{0.0f, 0.0f, 0.0f} {}

	gel_scene::simulation_state_t::simulation_state_t(const simulation_settings_t& settings) {
		reset(settings);
	}

	void gel_scene::simulation_state_t::integrate(float delta_time) {

	}

	void gel_scene::simulation_state_t::reset(const simulation_settings_t& settings) {
		this->settings = settings;

		point_masses.clear();
		point_masses.reserve(64);

		float dim = settings.spring_length * 3.0f;
		float step = settings.spring_length;
		float origin = dim * 0.5f;

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
						}
					}
				}
			}
		}
	}

	gel_scene::gel_scene(application_base& app) : 
		scene_base(app),
		m_state(m_settings),
		m_viewport(app, "Soft Body") {

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
		
		for (std::size_t index = 0; index < m_state.point_masses.size(); ++index) {
			m_springs_object->update_point(index, m_state.point_masses[index].x);
		}
	}

	void gel_scene::render(app_context& context) {
		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}

		if (m_springs_object) {
			auto springs_model = glm::mat4x4(1.0f);

			m_springs_object->rebuild_buffers();
			context.draw(m_springs_object, springs_model);
		}

		if (m_point_object) {
			for (const auto& point : m_state.point_masses) {
				auto position = point.x;
				auto model_matrix = glm::translate(glm::mat4x4(1.0f), position);

				context.draw(m_point_object, model_matrix);
			}
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

		ImGui::End();
		ImGui::PopStyleVar(1);
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