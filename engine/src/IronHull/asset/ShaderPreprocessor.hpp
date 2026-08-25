#pragma once

#include <string>

namespace IronHull
{
    // Splits a single shader source file into per-stage GLSL, so a vertex stage and a
    // fragment stage (or just a fragment stage) can be authored together in one file.
    // Stages are marked with a `#type vertex` / `#type fragment` line; everything up to
    // the next `#type` line (or end of file) belongs to that stage, and any text before
    // the first `#type` line is discarded. This keeps each stage's own `#version` line
    // first in its block, which most GLSL compilers require.
    class ShaderPreprocessor
    {
        public:
            struct Stages
            {
                std::string vertex;
                std::string fragment;
                bool has_vertex = false;
                bool has_fragment = false;
            };

        public:
            static Stages split(const std::string& source);
    };
}
