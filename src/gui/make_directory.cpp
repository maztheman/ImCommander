#include "make_directory.h"

#include "imgui.h"

#include "backend/file_operations.h"
#include "types/op_file.h"
#include "types/errors.h"

#include <vector>
#include <string>
#include <filesystem>

namespace {
    std::string last_error = "";
}

namespace fs = std::filesystem;

using namespace imc::errors;

int imc::gui::ask_make_directory(types::op_file_t& directory)
{
    int ret = didnt_do_nothin;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(4.0 * 150.0f, 0.0f));
    if (ImGui::BeginPopupModal("MkDir", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("Folder's Name:");
        bool do_ok = false;
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##makedir",  directory.file.data(), directory.file.size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
            do_ok = true;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", last_error.c_str());
        if (ImGui::Button("OK") || do_ok) {
            std::error_code ec = backend::make_directory(directory.old_file.parent_path() / directory.file.data());
            if (ec) {
                //hey there was an error
                last_error = ec.message();
                ret = failed_to_mkdir;
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

