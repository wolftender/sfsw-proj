#pragma once
#include "mesh.hpp"
#include "context.hpp"
#include "texture.hpp"

namespace mini {
	class model_object : public graphics_object {
		private:
			std::shared_ptr<triangle_mesh> m_mesh;
			std::shared_ptr<shader_program> m_shader;
			glm::vec4 m_surface_color;

		public:
			const glm::vec4& get_surface_color() const;
			void set_surface_color(const glm::vec4& color);

			model_object(std::shared_ptr<triangle_mesh> mesh, std::shared_ptr<shader_program> shader);
			model_object(const model_object&) = delete;
			model_object& operator=(const model_object&) = delete;

			// Inherited via graphics_object
			void render(app_context& context, const glm::mat4x4& world_matrix) const override;
	};
}