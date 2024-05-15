#include "delete_file.h"

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
    std::error_code last_ec;
}

namespace fs = std::filesystem;

using namespace imc::errors;

int imc::gui::ask_delete(types::op_file_t& delete_file)
{
    int ret = didnt_do_nothin;
    if (std::strlen(delete_file.file.data()) == 0)
        return ret;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(4.0 * 150.0f, 0.0f));
    if (ImGui::BeginPopupModal("Delete File", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("Do you really want to delete the items below?");
        if (ImGui::BeginListBox("##delete-list-1", ImVec2(-1.0f, 0.0f))) {
            ImGui::TextUnformatted(delete_file.file.data());
            ImGui::EndListBox();
        }
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", last_error.c_str());
        if (ImGui::Button("Delete")) {
            std::error_code ec = backend::delete_(delete_file.old_file);
            last_ec = ec;
            if (ec) {
                //hey there was an error
                last_error = ec.message();
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

