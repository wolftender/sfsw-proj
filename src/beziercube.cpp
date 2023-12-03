#include "beziercube.hpp"

namespace mini {
	template<std::size_t _NumElements>
	inline void build_indices(std::array<uint32_t, _NumElements>& indices) {
		const auto build_surface = [&](const std::function<void(int, int)>& f) {
			for (int x = 0; x < 4; ++x) {
				for (int y = 0; y < 4; ++y) {
					f(x, y);
				}
			}
		};

		const auto get_index = [&](int x, int y, int z) -> int {
			return (x * 16) + (y * 4) + z;
		};

		std::size_t index = 0;
		build_surface([&](int x, int y) {indices[index++] = get_index(x, 3-y, 0); });
		build_surface([&](int x, int y) {indices[index++] = get_index(x, y, 3); });
		build_surface([&](int x, int y) {indices[index++] = get_index(x, 0, y); });
		build_surface([&](int x, int y) {indices[index++] = get_index(x, 3, 3-y); });
		build_surface([&](int x, int y) {indices[index++] = get_index(0, x, 3-y); });
		build_surface([&](int x, int y) {indices[index++] = get_index(3, x, y); });
	}
	
	uint32_t bezier_cube::get_u_res() const {
		return m_u_res;
	}

	uint32_t bezier_cube::get_v_res() const {
		return m_v_res;
	}

	void bezier_cube::set_u_res(uint32_t u_res) {
		m_u_res = u_res;
	}

	void bezier_cube::set_v_res(uint32_t v_res) {
		m_v_res = v_res;
	}

	bool bezier_cube::get_wireframe() const {
		return m_wireframe_mode;
	}

	void bezier_cube::set_wireframe(bool enable) {
		m_wireframe_mode = enable;
	}

	void bezier_cube::set_albedo_map(std::shared_ptr<texture> albedo_map) {
		m_albedo_map = albedo_map;
	}

	void bezier_cube::set_normal_map(std::shared_ptr<texture> normal_map) {
		m_normal_map = normal_map;
	}

	std::shared_ptr<texture> bezier_cube::get_albedo_map() const {
		return m_albedo_map;
	}

	std::shared_ptr<texture> bezier_cube::get_normal_map() const {
		return m_normal_map;
	}

	bezier_cube::bezier_cube(std::shared_ptr<shader_program> shader) :
		m_u_res(24), m_v_res(24), m_color{1.0f, 0.0f, 0.0f, 1.0f}, m_wireframe_mode(true) {
		m_shader = shader;
		m_pos_buffer = m_uv_buffer = m_index_buffer = m_vao = 0;

		build_indices(m_indices);

		constexpr GLuint a_position = 0;
		constexpr GLuint a_normal = 1;
		constexpr GLuint a_uv = 2;
		constexpr GLuint a_color = 3;

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_pos_buffer);
		glGenBuffers(1, &m_index_buffer);
		glGenBuffers(1, &m_uv_buffer);
		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_bezier_cube_points * 3, m_positions.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(a_position, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(a_position);

		glBindBuffer(GL_ARRAY_BUFFER, m_uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_bezier_cube_points * 2, m_uv.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_uv, 2, GL_FLOAT, false, sizeof(float) * 2, (void*)0);
		glEnableVertexAttribArray(a_uv);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

	bezier_cube::~bezier_cube() {
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_pos_buffer);
		glDeleteBuffers(1, &m_index_buffer);
		glDeleteBuffers(1, &m_uv_buffer);
	}

	void bezier_cube::update_cube(const std::vector<glm::vec3> positions) {
		for (std::size_t i = 0; i < num_bezier_cube_points && i < positions.size(); ++i) {
			std::size_t base = 3 * i;
			
			m_positions[base + 0] = positions[i].x;
			m_positions[base + 1] = positions[i].y;
			m_positions[base + 2] = positions[i].z;
		}
	}

	void bezier_cube::update_point(std::size_t x, std::size_t y, std::size_t z, const glm::vec3& position) {
		std::size_t index = (x * 16) + (y * 4) + z;
		std::size_t base = 3 * index;

		m_positions[base + 0] = position.x;
		m_positions[base + 1] = position.y;
		m_positions[base + 2] = position.z;
	}

	void bezier_cube::update_point(std::size_t index, const glm::vec3& position) {
		std::size_t base = 3 * index;

		m_positions[base + 0] = position.x;
		m_positions[base + 1] = position.y;
		m_positions[base + 2] = position.z;
	}

	void bezier_cube::refresh_buffer() {
		glBindBuffer(GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_positions.size() * sizeof(float), m_positions.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void bezier_cube::render(app_context& context, const glm::mat4x4& world_matrix) const {
		glBindVertexArray(m_vao);

		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		if (m_wireframe_mode) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		m_bind_shader(context, *m_shader.get(), world_matrix);

		// first render pass - u,v
		m_shader->set_uniform_uint("u_resolution_v", static_cast<GLuint>(m_v_res));
		m_shader->set_uniform_uint("u_resolution_u", static_cast<GLuint>(m_u_res));
		m_shader->set_uniform_int("u_wireframe", static_cast<GLint>(m_wireframe_mode));
		m_shader->set_uniform_int("u_texture", 0);

		glPatchParameteri(GL_PATCH_VERTICES, 16);
		glDrawElements(GL_PATCHES, m_indices.size(), GL_UNSIGNED_INT, 0);

		if (m_wireframe_mode) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glBindVertexArray(0);
	}

	void bezier_cube::m_bind_shader(app_context& context, shader_program& shader, const glm::mat4x4& world_matrix) const {
		shader.bind();

		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		const auto& video_mode = context.get_video_mode();

		glm::vec2 resolution = {
			static_cast<float> (video_mode.get_buffer_width()),
			static_cast<float> (video_mode.get_buffer_height())
		};

		shader.set_uniform("u_world", world_matrix);
		shader.set_uniform("u_view", view_matrix);
		shader.set_uniform("u_projection", proj_matrix);
		shader.set_uniform("u_resolution", resolution);
		shader.set_uniform("u_line_width", 2.0f);
		shader.set_uniform("u_color", m_color);
		shader.set_uniform("u_shininess", 64.0f);

		if (m_albedo_map) {
			m_albedo_map->bind(GL_TEXTURE0);
			shader.set_uniform_int("u_enable_albedo", 1);
			shader.set_uniform_int("u_albedo_map", 0);
		} else {
			shader.set_uniform_int("u_enable_albedo", 0);
		}

		if (m_normal_map) {
			m_normal_map->bind(GL_TEXTURE1);
			shader.set_uniform_int("u_enable_normal", 1);
			shader.set_uniform_int("u_normal_map", 1);
		} else {
			shader.set_uniform_int("u_enable_normal", 0);
		}

		context.set_lights(shader);
	}
}