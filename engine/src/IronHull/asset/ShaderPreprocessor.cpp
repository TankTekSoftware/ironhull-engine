#include <IronHull/asset/ShaderPreprocessor.hpp>

#include <sstream>
#include <stdexcept>

namespace IronHull
{
    namespace
    {
        std::string trim(const std::string& text)
        {
            size_t start = text.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }

            size_t end = text.find_last_not_of(" \t\r\n");
            return text.substr(start, end - start + 1);
        }
    }

    ShaderPreprocessor::Stages ShaderPreprocessor::split(const std::string& source)
    {
        static const std::string directive = "#type";

        Stages stages;
        std::string* current = nullptr;

        std::istringstream stream(source);
        std::string line;

        while (std::getline(stream, line)) {
            std::string trimmed = trim(line);

            if (trimmed.compare(0, directive.size(), directive) == 0) {
                std::string type = trim(trimmed.substr(directive.size()));

                if (type == "vertex") {
                    current = &stages.vertex;
                    stages.has_vertex = true;
                } else if (type == "fragment") {
                    current = &stages.fragment;
                    stages.has_fragment = true;
                } else {
                    throw std::invalid_argument("ShaderPreprocessor: unknown shader type '" + type + "' (expected 'vertex' or 'fragment')");
                }

                continue;
            }

            if (current != nullptr) {
                *current += line;
                *current += '\n';
            }
        }

        return stages;
    }
}
