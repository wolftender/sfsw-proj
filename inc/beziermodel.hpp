#pragma once
#include "mesh.hpp"
#include "context.hpp"
#include "texture.hpp"
#include "beziercube.hpp"

namespace mini {
	class bezier_model_object : public graphics_object {
		private:
			constexpr static int BEZIER_POINT_COUNT = 64;

			std::shared_ptr<triangle_mesh> m_mesh;
			std::shared_ptr<texture> m_texture;
			std::shared_ptr<shader_program> m_shader;

			std::array<glm::vec3, BEZIER_POINT_COUNT> m_control_points;
			glm::vec4 m_surface_color;

		public:
			const glm::vec4 & get_surface_color() const;
			void set_surface_color(const glm::vec4& color);

			void update_point(int index, const glm::vec3& position);

			bezier_model_object(
				std::shared_ptr<shader_program> shader, 
				std::shared_ptr<triangle_mesh> mesh, 
				std::shared_ptr<texture> texture);

			~bezier_model_object();

			bezier_model_object(const bezier_model_object&) = delete;
			bezier_model_object& operator=(const bezier_model_object&) = delete;

			// Inherited via graphics_object
			void render(app_context& context, const glm::mat4x4& world_matrix) const override;
	};
}