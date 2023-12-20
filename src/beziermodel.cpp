#include "beziermodel.hpp"

namespace mini {
	const glm::vec4& bezier_model_object::get_surface_color() const {
		return m_surface_color;
	}

	void bezier_model_object::set_surface_color(const glm::vec4& color) {
		m_surface_color = color;
	}

	bezier_model_object::bezier_model_object(
		std::shared_ptr<shader_program> shader,
		std::shared_ptr<triangle_mesh> mesh, 
		std::shared_ptr<texture> texture) : 
		m_surface_color{1.0f, 1.0f, 1.0f, 1.0f} {
		m_shader = shader;
		m_mesh = mesh;
		m_texture = texture;
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

		if (m_texture) {
			m_texture->bind(GL_TEXTURE0);
			m_shader->set_uniform_int("u_enable_albedo", 1);
			m_shader->set_uniform_int("u_albedo_map", 0);
		}

		context.set_lights(*m_shader);
		m_mesh->draw();
	}
}