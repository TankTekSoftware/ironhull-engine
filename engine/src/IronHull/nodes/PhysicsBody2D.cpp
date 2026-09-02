#include "IronHull/core/Application.hpp"
#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/PhysicsWorld2D.hpp"
#include "IronHull/utils/DataUtils.hpp"
#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "box2d/types.h"
#include "raylib.h"
#include <IronHull/nodes/PhysicsBody2D.hpp>
#include <vector>

namespace IronHull
{
    void PhysicsBody2D::init(const std::vector<CollisionShape2D*>& shapes, b2BodyType type)
    {
        PhysicsWorld2D* world = PhysicsWorld2D::get_singleton();
        
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = type;
        body_def.position = DataUtils::to_box2d(this->position);
        body_def.userData = this;
        this->body_id = b2CreateBody(world->get_world_id(), &body_def);

        for (CollisionShape2D* shape : shapes) {
            b2ShapeDef shape_def = b2DefaultShapeDef();
            shape_def.density = shape->density;
            shape_def.material.friction = shape->friction;
            shape_def.enableContactEvents = true;

            switch (shape->shape_type) {
                case IronHull::CollisionShapeType2D::BOX:
                    {
                        b2Polygon box = b2MakeBox(
                                shape->extends.x / Application::get_physics2d().pixels_per_unit / 2.0f,
                                shape->extends.y / Application::get_physics2d().pixels_per_unit / 2.0f
                                );

                        b2CreatePolygonShape(this->body_id, &shape_def, &box);
                        break;
                    }
                case IronHull::CollisionShapeType2D::CIRCLE:
                    {
                        b2Circle circle = { { 0.0f, 0.0f }, shape->radius / Application::get_physics2d().pixels_per_unit };
                        b2CreateCircleShape(this->body_id, &shape_def, &circle);
                        break;
                    }
                case IronHull::CollisionShapeType2D::CAPSULE:
                    {
                        float r = shape->radius / Application::get_physics2d().pixels_per_unit;
                        float half_length = (shape->length / Application::get_physics2d().pixels_per_unit) / 2.0f;
                        b2Capsule capsule = { 0, -half_length, { 0, half_length }, r };
                        b2CreateCapsuleShape(this->body_id, &shape_def, &capsule);
                        break;
                    }
            }
        }
    }

}
