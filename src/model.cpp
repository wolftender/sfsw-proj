#include "model.hpp"

namespace mini {
	const glm::vec4& model_object::get_surface_color() const {
		return m_surface_color;
	}

	void model_object::set_surface_color(const glm::vec4& color) {
		m_surface_color = color;
	}

	model_object::model_object(std::shared_ptr<triangle_mesh> mesh, std::shared_ptr<shader_program> shader) {
		m_mesh = mesh;
		m_shader = shader;
	}

	void model_object::render(app_context& context, const glm::mat4x4& world_matrix) const {
		m_shader->bind();

		// set uniforms
		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		m_shader->set_uniform("u_world", world_matrix);
		m_shader->set_uniform("u_view", view_matrix);
		m_shader->set_uniform("u_projection", proj_matrix);
		m_shader->set_uniform("u_surface_color", m_surface_color);
		m_shader->set_uniform("u_shininess", 0.0f);

		context.set_lights(*m_shader);
		m_mesh->draw();
	}
}