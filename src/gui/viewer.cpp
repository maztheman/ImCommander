#include "viewer.h"

#include "imgui.h"

#include <fmt/format.h>

#include <fstream>

namespace {
    constexpr size_t MAX_VIEW_FILESIZE = 10ULL * 1024ULL * 1024ULL;
    std::filesystem::path last_file;
    std::filesystem::file_time_type old_file_time;
    std::string file_data;
}

int imc::gui::view_file(const fs::path& file)
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("View File", nullptr, ImGuiWindowFlags_None)) {
        if (last_file != file || fs::last_write_time(file) > old_file_time) {
            auto sz = fs::file_size(file);
            if (sz > MAX_VIEW_FILESIZE) {
                file_data = fmt::format("File '{}' is too large ({})\n", file.generic_string(), sz);
            } else {
                //re-read file
                std::fstream f(file);
                file_data.resize(sz);
                f.read(file_data.data(), sz);
            }
        }
        float height = ImGui::GetWindowHeight();
        if (ImGui::BeginChild("ViewFileData", ImVec2(0.0f, height - 62.0f), 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::TextUnformatted(file_data.c_str());
        }
        ImGui::EndChild();
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    return 0;
}
