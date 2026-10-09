#pragma once

#include <raylib.h>

#include <IronHull/geometry/Plane.hpp>

namespace IronHull
{
    // Converts between the two coordinate systems a map lives in.
    //
    // Brushes are authored Z-up: that is what every brush editor works in, what the Quake
    // .map format assumes, and what the entity bounds in `content://entities/*.lua` mean
    // when they describe something 72 units tall. The engine renders Y-up, because raylib
    // and Box3D do.
    //
    // Something has to bridge the two, and doing it once in the compiler - rather than at
    // every draw call or collision query - is what keeps the runtime free of the question.
    // Map space goes in, engine space comes out, and nothing downstream of the compiler
    // deals with Z-up again.
    //
    // The mapping is (x, y, z) -> (x, z, -y): a quarter turn about X. It preserves
    // handedness, so brushes do not come out mirrored and face windings keep their facing.
    //
    // `scale` multiplies distances on the way out. Map units are inches by convention, so a
    // scale of 1 keeps one unit to one engine unit, and 1/32 gives roughly metres - which is
    // what Box3D's default gravity assumes. The compiler exposes it as an option rather than
    // picking for you.
    class MapSpace
    {
        public:
            static Vector3 to_engine(Vector3 point, float scale = 1.0f);
            static Vector3 to_map(Vector3 point, float scale = 1.0f);

        public:
            // Normals rotate like points but are not scaled; the distance from the origin is
            // scaled instead, which is what keeps the plane describing the same surface.
            static Plane to_engine(const Plane& plane, float scale = 1.0f);

            // A box has to be rebuilt rather than rotated, since the mapping permutes and
            // negates axes and so does not keep min below max.
            static BoundingBox bounds_to_engine(const BoundingBox& box, float scale = 1.0f);

        public:
            // Builds a direction from a Quake-style (pitch, yaw, roll) angles triple, as
            // authored on an entity, and returns it in engine space.
            //
            // Angles are deliberately left as authored in the compiled map rather than
            // converted: they are an Euler convention, not a coordinate, and rewriting them
            // into some engine-space equivalent would make an entity's "angles" property
            // stop matching what the mapper typed. This is where the convention is applied.
            static Vector3 angles_to_forward(Vector3 angles);
            static Vector3 angles_to_right(Vector3 angles);
            static Vector3 angles_to_up(Vector3 angles);
    };
}
