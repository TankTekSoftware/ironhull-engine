#pragma once

#include "IronHull/nodes/Node.hpp"

#include <box3d/box3d.h>

namespace IronHull
{
    class PhysicsWorld3D : public Node
    {
        private:
            static PhysicsWorld3D* instance;
            b3WorldId world_id;
            float accumulator = 0.0f;
        public:
            static PhysicsWorld3D* get_singleton();
        public:
            void compose();
            void step(float delta);
            void dispose();
            b3WorldId get_world_id() const;
        private:
            void process_contact_events();
    };
}
