#include "Physics.h"

bool Physics::overlaps(const Aabb& a, const Aabb& b)
{
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

Aabb Physics::placeBounds(const Aabb& local, Vec3 origin, float scale)
{
    return {
        {origin.x + local.min.x * scale, origin.y + local.min.y * scale, origin.z + local.min.z * scale},
        {origin.x + local.max.x * scale, origin.y + local.max.y * scale, origin.z + local.max.z * scale}
    };
}
