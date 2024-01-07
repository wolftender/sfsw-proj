#include "gui.hpp"

namespace mini {
	namespace gui {
		void prefix_label (const std::string & label, float min_width) {
			float width = ImGui::CalcItemWidth ();

			if (width < min_width) {
				width = min_width;
			}

			float x = ImGui::GetCursorPosX ();
			ImGui::Text (label.c_str ());
			ImGui::SameLine ();
			ImGui::SetCursorPosX (x + width * 0.5f + ImGui::GetStyle ().ItemInnerSpacing.x);
			ImGui::SetNextItemWidth (-1);
		}

		bool vector_editor (const std::string & label, glm::vec2 & vector) {
			const std::string label_x = "##" + label + "_x";
			const std::string label_y = "##" + label + "_y";

			bool changed = false;

			if (ImGui::TreeNode (label.c_str ())) {
				gui::prefix_label ("X: ", 100.0f);
				changed = ImGui::InputFloat (label_x.c_str (), &vector[0]) || changed;

				gui::prefix_label ("Y: ", 100.0f);
				changed = ImGui::InputFloat (label_y.c_str (), &vector[1]) || changed;

				ImGui::TreePop ();
			}

			return changed;
		}

        bool vector_editor_2(const std::string& label, glm::vec2& vector) {
            const std::string label_x = "##" + label + "_x";
            const std::string label_y = "##" + label + "_y";

            bool changed = false;

            gui::prefix_label("X: ", 100.0f);
            changed = ImGui::InputFloat(label_x.c_str(), &vector[0]) || changed;

            gui::prefix_label("Y: ", 100.0f);
            changed = ImGui::InputFloat(label_y.c_str(), &vector[1]) || changed;

            return changed;
        }

		bool vector_editor (const std::string & label, glm::vec3 & vector) {
			const std::string label_x = "##" + label + "_x";
			const std::string label_y = "##" + label + "_y";
			const std::string label_z = "##" + label + "_z";

			bool changed = false;

			if (ImGui::TreeNode (label.c_str ())) {
				gui::prefix_label ("X: ", 100.0f);
				changed = ImGui::InputFloat (label_x.c_str (), &vector[0]) || changed;

				gui::prefix_label ("Y: ", 100.0f);
				changed = ImGui::InputFloat (label_y.c_str (), &vector[1]) || changed;

				gui::prefix_label ("Z: ", 100.0f);
				changed = ImGui::InputFloat (label_z.c_str (), &vector[2]) || changed;

				ImGui::TreePop ();
			}

			return changed;
		}

        bool vector_editor_2(const std::string& label, glm::vec3& vector) {
            const std::string label_x = "##" + label + "_x";
            const std::string label_y = "##" + label + "_y";
            const std::string label_z = "##" + label + "_z";

            bool changed = false;

            gui::prefix_label("X: ", 100.0f);
            changed = ImGui::InputFloat(label_x.c_str(), &vector[0]) || changed;

            gui::prefix_label("Y: ", 100.0f);
            changed = ImGui::InputFloat(label_y.c_str(), &vector[1]) || changed;

            gui::prefix_label("Z: ", 100.0f);
            changed = ImGui::InputFloat(label_z.c_str(), &vector[2]) || changed;

            return changed;
        }

		bool color_editor (const std::string & label, glm::vec4 & color) {
			float c[3] = { color.r, color.g, color.b };

			if (ImGui::ColorEdit3 (label.c_str (), c)) {
				color.r = c[0];
				color.g = c[1];
				color.b = c[2];
				color.a = 1.0f;

				return true;
			}

			return false;
		}
		
        static ImVector<ImRect> s_GroupPanelLabelStack;

        void BeginGroupPanel(const char* name, const ImVec2& size) {
            ImGui::BeginGroup();

            auto itemSpacing = ImGui::GetStyle().ItemSpacing;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

            auto frameHeight = ImGui::GetFrameHeight();
            ImGui::BeginGroup();

            ImVec2 effectiveSize = size;
            if (size.x < 0.0f)
                effectiveSize.x = ImGui::GetContentRegionAvail().x;
            else
                effectiveSize.x = size.x;
            ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

            ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::BeginGroup();
            ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::TextUnformatted(name);
            auto labelMin = ImGui::GetItemRectMin();
            auto labelMax = ImGui::GetItemRectMax();
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
            ImGui::BeginGroup();

            //ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

            ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
            ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
            ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
            ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
            ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
            ImGui::GetCurrentWindow()->Size.x -= frameHeight;

            auto itemWidth = ImGui::CalcItemWidth();
            ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

            s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
        }

        void EndGroupPanel() {
            ImGui::PopItemWidth();

            auto itemSpacing = ImGui::GetStyle().ItemSpacing;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

            auto frameHeight = ImGui::GetFrameHeight();

            ImGui::EndGroup();

            //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

            ImGui::EndGroup();

            ImGui::SameLine(0.0f, 0.0f);
            ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
            ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

            ImGui::EndGroup();

            auto itemMin = ImGui::GetItemRectMin();
            auto itemMax = ImGui::GetItemRectMax();
            //ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

            auto labelRect = s_GroupPanelLabelStack.back();
            s_GroupPanelLabelStack.pop_back();

            ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
            ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
            labelRect.Min.x -= itemSpacing.x;
            labelRect.Max.x += itemSpacing.x;
            for (int i = 0; i < 4; ++i) {
                switch (i) {
                    // left half-plane
                case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
                    // right half-plane
                case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
                    // top
                case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
                    // bottom
                case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
                }

                ImGui::GetWindowDrawList()->AddRect(
                    frameRect.Min, frameRect.Max,
                    ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
                    halfFrame.x);

                ImGui::PopClipRect();
            }

            ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
            ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
            ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
            ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
            ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
            ImGui::GetCurrentWindow()->Size.x += frameHeight;

            ImGui::Dummy(ImVec2(0.0f, 0.0f));

            ImGui::EndGroup();
        }
	}
}