#pragma once
#include "context.hpp"
#include "shader.hpp"

namespace mini {
	class cube_object : public graphics_object {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_normal_buffer, m_uv_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_program> m_shader;

		public:
			cube_object(std::shared_ptr<shader_program> shader);
			~cube_object();

			cube_object(const cube_object&) = delete;
			cube_object& operator= (const cube_object&) = delete;

			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;
	};
}