#include "copy_file.h"

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
}

namespace fs = std::filesystem;

using namespace imc::errors;

int imc::gui::ask_copy(types::op_file_t& copy_file)
{
    int ret = didnt_do_nothin;
    if (std::strlen(copy_file.file.data()) == 0)
        return ret;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(4.0 * 150.0f, 0.0f));
    if (ImGui::BeginPopupModal("Copy File", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Copy %s to:", copy_file.old_file.filename().generic_string().c_str());
        ImGui::SetNextItemWidth(-1.0f);
        bool do_ok = false;
        if (ImGui::InputText("##copyfile",  copy_file.file.data(), copy_file.file.size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
            do_ok = true;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", last_error.c_str());
        if (ImGui::Button("OK") || do_ok) {
            std::error_code ec = backend::copy(copy_file.old_file, fs::path(copy_file.file.data()));
            if (ec) {
                //hey there was an error
                last_error = ec.message();
                ret = failed_to_copy;
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

