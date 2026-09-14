#include "IronHull/nodes/CollisionShape3D.hpp"
#include "IronHull/nodes/PhysicsWorld3D.hpp"
#include "IronHull/utils/DataUtils.hpp"
#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/types.h"
#include "raylib.h"
#include <IronHull/nodes/PhysicsBody3D.hpp>
#include <raymath.h>
#include <vector>

namespace IronHull
{
    void PhysicsBody3D::init(const std::vector<CollisionShape3D*>& shapes, b3BodyType type)
    {
        PhysicsWorld3D* world = PhysicsWorld3D::get_singleton();

        b3BodyDef body_def = b3DefaultBodyDef();
        body_def.type = type;
        body_def.position = b3ToPos(DataUtils::to_box3d(this->position));
        body_def.rotation = DataUtils::to_box3d(QuaternionNormalize(this->rotation));
        body_def.userData = this;
        this->body_id = b3CreateBody(world->get_world_id(), &body_def);

        for (CollisionShape3D* shape : shapes) {
            b3ShapeDef shape_def = b3DefaultShapeDef();
            shape_def.density = shape->density;
            shape_def.baseMaterial.friction = shape->friction;
            shape_def.enableContactEvents = true;

            // The shape's own position/rotation place it relative to the body.
            Quaternion local_rotation = QuaternionNormalize(shape->rotation);

            switch (shape->shape_type) {
                case IronHull::CollisionShapeType3D::BOX:
                    {
                        b3Transform offset = {
                            DataUtils::to_box3d(shape->position),
                            DataUtils::to_box3d(local_rotation)
                        };
                        b3BoxHull box = b3MakeTransformedBoxHull(
                                shape->extends.x / 2.0f,
                                shape->extends.y / 2.0f,
                                shape->extends.z / 2.0f,
                                offset
                                );

                        b3CreateHullShape(this->body_id, &shape_def, &box.base);
                        break;
                    }
                case IronHull::CollisionShapeType3D::SPHERE:
                    {
                        b3Sphere sphere = { DataUtils::to_box3d(shape->position), shape->radius };
                        b3CreateSphereShape(this->body_id, &shape_def, &sphere);
                        break;
                    }
                case IronHull::CollisionShapeType3D::CAPSULE:
                    {
                        Vector3 half_axis = Vector3RotateByQuaternion({ 0.0f, shape->length / 2.0f, 0.0f }, local_rotation);
                        b3Capsule capsule = {
                            DataUtils::to_box3d(Vector3Subtract(shape->position, half_axis)),
                            DataUtils::to_box3d(Vector3Add(shape->position, half_axis)),
                            shape->radius
                        };
                        b3CreateCapsuleShape(this->body_id, &shape_def, &capsule);
                        break;
                    }
            }
        }
    }
}
