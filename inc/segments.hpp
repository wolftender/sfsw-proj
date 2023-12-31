#pragma once
#include "context.hpp"

namespace mini {
	class segments_array : public graphics_object {
		public:
			using segment_t = std::pair<std::size_t, std::size_t>;
		
		private:
			std::vector<glm::vec3> m_points;
			std::shared_ptr<shader_program> m_line_shader;

			std::vector<float> m_positions;
			std::vector<uint32_t> m_indices;
			GLuint m_vao, m_position_buffer, m_index_buffer;

			bool m_ready;
			bool m_ignore_depth;

			glm::vec4 m_color;
			float m_line_width;

		public:
			segments_array(
				std::shared_ptr<shader_program> line_shader, 
				std::size_t num_points);
			~segments_array();

			segments_array(const segments_array&) = delete;
			segments_array& operator=(const segments_array&) = delete;

			bool get_ignore_depth() const;
			void set_ignore_depth(bool ignore);

			float get_line_width() const;
			const glm::vec4& get_color() const;

			void set_line_width(float width);
			void set_color(const glm::vec4& color);

			void add_segment(std::size_t begin, std::size_t end);
			void add_segments(const std::vector<segment_t>& segments);
			void clear_segments();

			void update_point(std::size_t index, const glm::vec3& data);
			void update_points(const std::vector<glm::vec3>& points);
			void rebuild_buffers();

			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;

		private:
			void m_rebuild_buffers();
			void m_free_buffers();
	};
}