#include "gui.hpp"
#include "scenes/puma.hpp"

namespace mini {
	puma_scene::puma_scene(application_base& app) : 
		scene_base(app), 
		m_context1(app.get_context()),
		m_context2(video_mode_t(600, 400)), 
		m_viewport1(app, m_context1, "Config Interp."),
		m_viewport2(app, m_context2, "Effector Interp.") {

		m_context1.set_clear_color({ 0.75f, 0.75f, 0.9f });
		m_context2.set_clear_color({ 0.75f, 0.75f, 0.9f });

		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto puma_shader = get_app().get_store().get_shader("puma");

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (puma_shader) {
			m_effector_mesh = m_make_effector_mesh();
			m_arm_mesh = triangle_mesh::make_cylinder(0.4f, 5.0f, 20, 20);
			m_joint_mesh = triangle_mesh::make_cylinder(0.6f, 2.0f, 20, 20);

			m_effector_model_x = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_effector_model_y = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_effector_model_z = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_arm_model = std::make_shared<model_object>(m_arm_mesh, puma_shader);
			m_joint_model = std::make_shared<model_object>(m_joint_mesh, puma_shader);

			m_effector_model_x->set_surface_color({ 1.0f, 0.0f, 0.0f, 1.0f });
			m_effector_model_y->set_surface_color({ 0.0f, 1.0f, 0.0f, 1.0f });
			m_effector_model_z->set_surface_color({ 0.0f, 0.0f, 1.0f, 1.0f });
			m_arm_model->set_surface_color({ 0.0f, 1.0f, 0.0f, 1.0f });
			m_joint_model->set_surface_color({0.0f, 0.0f, 1.0f, 1.0f});
		}

		auto camera = std::make_unique<default_camera>();
		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
	}

	void puma_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id);
		auto dock_id_top_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Config Interp.", dockspace_id);
		ImGui::DockBuilderDockWindow("Effector Interp.", dock_id_top_left);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_bottom);
	}

	void puma_scene::integrate(float delta_time) {
		m_viewport1.update(delta_time);
		m_viewport2.update(delta_time);
	}

	inline void setup_light(app_context& context) {
		context.clear_lights();
		auto& light = context.get_light(0);

		light.color = { 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.position = { -5.0f, -5.0f, -5.0f };
		light.att_const = 1.0f;
	}

	void puma_scene::m_draw_puma(app_context& context, const puma_config_t& config) const {
		context.draw(m_arm_model, glm::mat4x4(1.0f));
		context.draw(m_joint_model, glm::mat4x4(1.0f));
		context.draw(m_effector_model_x, glm::mat4x4(1.0f));
	}

	void puma_scene::render(app_context& context) {
		setup_light(m_context1);
		setup_light(m_context2);

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			m_context1.draw(m_grid, grid_model);
			m_context2.draw(m_grid, grid_model);
		}

		if (m_arm_mesh) {
			m_draw_puma(m_context1, m_config1);
			m_draw_puma(m_context2, m_config2);
		}

		m_context2.display(false, true);
	}

	void puma_scene::gui() {
		m_gui_settings();
		m_viewport1.display();
		m_viewport2.display();
	}

	void puma_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	std::shared_ptr<triangle_mesh> puma_scene::m_make_effector_mesh() {
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> uvs;

		std::vector<GLuint> indices;

		const int res = 20;
		const float r1 = 0.25f;
		const float r2 = 0.18f;
		const float r3 = 0.18f;
		const float h = 0.5f;

		gizmo::make_gizmo_verts(positions, indices, res, r1, r2, r3, h);

		int num_vertices = positions.size() / 3;
		normals.resize(positions.size());
		uvs.resize(num_vertices * 2);

		std::fill(uvs.begin(), uvs.end(), 0.0f);

		// automatically calculate normals
		std::vector<glm::vec3> sums;
		sums.resize(num_vertices);
		std::fill(sums.begin(), sums.end(), glm::vec3{0.0f, 0.0f, 0.0f});

		int num_faces = indices.size() / 3;

		for (int face = 0; face < num_faces; ++face) {
			int base = face * 3;
			int i1 = indices[base + 0];
			int i2 = indices[base + 1];
			int i3 = indices[base + 2];

			int b1 = 3 * i1;
			int b2 = 3 * i2;
			int b3 = 3 * i3;

			auto p1 = glm::vec3{ positions[b1 + 0], positions[b1 + 1], positions[b1 + 2] };
			auto p2 = glm::vec3{ positions[b2 + 0], positions[b2 + 1], positions[b2 + 2] };
			auto p3 = glm::vec3{ positions[b3 + 0], positions[b3 + 1], positions[b3 + 2] };

			auto n1 = glm::cross(p2 - p1, p3 - p1);
			auto n2 = glm::cross(p1 - p3, p3 - p1);
			auto n3 = glm::cross(p1 - p3, p2 - p3);

			sums[i1] += n1;
			sums[i2] += n2;
			sums[i3] += n3;
		}

		for (int i = 0; i < num_vertices; ++i) {
			sums[i] = glm::normalize(sums[i]);
			normals[3 * i + 0] = sums[i].x;
			normals[3 * i + 1] = sums[i].y;
			normals[3 * i + 2] = sums[i].z;
		}

		return std::make_shared<triangle_mesh>(positions, normals, uvs, indices);
	}

	void puma_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto width = max.x - min.x;

		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}
