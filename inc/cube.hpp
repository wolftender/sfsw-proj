#pragma once
#include "context.hpp"
#include "shader.hpp"

namespace mini {
	class cube_object : public graphics_object {
		public:
			enum class culling_mode_t {
				front,
				back,
				none
			};

		private:
			GLuint m_pos_buffer, m_color_buffer, m_normal_buffer, m_uv_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_program> m_shader;

			culling_mode_t m_cull_mode;
			glm::vec4 m_surface_color;

		public:
			void set_cull_mode(culling_mode_t mode);
			culling_mode_t get_cull_mode() const;

			void set_surface_color(const glm::vec4& color);
			const glm::vec4& get_surface_color() const;

			cube_object(std::shared_ptr<shader_program> shader);
			~cube_object();

			cube_object(const cube_object&) = delete;
			cube_object& operator= (const cube_object&) = delete;

			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;
	};
}