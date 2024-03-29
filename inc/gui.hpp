#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS

#include <string>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <implot.h>
#include <implot_internal.h>

#include <glm/glm.hpp>

namespace mini {
	namespace gui {
		void prefix_label (const std::string & label, float min_width = 0.0f);

		bool vector_editor(const std::string& label, glm::vec2& vector);
		bool vector_editor(const std::string& label, glm::vec3& vector);

		bool vector_editor_2(const std::string& label, glm::vec2& vector);
		bool vector_editor_2(const std::string & label, glm::vec3 & vector);
		
		template<typename T> void clamp (T & value, T min, T max) {
			if (value < min) {
				value = min;
			} else if (value > max) {
				value = max;
			}
		}

		bool color_editor (const std::string & label, glm::vec4 & color);

		void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));
		void EndGroupPanel();
	}
}