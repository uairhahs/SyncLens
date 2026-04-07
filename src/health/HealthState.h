#pragma once

enum class HealthState {
    Unknown,
    Running,
    Starting,
    Stopped,
    Unreachable,
    Error
};
