#include <format>
#include "beziermodel.hpp"

namespace mini {
	const glm::vec4& bezier_model_object::get_surface_color() const {
		return m_surface_color;
	}

	void bezier_model_object::set_surface_color(const glm::vec4& color) {
		m_surface_color = color;
	}

	void bezier_model_object::update_point(int index, const glm::vec3& position) {
		if (index > 0 && index < BEZIER_POINT_COUNT) {
			m_control_points[index] = position;
		}
	}

	bezier_model_object::bezier_model_object(
		std::shared_ptr<shader_program> shader,
		std::shared_ptr<triangle_mesh> mesh, 
		std::shared_ptr<texture> texture) : 
		m_surface_color{1.0f, 1.0f, 1.0f, 1.0f} {

		m_shader = shader;
		m_mesh = mesh;
		m_texture = texture;

		std::fill(m_control_points.begin(), m_control_points.end(), glm::vec3{ 0.0f, 0.0f, 0.0f });
	}

	bezier_model_object::~bezier_model_object() {}

	void bezier_model_object::render(app_context& context, const glm::mat4x4& world_matrix) const {
		m_shader->bind();

		// set uniforms
		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		m_shader->set_uniform("u_world", world_matrix);
		m_shader->set_uniform("u_view", view_matrix);
		m_shader->set_uniform("u_projection", proj_matrix);
		m_shader->set_uniform("u_surface_color", m_surface_color);
		m_shader->set_uniform("u_shininess", 0.0f);

		// this method works because theres only 64 points
		for (int index = 0; index < BEZIER_POINT_COUNT; ++index) {
			m_shader->set_uniform(std::format("u_control_points[{}]", index), m_control_points[index]);
		}

		if (m_texture) {
			m_texture->bind(GL_TEXTURE0);
			m_shader->set_uniform_int("u_enable_albedo", 1);
			m_shader->set_uniform_int("u_albedo_map", 0);
		}

		context.set_lights(*m_shader);
		m_mesh->draw();
	}
}
