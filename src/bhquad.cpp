#include "bhquad.hpp"

namespace mini {
	constexpr std::array<float, 6 * 3> QUAD_POSITIONS = {
		1.0f, 1.0f, 0.5f,
		-1.0f, 1.0f, 0.5f,
		-1.0f, -1.0f, 0.5f,

		1.0f, 1.0f, 0.5f,
		-1.0f, -1.0f, 0.5f,
		1.0f, -1.0f, 0.5f
	};

	constexpr unsigned int NUM_QUAD_VERTS = 6;

	black_hole_quad::black_hole_quad(
		std::shared_ptr<shader_program> program, 
		std::shared_ptr<cubemap> cubemap) : 
		m_program(program), 
		m_cubemap(cubemap),
		m_distance(1000.0f),
		m_star_mass(10.0f) {

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		glGenBuffers(1, &m_pos_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * NUM_QUAD_VERTS * 3, QUAD_POSITIONS.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}

	black_hole_quad::~black_hole_quad() {
		if (m_vao) {
			glDeleteVertexArrays(1, &m_vao);
		}

		if (m_pos_buffer) {
			glDeleteBuffers(1, &m_pos_buffer);
		}
	}

    float black_hole_quad::get_star_mass() const {
        return m_star_mass;
    }

    float black_hole_quad::get_distance() const {
        return m_distance;
    }

    void black_hole_quad::set_star_mass(float star_mass) {
		m_star_mass = star_mass;
    }

    void black_hole_quad::set_distance(float distance) {
		m_distance = distance;
    }

    void black_hole_quad::set_cube_map(std::shared_ptr<cubemap> cubemap) {
		m_cubemap = cubemap;
    }

    void black_hole_quad::render(app_context& context, const glm::mat4x4& world_matrix) const {
		glBindVertexArray(m_vao);

		m_program->bind();

		// set uniforms
		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		const auto& video_mode = context.get_video_mode();

		glm::vec2 resolution = {
			static_cast<float>(video_mode.get_buffer_width()),
			static_cast<float>(video_mode.get_buffer_height())
		};

		m_program->set_uniform("u_world", world_matrix);
		m_program->set_uniform("u_view", view_matrix);
		m_program->set_uniform("u_projection", proj_matrix);
		m_program->set_uniform("u_resolution", resolution);
		m_program->set_uniform("u_distance", m_distance);
		m_program->set_uniform("u_star_mass", m_star_mass);

		m_cubemap->bind();

		glDrawArrays(GL_TRIANGLES, 0, NUM_QUAD_VERTS);
		glBindVertexArray(0);
	}
}
