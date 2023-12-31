#include "segments.hpp"

namespace mini {
	segments_array::segments_array(
		std::shared_ptr<shader_program> line_shader, 
		std::size_t num_points) {
		m_line_shader = line_shader;

		m_points.resize(num_points);

		m_vao = 0;
		m_position_buffer = 0;
		m_index_buffer = 0;
		m_ready = false;
		m_ignore_depth = false;
		m_line_width = 2.0f;

		m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	segments_array::~segments_array() {
		m_free_buffers();
	}

	bool segments_array::get_ignore_depth() const {
		return m_ignore_depth;
	}

	void segments_array::set_ignore_depth(bool ignore) {
		m_ignore_depth = ignore;
	}

	float segments_array::get_line_width() const {
		return m_line_width;
	}

	const glm::vec4& segments_array::get_color() const {
		return m_color;
	}

	void segments_array::set_line_width(float width) {
		m_line_width = width;
	}

	void segments_array::set_color(const glm::vec4& color) {
		m_color = color;
	}

	void segments_array::add_segment(std::size_t begin, std::size_t end) {
		if (begin < m_points.size() && end < m_points.size()) {
			m_indices.push_back(begin);
			m_indices.push_back(end);
		}
	}

	void segments_array::add_segments(const std::vector<segment_t>& segments) {
		for (const auto& segment : segments) {
			if (segment.first < m_points.size() && segment.second < m_points.size()) {
				m_indices.push_back(segment.first);
				m_indices.push_back(segment.second);
			}
		}
	}

	void segments_array::clear_segments() {
		m_indices.clear();
	}

	void segments_array::update_point(std::size_t index, const glm::vec3& data) {
		m_points[index] = data;
	}

	void segments_array::update_points(const std::vector<glm::vec3>& points) {
		for (std::size_t i = 0; i < m_points.size() && i < points.size(); ++i) {
			m_points[i] = points[i];
		}
	}

	void segments_array::rebuild_buffers() {
		m_rebuild_buffers();
	}

	void segments_array::render(app_context& context, const glm::mat4x4& world_matrix) const {
		if (!m_ready) {
			return;
		}

		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		const auto& video_mode = context.get_video_mode();

		glm::vec2 resolution = {
			static_cast<float> (video_mode.get_buffer_width()),
			static_cast<float> (video_mode.get_buffer_height())
		};

		glBindVertexArray(m_vao);

		m_line_shader->bind();
		m_line_shader->set_uniform("u_world", world_matrix);
		m_line_shader->set_uniform("u_view", view_matrix);
		m_line_shader->set_uniform("u_projection", proj_matrix);
		m_line_shader->set_uniform("u_resolution", resolution);
		m_line_shader->set_uniform("u_line_width", m_line_width);
		m_line_shader->set_uniform("u_color", m_color);

		if (m_ignore_depth) {
			glDisable(GL_DEPTH_TEST);
		}

		glDrawElements(GL_LINES, m_indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
		if (m_ignore_depth) {
			glEnable(GL_DEPTH_TEST);
		}
	}

	void segments_array::m_rebuild_buffers() {
		if (m_ready) {
			m_free_buffers();
		}

		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		if (m_ready) {
			m_free_buffers();
		}

		m_ready = false;

		if (m_points.size() == 0) {
			return;
		}

		m_positions.clear();
		m_positions.resize(m_points.size() * 3);

		for (int i = 0; i < m_points.size(); ++i) {
			m_positions[3 * i + 0] = m_points[i].x;
			m_positions[3 * i + 1] = m_points[i].y;
			m_positions[3 * i + 2] = m_points[i].z;
		}

		// allocate gpu buffers
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_position_buffer);
		glGenBuffers(1, &m_index_buffer);

		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m_positions.size(), reinterpret_cast<void*> (m_positions.data()), GL_STATIC_DRAW);
		glVertexAttribPointer(a_position, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(a_position);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
		m_ready = true;
	}

	void segments_array::m_free_buffers() {
		if (m_vao) {
			glDeleteVertexArrays(1, &m_vao);
			m_vao = 0;
		}

		if (m_position_buffer) {
			glDeleteBuffers(1, &m_position_buffer);
			m_position_buffer = 0;
		}

		if (m_index_buffer) {
			glDeleteBuffers(1, &m_index_buffer);
			m_index_buffer = 0;
		}
	}
}