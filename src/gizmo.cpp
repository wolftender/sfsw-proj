#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gizmo.hpp"

namespace mini {
	void gizmo::make_gizmo_verts(std::vector<float>& positions, std::vector<GLuint>& indices, 
		int res, float r1, float r2, float r3, float h) {

		// arrow consists of 1 vertex on top and 3 rings of vertices and one on the bottom
		int num_vertices = res * 3 + 2;

		positions.resize(num_vertices * 3);
		positions[0] = 0.0f;
		positions[1] = 1.0f;
		positions[2] = 0.0f;

		int v = 1;
		for (int i = 0; i < res; ++i, ++v) {
			int b = 3 * v;
			float t = glm::pi<float>() * 2.0f / static_cast<float> (res) * static_cast<float>(i);

			positions[b + 0] = r1 * glm::cos(t);
			positions[b + 1] = h;
			positions[b + 2] = r1 * glm::sin(t);
		}

		for (int i = 0; i < res; ++i, ++v) {
			int b = 3 * v;
			float t = glm::pi<float>() * 2.0f / static_cast<float> (res) * static_cast<float>(i);

			positions[b + 0] = r2 * glm::cos(t);
			positions[b + 1] = h;
			positions[b + 2] = r2 * glm::sin(t);
		}

		for (int i = 0; i < res; ++i, ++v) {
			int b = 3 * v;
			float t = glm::pi<float>() * 2.0f / static_cast<float> (res) * static_cast<float>(i);

			positions[b + 0] = r3 * glm::cos(t);
			positions[b + 1] = -1.0f;
			positions[b + 2] = r3 * glm::sin(t);
		}

		int b = 3 * v;
		positions[b + 0] = 0.0f;
		positions[b + 1] = -1.0f;
		positions[b + 2] = 0.0f;

		// construct indices
		int b1 = 1;
		int b2 = 1 + res;
		int b3 = 1 + (res * 2);
		int f = num_vertices - 1;

		indices.reserve(res * 6);
		for (int i = 0; i < res; ++i) {
			indices.push_back(b1 + ((i + 1) % res));
			indices.push_back(b1 + i);
			indices.push_back(0);
		}

		for (int i = 0; i < res; ++i) {
			indices.push_back(b2 + i);
			indices.push_back(b1 + i);
			indices.push_back(b1 + ((i + 1) % res));

			indices.push_back(b2 + ((i + 1) % res));
			indices.push_back(b2 + i);
			indices.push_back(b1 + ((i + 1) % res));
		}

		for (int i = 0; i < res; ++i) {
			indices.push_back(b3 + i);
			indices.push_back(b2 + i);
			indices.push_back(b2 + ((i + 1) % res));

			indices.push_back(b3 + ((i + 1) % res));
			indices.push_back(b3 + i);
			indices.push_back(b2 + ((i + 1) % res));
		}

		for (int i = 0; i < res; ++i) {
			indices.push_back(b3 + ((i + 1) % res));
			indices.push_back(f);
			indices.push_back(b3 + i);
		}
	}

	gizmo::gizmo(std::shared_ptr<shader_program> shader_mesh) {
		m_shader_mesh = shader_mesh;
		m_gen_arrow_mesh(m_mesh_arrow);
	}

	gizmo::~gizmo() {
		m_free_mesh(m_mesh_arrow);
	}

	void gizmo::render(app_context& context, const glm::mat4x4& world_matrix) const {
		glBindVertexArray(m_mesh_arrow.vao);
		m_shader_mesh->bind();

		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		auto local = glm::mat4x4(1.0f);

		glm::vec3 center = world_matrix * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		float dist = glm::distance(context.get_camera().get_position(), center);
		float world_size = 2.0f * glm::tan(glm::pi<float>() / 3.0f) * dist * 0.018f;

		auto offset = glm::vec3{ 0.0f, 1.0f, 0.0f };

		glm::mat4x4 up = glm::translate(glm::rotate(local, glm::pi<float>(), glm::vec3{ 1.0f, 0.0f, 0.0f }), offset);
		glm::mat4x4 forward = glm::translate(glm::rotate(local, -glm::pi<float>() * 0.5f, glm::vec3{ 0.0f, 0.0f, 1.0f }), offset);
		glm::mat4x4 left = glm::translate(glm::rotate(local, -glm::pi<float>() * 0.5f, glm::vec3{ 1.0f, 0.0f, 0.0f }), offset);

		m_shader_mesh->set_uniform("u_view", view_matrix);
		m_shader_mesh->set_uniform("u_projection", proj_matrix);

		m_shader_mesh->set_uniform("u_world", world_matrix * up);
		m_shader_mesh->set_uniform("u_color", glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
		glDrawElements(GL_TRIANGLES, m_mesh_arrow.indices.size(), GL_UNSIGNED_INT, NULL);

		m_shader_mesh->set_uniform("u_world", world_matrix * forward);
		m_shader_mesh->set_uniform("u_color", glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
		glDrawElements(GL_TRIANGLES, m_mesh_arrow.indices.size(), GL_UNSIGNED_INT, NULL);

		m_shader_mesh->set_uniform("u_world", world_matrix * left);
		m_shader_mesh->set_uniform("u_color", glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f });
		glDrawElements(GL_TRIANGLES, m_mesh_arrow.indices.size(), GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);
	}

	inline float line_distance(const glm::vec3& a1, const glm::vec3& b1, const glm::vec3& a2, const glm::vec3& b2) {
		glm::vec3 c = glm::cross(b1, b2);
		float num = glm::dot(c, a2 - a1);
		float den = glm::length(c);

		return glm::abs(num / den);
	}

	inline float line_distance(const glm::vec3& a, const glm::vec3& n, const glm::vec3& p) {
		return glm::length(glm::cross(p - a, n)) / glm::length(n);
	}

	void gizmo::m_gen_arrow_mesh(gizmo_mesh_t& mesh) {
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		std::vector<float> positions;
		std::vector<GLuint> indices;

		const int res = 20;
		const float r1 = 0.15f;
		const float r2 = 0.05f;
		const float r3 = 0.05f;
		const float h = 0.5f;

		make_gizmo_verts(positions, indices, res, r1, r2, r3, h);

		glGenVertexArrays(1, &mesh.vao);
		glGenBuffers(1, &mesh.pos_buffer);
		glGenBuffers(1, &mesh.index_buffer);

		glBindVertexArray(mesh.vao);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.pos_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_position, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(a_position);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);

		mesh.indices = indices;
		mesh.positions = positions;
	}

	void gizmo::m_free_mesh(gizmo_mesh_t& mesh) {
		if (mesh.vao) {
			glDeleteVertexArrays(1, &mesh.vao);
			mesh.vao = 0;
		}

		if (mesh.pos_buffer) {
			glDeleteBuffers(1, &mesh.pos_buffer);
			mesh.pos_buffer = 0;
		}

		if (mesh.index_buffer) {
			glDeleteBuffers(1, &mesh.index_buffer);
			mesh.index_buffer = 0;
		}
	}
}