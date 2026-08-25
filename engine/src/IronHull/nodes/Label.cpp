#include "IronHull/asset/AssetRegistry.hpp"
#include "raylib.h"
#include <IronHull/nodes/Label.hpp>

namespace IronHull
{
    Label::Label(const std::string& font_key, const std::string& text)
    {
        this->font = AssetRegistry::get<Font>(font_key);
        this->position = { 0.0f, 0.0f };
        this->origin = { 0.0f, 0.0f };
        this->text = text;
        this->font_size = (float)this->font.baseSize;
        this->spacing = 1.0f;
        this->color = WHITE;
    }

    void Label::draw()
    {
        DrawTextEx(
                this->font,
                this->text.c_str(),
                { this->position.x - this->origin.x, this->position.y - this->origin.y },
                this->font_size,
                this->spacing,
                this->color
        );
    }

    void Label::center_origin()
    {
        this->origin = {
            this->get_text_size().x / 2.0f,
            this->get_text_size().y / 2.0f
        };
    }

    Vector2 Label::get_text_size() const
    {
        return MeasureTextEx(this->font, this->text.c_str(), this->font_size, this->spacing);
    }

    Rectangle Label::get_bounds() const
    {
        Vector2 text_size = this->get_text_size();
        return {
            this->position.x - this->origin.x,
            this->position.y - this->origin.y,
            text_size.x,
            text_size.y
        };
    }
};
