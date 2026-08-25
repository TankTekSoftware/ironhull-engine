#pragma once

#include "raylib.h"

namespace IronHull
{
    // Draws a flat-filled, untextured rectangle - useful for UI backdrops, overlays,
    // and other solid fills that don't need a texture or shader.
    class ColorRect
    {
        public:
            Vector2 position;
            Vector2 size;
            Color color;
        public:
            ColorRect(Vector2 size);
        public:
            void draw();
    };
}
