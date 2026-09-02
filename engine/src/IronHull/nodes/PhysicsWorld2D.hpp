#pragma once

#include "IronHull/nodes/Node.hpp"

#include <box2d/box2d.h>

namespace IronHull 
{
    class PhysicsWorld2D : public Node
    {
        private:
            static PhysicsWorld2D* instance;
            b2WorldId world_id;
            float accumulator = 0.0f;
        public:
            static PhysicsWorld2D* get_singleton();
        public:
            void compose();
            void step(float delta);
            void dispose();
            b2WorldId get_world_id() const;
        private:
            void process_contact_events();
    };
}
