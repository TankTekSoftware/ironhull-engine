#include <IronHull/utils/DataUtils.hpp>
#include <algorithm>

namespace IronHull
{
    unsigned char DataUtils::float_to_byte(float value)
    {
        int clamped = std::clamp(static_cast<int>(value * 255.0f + 0.5f), 0, 255);
        return static_cast<unsigned char>(clamped);
    }
}
