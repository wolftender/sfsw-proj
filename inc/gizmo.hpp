#pragma once
#include <glad/glad.h>

#include "context.hpp"

namespace mini {
	class gizmo : public graphics_object {
		private:
			struct gizmo_mesh_t {
				GLuint pos_buffer, index_buffer, vao;

				std::vector<float> positions;
				std::vector<GLuint> indices;

				gizmo_mesh_t() {
					pos_buffer = 0;
					index_buffer = 0;
					vao = 0;
				}
			};

			gizmo_mesh_t m_mesh_arrow;
			std::shared_ptr<shader_program> m_shader_mesh;

		public:
			gizmo(std::shared_ptr<shader_program> shader_mesh);
			~gizmo();

			gizmo(const gizmo&) = delete;
			gizmo& operator= (const gizmo&) = delete;

			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;

		private:
			void m_gen_arrow_mesh(gizmo_mesh_t& mesh);
			void m_free_mesh(gizmo_mesh_t& mesh);
	};
}