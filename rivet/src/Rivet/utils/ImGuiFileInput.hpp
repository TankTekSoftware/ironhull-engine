#pragma once

#include <string>

namespace Rivet
{
    enum class FileInputMode
    {
        MODE_FOLDER,
        MODE_OPEN_FILE,
        MODE_SAVE_FILE,
    };

    class ImGuiFileInput
    {
        private:
            std::string label;
            FileInputMode mode;
            char path[256];
        public:
            ImGuiFileInput(const std::string& label, FileInputMode mode = FileInputMode::MODE_FOLDER);
        public:
            bool draw();
            void clear();
        public:
            const char* get_path() const;
            void set_path(const char* path);
            bool is_empty() const;
        private:
            bool browse();
    };
}
