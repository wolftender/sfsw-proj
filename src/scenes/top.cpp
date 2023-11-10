#include "gui.hpp"
#include "scenes/top.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace mini {
	constexpr float SQRT3 = 1.73205080757f;
	constexpr float SQRT3INV = 1.0f / SQRT3;

	// simulation code starts here
	static inline glm::mat3x3 cube_inertia_tensor(const float diagonal, const float density) {
		// edge width
		const float a = SQRT3INV * diagonal;
		const float mass = density * a * a * a;
		
		return mass * glm::mat3x3{
			2.0f*a*a/3.0f, -a*a/4.0f, -a*a/4.0f,
			-a*a/4.0f, 2.0f*a*a/3.0f, -a*a/4.0f,
			-a*a/4.0f, -a*a/4.0f, 2.0f*a*a/3.0f
		};
	}

	top_scene::simulation_state_t::simulation_state_t(const simulation_parameters_t& parameters) :
		rotation(1.0f, 0.0f, 0.0f, 0.0f),
		parameters(parameters) {

		// compute inertia tensor based on parameters
		inertia_tensor = cube_inertia_tensor(parameters.diagonal_length, parameters.cube_density);
	}

	void top_scene::simulation_state_t::integrate(float delta_time) {
	}

	top_scene::top_scene(application_base& app) : scene_base(app),
		m_start_params(),
		m_state(m_start_params),
		m_viewport(app, "Spinning Top"),
		m_display_cube(true),
		m_display_plane(true),
		m_display_diagonal(true),
		m_display_grid(true),
		m_display_path(true),
		m_max_data_points(MAX_DATA_POINTS),
		m_num_data_points(0UL) {

		std::fill(m_path_points.begin(), m_path_points.end(), glm::vec3{0.0f, 0.0f, 0.0f});

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
	}

	void top_scene::render(app_context& context) {
		if (m_grid && m_display_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			context.draw(m_grid, grid_model);
		}

		if (m_cube && m_display_cube) {
			auto cube_model = glm::mat4x4(1.0f);

			const float edge_len = SQRT3INV * m_state.parameters.diagonal_length;
			cube_model = glm::scale(cube_model, {edge_len, edge_len, edge_len});

			context.draw(m_cube, cube_model);
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

	void top_scene::m_export_data() {
		// nothing to export
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
			ImGui::InputFloat("##top_step", &m_start_params.int_step);

			gui::prefix_label("Ang. Velocity: ", 250.0f);
			ImGui::InputFloat("##top_angvel", &m_start_params.angular_velocity);

			if (ImGui::Button("Apply Settings")) {
				m_state = simulation_state_t(m_start_params);
			}

			ImGui::NewLine();
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
}
