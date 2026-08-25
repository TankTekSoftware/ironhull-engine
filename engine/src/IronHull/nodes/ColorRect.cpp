#include "raylib.h"
#include <IronHull/nodes/ColorRect.hpp>

namespace IronHull
{
    ColorRect::ColorRect(Vector2 size)
    {
        this->position = { 0.0f, 0.0f };
        this->size = size;
        this->color = WHITE;
    }

    void ColorRect::draw()
    {
        DrawRectangleV(this->position, this->size, this->color);
    }
};
