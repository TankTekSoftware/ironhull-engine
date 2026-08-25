#pragma once

#include <raylib.h>
#include <string>

namespace IronHull
{
    class Sprite2D
    {
        public:
            Vector2 position;
            Vector2 scale;
            float rotation;
            Vector2 origin;
            Color tint;
        private:
            Texture2D texture;
        public:
            Sprite2D(const std::string& texture_key);
        public:
            void center_origin();
            void draw();
            void set_filter(TextureFilter filter);
        public:
            int get_width() const;
            int get_height() const;
            Rectangle get_bounds() const;
    };
}
