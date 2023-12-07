#pragma once
#include <glad/glad.h>

#include "context.hpp"

namespace mini {
	class plane_object : public graphics_object {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_index_buffer, m_uv_buffer, m_vao;

			glm::vec2 m_size;
			glm::vec4 m_color_tint;

			std::shared_ptr<shader_program> m_shader;

		public:
			const glm::vec2& get_size() const;
			const glm::vec4& get_color_tint() const;

			void set_size(const glm::vec2& size);
			void set_color_tint(const glm::vec4& color_tint);

			plane_object(std::shared_ptr<shader_program> shader);
			~plane_object();

			plane_object(const plane_object&) = delete;
			plane_object& operator= (const plane_object&) = delete;

			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;

		private:
			void m_initialize();
	};
}