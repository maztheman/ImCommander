#include "move_file.h"

#include "imgui.h"

#include "backend/file_operations.h"
#include "types/op_file.h"
#include "types/errors.h"

#include <vector>
#include <string>
#include <filesystem>
#include <cstring>

namespace {
    std::string last_error = "";
    std::pair<std::error_code, std::error_code> last_ec;
    bool can_override = false;
}

namespace fs = std::filesystem;

using namespace imc::errors;

int imc::gui::ask_move(types::op_file_t& move_file)
{
    int ret = didnt_do_nothin;
    if (std::strlen(move_file.file.data()) == 0)
        return ret;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(4.0 * 150.0f, 0.0f));
    if (ImGui::BeginPopupModal("Move File", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Move %s to:", move_file.old_file.filename().generic_string().c_str());
        ImGui::SetNextItemWidth(-1.0f);
        bool do_ok = false;
        if (ImGui::InputText("##movefile",  move_file.file.data(), move_file.file.size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
            do_ok = true;
        }
        ImGui::Checkbox("Can overwrite?", &can_override);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", last_error.c_str());
        if (ImGui::Button("OK") || do_ok) {
            std::pair<std::error_code, std::error_code> ec = backend::move(move_file.old_file, fs::path(move_file.file.data()), can_override);
            last_ec = ec;
            if (ec.first) {
                //hey there was an error
                last_error = ec.first.message();
                ret = failed_to_copy;
            } else if (ec.second) {
                last_error = ec.second.message();
                ret = failed_to_remove;
            } else {
                ret = success;
                last_error.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            last_error.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return ret;
}

