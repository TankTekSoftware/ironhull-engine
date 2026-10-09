// ihbsp - the IronHull map compiler.
//
// Reads an authored brush map and writes the compiled form the engine loads:
//
//   ihbsp sandbox/maps/testmap.map
//   ihbsp sandbox/maps/testmap.map -o build/testmap.ihbsp --scale 0.03125
//   ihbsp --info sandbox/maps/testmap.ihbsp
//
// This is a build step, so it works in ordinary filesystem paths rather than the engine's
// content:// URIs: it runs before there is a packaged content directory to read, and it has
// no business writing into one. The compilation itself lives in the engine library, so the
// editor can compile the map it has open without going through a file at all.

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <IronHull/map/BspFile.hpp>
#include <IronHull/map/MapFile.hpp>
#include <IronHull/map/compile/MapCompiler.hpp>

namespace
{
    void print_usage()
    {
        std::cout
            << "ihbsp - IronHull map compiler\n\n"
            << "Usage:\n"
            << "  ihbsp <input.map> [options]\n"
            << "  ihbsp --info <compiled.ihbsp>\n\n"
            << "Options:\n"
            << "  -o, --output <path>   Where to write the compiled map.\n"
            << "                        Defaults to the input path with an .ihbsp extension.\n"
            << "      --scale <factor>  Scale applied to every distance. Map units are inches by\n"
            << "                        convention; 0.03125 (1/32) gives roughly metres. Default 1.\n"
            << "      --max-depth <n>   Hard limit on BSP tree depth. Default 96.\n"
            << "      --no-csg          Keep surfaces buried inside other brushes.\n"
            << "      --no-cull         Keep the map's outward-facing skin.\n"
            << "      --info            Print a compiled map's contents instead of compiling.\n"
            << "  -q, --quiet           Only report warnings and errors.\n"
            << "  -h, --help            Show this message.\n";
    }

    bool read_file(const std::string& path, std::string& out)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return !file.bad();
    }

    bool write_file(const std::string& path, const std::vector<unsigned char>& bytes)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }

        if (!bytes.empty()) {
            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        return file.good();
    }

    std::string replace_extension(const std::string& path, const std::string& extension)
    {
        size_t dot = path.find_last_of('.');
        size_t slash = path.find_last_of("/\\");

        // Only strip a dot that belongs to the filename, not one in a directory name.
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
            return path + extension;
        }

        return path.substr(0, dot) + extension;
    }

    void print_report(const IronHull::MapCompiler::Report& report)
    {
        std::printf("  brushes     %d\n", report.brushes);
        std::printf("  faces       %d (%d culled)\n", report.faces, report.culled_faces);
        std::printf("  vertices    %d\n", report.vertices);
        std::printf("  triangles   %d\n", report.triangles);
        std::printf("  nodes       %d\n", report.nodes);
        std::printf("  leaves      %d (%d solid)\n", report.leaves, report.solid_leaves);
        std::printf("  tree depth  %d\n", report.max_depth);
        std::printf("  areas       %d\n", report.areas);
        std::printf("  models      %d\n", report.models);
        std::printf("  entities    %d\n", report.entities);
        std::printf("  textures    %d\n", report.textures);
        std::printf("  time        %.3fs\n", report.seconds);
    }

    int print_info(const std::string& path)
    {
        std::string text;
        if (!read_file(path, text)) {
            std::fprintf(stderr, "ihbsp: cannot read '%s'\n", path.c_str());
            return 1;
        }

        std::vector<unsigned char> bytes(text.begin(), text.end());

        try {
            IronHull::CompiledMap map = IronHull::BspFile::read(bytes);

            std::printf("%s\n", path.c_str());
            std::printf("  planes      %zu\n", map.planes.size());
            std::printf("  vertices    %zu\n", map.vertices.size());
            std::printf("  triangles   %zu\n", map.indices.size() / 3);
            std::printf("  faces       %zu\n", map.faces.size());
            std::printf("  nodes       %zu\n", map.nodes.size());
            std::printf("  leaves      %zu\n", map.leaves.size());
            std::printf("  brushes     %zu\n", map.brushes.size());
            std::printf("  models      %zu\n", map.models.size());
            std::printf("  entities    %zu\n", map.entities.size());

            std::printf("  textures    %zu\n", map.textures.size());
            for (const IronHull::BspTexture& texture : map.textures) {
                std::printf("    %s\n", texture.name.c_str());
            }

            std::printf("  entity list\n");
            for (const IronHull::BspEntity& entity : map.entities) {
                std::printf("    %s", entity.classname().c_str());

                std::string origin = entity.get("origin");
                if (!origin.empty()) {
                    std::printf(" at %s", origin.c_str());
                }

                std::string model = entity.get("model");
                if (!model.empty()) {
                    std::printf(" model %s", model.c_str());
                }

                if (!entity.connections.empty()) {
                    std::printf(" (%zu connections)", entity.connections.size());
                }

                std::printf("\n");
            }

            return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "ihbsp: %s\n", error.what());
            return 1;
        }
    }
}

int main(int argc, char* argv[])
{
    std::string input;
    std::string output;
    bool info = false;
    bool quiet = false;

    IronHull::MapCompiler::Options options;

    for (int index = 1; index < argc; ++index) {
        std::string argument = argv[index];

        auto next_value = [&](const char* name) -> std::string {
            if (index + 1 >= argc) {
                std::fprintf(stderr, "ihbsp: %s needs a value\n", name);
                std::exit(1);
            }

            return argv[++index];
        };

        if (argument == "-h" || argument == "--help") {
            print_usage();
            return 0;
        } else if (argument == "-q" || argument == "--quiet") {
            quiet = true;
        } else if (argument == "--info") {
            info = true;
        } else if (argument == "-o" || argument == "--output") {
            output = next_value("--output");
        } else if (argument == "--scale") {
            options.scale = std::stof(next_value("--scale"));
        } else if (argument == "--max-depth") {
            options.max_depth = std::stoi(next_value("--max-depth"));
        } else if (argument == "--no-csg") {
            options.face_csg = false;
        } else if (argument == "--no-cull") {
            options.cull_outside = false;
        } else if (!argument.empty() && argument[0] == '-') {
            std::fprintf(stderr, "ihbsp: unknown option '%s'\n", argument.c_str());
            return 1;
        } else if (input.empty()) {
            input = argument;
        } else {
            std::fprintf(stderr, "ihbsp: unexpected extra argument '%s'\n", argument.c_str());
            return 1;
        }
    }

    if (input.empty()) {
        print_usage();
        return 1;
    }

    if (info) {
        return print_info(input);
    }

    if (options.scale <= 0.0f) {
        std::fprintf(stderr, "ihbsp: --scale must be greater than zero\n");
        return 1;
    }

    if (output.empty()) {
        output = replace_extension(input, ".ihbsp");
    }

    std::string text;
    if (!read_file(input, text)) {
        std::fprintf(stderr, "ihbsp: cannot read '%s'\n", input.c_str());
        return 1;
    }

    try {
        IronHull::MapFile map = IronHull::MapFile::parse(text);

        IronHull::MapCompiler::Report report;
        IronHull::CompiledMap compiled = IronHull::MapCompiler::compile(map, options, report);

        std::vector<unsigned char> bytes = IronHull::BspFile::write(compiled);

        if (!write_file(output, bytes)) {
            std::fprintf(stderr, "ihbsp: cannot write '%s'\n", output.c_str());
            return 1;
        }

        for (const std::string& warning : report.warnings) {
            std::fprintf(stderr, "ihbsp: warning: %s\n", warning.c_str());
        }

        if (!quiet) {
            std::printf("%s -> %s (%zu bytes)\n", input.c_str(), output.c_str(), bytes.size());
            print_report(report);
        }

        return 0;
    } catch (const IronHull::MapParseError& error) {
        std::fprintf(stderr, "ihbsp: %s\n", error.what());
        return 1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "ihbsp: %s\n", error.what());
        return 1;
    }
}
