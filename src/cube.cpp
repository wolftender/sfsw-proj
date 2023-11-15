#include "cube.hpp"

namespace mini {
	constexpr std::array<float, 72> cube_positions = {
		 1,  1, -1,
		-1,  1, -1,
		-1,  1,  1,
		 1,  1,  1,
		 1, -1,  1,
		 1,  1,  1,
		-1,  1,  1,
		-1, -1,  1,
		-1, -1,  1,
		-1,  1,  1,
		-1,  1, -1,
		-1, -1, -1,
		-1, -1, -1,
		 1, -1, -1,
		 1, -1,  1,
		-1, -1,  1,
		 1, -1, -1,
		 1,  1, -1,
		 1,  1,  1,
		 1, -1,  1,
		-1, -1, -1,
		-1,  1, -1,
		 1,  1, -1,
		 1, -1, -1
	};

	constexpr std::array<float, 96> cube_colors = {
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1
	};

	constexpr std::array<float, 72> cube_normals = {
		 0,  1,  0,
		 0,  1,  0,
		 0,  1,  0,
		 0,  1,  0,
		 0,  0,  1,
		 0,  0,  1,
		 0,  0,  1,
		 0,  0,  1,
		-1,  0,  0,
		-1,  0,  0,
		-1,  0,  0,
		-1,  0,  0,
		 0, -1,  0,
		 0, -1,  0,
		 0, -1,  0,
		 0, -1,  0,
		 1,  0,  0,
		 1,  0,  0,
		 1,  0,  0,
		 1,  0,  0,
		 0,  0, -1,
		 0,  0, -1,
		 0,  0, -1,
		 0,  0, -1
	};

	constexpr std::array<float, 48> cube_uv = {
		0, 0,
		1, 0,
		1, 1,
		0, 1,
		0, 0,
		1, 0,
		1, 1,
		0, 1,
		0, 0,
		1, 0,
		1, 1,
		0, 1,
		0, 0,
		1, 0,
		1, 1,
		0, 1,
		0, 0,
		1, 0,
		1, 1,
		0, 1,
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	constexpr std::array<uint32_t, 36> cube_indices = {
		0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23
	};

	cube_object::cube_object(std::shared_ptr<shader_program> shader) {

		constexpr int num_vertices = 24;


		constexpr GLuint a_position = 0;
		constexpr GLuint a_normal = 1;
		constexpr GLuint a_uv = 2;
		constexpr GLuint a_color = 3;

		m_shader = shader;
		m_pos_buffer = m_normal_buffer = m_uv_buffer = m_color_buffer = m_index_buffer = m_vao = 0;

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_pos_buffer);
		glGenBuffers(1, &m_color_buffer);
		glGenBuffers(1, &m_index_buffer);
		glGenBuffers(1, &m_normal_buffer);
		glGenBuffers(1, &m_uv_buffer);
		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 3, cube_positions.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_position, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(a_position);

		glBindBuffer(GL_ARRAY_BUFFER, m_normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 3, cube_normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_normal, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(a_normal);

		glBindBuffer(GL_ARRAY_BUFFER, m_uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 2, cube_uv.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_uv, 2, GL_FLOAT, false, sizeof(float) * 2, (void*)0);
		glEnableVertexAttribArray(a_uv);

		glBindBuffer(GL_ARRAY_BUFFER, m_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 4, cube_colors.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_color, 4, GL_FLOAT, false, sizeof(float) * 4, (void*)0);
		glEnableVertexAttribArray(a_color);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * cube_indices.size(), cube_indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

	cube_object::~cube_object() {
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_pos_buffer);
		glDeleteBuffers(1, &m_color_buffer);
		glDeleteBuffers(1, &m_index_buffer);
		glDeleteBuffers(1, &m_normal_buffer);
		glDeleteBuffers(1, &m_uv_buffer);
	}

	void cube_object::render(app_context& context, const glm::mat4x4& world_matrix) const {
		glEnable(GL_CULL_FACE);
		glBindVertexArray(m_vao);

		m_shader->bind();

		// set uniforms
		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		m_shader->set_uniform("u_world", world_matrix);
		m_shader->set_uniform("u_view", view_matrix);
		m_shader->set_uniform("u_projection", proj_matrix);
		m_shader->set_uniform("u_surface_color", glm::vec4{1.0f, 1.0f, 0.0f, 0.65f});

		context.set_lights(*m_shader);

		glDrawElements(GL_TRIANGLES, cube_indices.size(), GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
		glDisable(GL_CULL_FACE);
	}
}