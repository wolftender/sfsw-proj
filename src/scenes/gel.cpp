#include "gui.hpp"

#include "scenes/gel.hpp"

namespace mini {
	gel_scene::point_mass_t::point_mass_t(const simulation_settings_t& settings, const glm::vec3& x) :
		settings(settings), x(x), dx{0.0f, 0.0f, 0.0f}, ddx{0.0f, 0.0f, 0.0f} {}

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

					point_masses.push_back(std::make_unique<point_mass_t>(settings, glm::vec3{x,y,z}));
				}
			}
		}
	}

	gel_scene::gel_scene(application_base& app) : 
		scene_base(app),
		m_viewport(app, "Soft Body") {

		auto line_shader = get_app().get_store().get_shader("line");
		auto cube_shader = get_app().get_store().get_shader("cube");
		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto point_shader = get_app().get_store().get_shader("point");

		get_app().get_context().set_clear_color({ 0.75f, 0.75f, 0.9f });

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
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
	}

	void gel_scene::render(app_context& context) {
		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}

		if (m_point_object) {
			auto point_model = glm::mat4x4(1.0f);
			context.draw(m_point_object, point_model);
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
}