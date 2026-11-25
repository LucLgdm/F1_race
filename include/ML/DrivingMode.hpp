#pragma once
#include <cstddef>

enum class DrivingMode { Chrono, Race };
inline constexpr std::size_t drivingInputCount(DrivingMode mode) {
    return mode == DrivingMode::Race ? 23 : 8;
}
