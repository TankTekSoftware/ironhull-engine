#pragma once

#include "raylib.h"
#include <string>

namespace IronHull
{
    // Draws static, non-interactive text - useful for HUD readouts, dialogue, and other
    // labels that don't need TextButton's hover/press handling. Measures itself against
    // the loaded font so callers can center/align it, and re-measures on demand since
    // `text` is mutable after construction (e.g. a score counter changing every frame).
    class Label
    {
        public:
            Vector2 position;
            Vector2 origin;
            std::string text;
            float font_size;
            float spacing;
            Color color;
        private:
            Font font;
        public:
            Label(const std::string& font_key, const std::string& text);
        public:
            void center_origin();
            void draw();
        public:
            Vector2 get_text_size() const;
            Rectangle get_bounds() const;
    };
}
