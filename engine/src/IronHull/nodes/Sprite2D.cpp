#include "IronHull/asset/AssetRegistry.hpp"
#include "raylib.h"
#include <IronHull/nodes/Sprite2D.hpp>
#include <string>

namespace IronHull
{
    Sprite2D::Sprite2D(const std::string& texture_key)
    {
        this->texture = AssetRegistry::get<Texture2D>(texture_key);
        this->position = { 0.0f, 0.0f };
        this->rotation = 0.0f;
        this->scale = { 1.0f, 1.0f };
        this->origin = { 0.0f, 0.0f };
        this->tint = WHITE;
    }

    void Sprite2D::draw()
    {
        DrawTexturePro(
                this->texture,
                { 0.0f, 0.0f, (float)this->texture.width, (float)this->texture.height },
                { this->position.x, this->position.y, (float)this->texture.width * this->scale.x, (float)this->texture.height * this->scale.y},
                { this->origin.x * this->scale.x, this->origin.y * this->scale.y },
                this->rotation,
                this->tint
        );

    }

    void Sprite2D::set_filter(TextureFilter filter)
    {
        SetTextureFilter(this->texture, filter);
    }

    void Sprite2D::center_origin()
    {
        this->origin = {
            this->get_width() / 2.0f,
            this->get_height() / 2.0f
        }; 
    }

    int Sprite2D::get_width() const
    {
        return this->texture.width;
    }

    int Sprite2D::get_height() const
    {
        return this->texture.height;
    }

    Rectangle Sprite2D::get_bounds() const
    {
        return {
            this->position.x - this->origin.x * this->scale.x,
            this->position.y - this->origin.y * this->scale.y,
            this->get_width() * this->scale.x,
            this->get_height() * this->scale.y
        };
    }
};

