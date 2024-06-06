#pragma once
#include <functional>
#include <optional>

#include "context.hpp"
#include "shader.hpp"
#include "cubemap.hpp"

namespace mini {
	class black_hole_quad : public graphics_object {
		private:
			std::shared_ptr<shader_program> m_program;
			std::shared_ptr<cubemap> m_cubemap;

			GLuint m_vao, m_pos_buffer;

		public:
			black_hole_quad(std::shared_ptr<shader_program> program, std::shared_ptr<cubemap> cubemap);
			~black_hole_quad();

			black_hole_quad(const black_hole_quad&) = delete;
			black_hole_quad& operator=(const black_hole_quad&) = delete;
			
			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;
	};
}