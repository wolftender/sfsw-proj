#pragma once
#include <vector>
#include <filesystem>
#include <memory>

#include <glm/glm.hpp>

#include "shader.hpp"

namespace fs = std::filesystem;

namespace mini {
	class triangle_mesh {
		private:
			std::vector<float> m_positions;
			std::vector<float> m_normals;
			std::vector<float> m_uvs;
			std::vector<uint32_t> m_indices;

			GLuint m_array_object;
			GLuint m_position_buffer;
			GLuint m_normal_buffer;
			GLuint m_uv_buffer;
			GLuint m_index_buffer;

		public:
			const std::vector<float>& get_positions() const;
			const std::vector<float>& get_normals() const;
			const std::vector<float>& get_uvs() const;
			const std::vector<uint32_t>& get_indices() const;

			triangle_mesh(
				const std::vector<float>& positions, 
				const std::vector<float>& normals,
				const std::vector<float>& uvs,
				const std::vector<uint32_t>& indices);

			~triangle_mesh();

			triangle_mesh(const triangle_mesh&);
			triangle_mesh& operator= (const triangle_mesh&);

			void draw();

		private:
			void m_initialize();
			void m_destroy();

		public:
			static std::shared_ptr<triangle_mesh> read_from_file(const fs::path& path, const glm::vec3& offset, const glm::vec3& scale);
			static std::shared_ptr<triangle_mesh> make_plane_mesh();
			static std::shared_ptr<triangle_mesh> make_plane_mesh_front();
			static std::shared_ptr<triangle_mesh> make_plane_mesh_back();
			static std::shared_ptr<triangle_mesh> make_cube_mesh();
			static std::shared_ptr<triangle_mesh> make_cylinder(float radius, float height, int latitudes, int longitudes);
	};
}