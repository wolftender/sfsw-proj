#pragma once
#include "context.hpp"

namespace mini {
	static constexpr int num_bezier_cube_points = 64;
	static constexpr int num_bezier_cube_indices = 96;

	class bezier_cube : public graphics_object {
		private:
			GLuint m_pos_buffer, m_uv_buffer, m_index_buffer, m_vao;

			std::array<float, num_bezier_cube_points * 3> m_positions;
			std::array<float, num_bezier_cube_points * 2> m_uv;
			std::array<uint32_t, num_bezier_cube_indices> m_indices;

			uint32_t m_u_res;
			uint32_t m_v_res;

			glm::vec4 m_color;
			std::shared_ptr<shader_program> m_shader;

			bool m_wireframe_mode;

		public:
			uint32_t get_u_res() const;
			uint32_t get_v_res() const;
			
			void set_u_res(uint32_t u_res);
			void set_v_res(uint32_t v_res);

			bool get_wireframe() const;
			void set_wireframe(bool enable);

			bezier_cube(std::shared_ptr<shader_program> shader);
			~bezier_cube();

			bezier_cube(const bezier_cube&) = delete;
			bezier_cube& operator= (const bezier_cube&) = delete;

			void update_cube(const std::vector<glm::vec3> positions);
			void update_point(std::size_t x, std::size_t y, std::size_t z, const glm::vec3& position);
			void update_point(std::size_t index, const glm::vec3& position);
			void refresh_buffer();

			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;

		private:
			void m_bind_shader(app_context& context, shader_program& shader, const glm::mat4x4& world_matrix) const;
	};
}