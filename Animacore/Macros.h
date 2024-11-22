#pragma once
#include "CoreSystemEvents.h"

#define LOG_ERROR(scene, fmt, ...) \
    (scene).GetDispatcher()->trigger<core::OnThrow>({core::OnThrow::Error, fmt, ##__VA_ARGS__})

#define LOG_WARN(scene, fmt, ...) \
    (scene).GetDispatcher()->trigger<core::OnThrow>({core::OnThrow::Warn, fmt, ##__VA_ARGS__})

#define LOG_INFO(scene, fmt, ...) \
    (scene).GetDispatcher()->trigger<core::OnThrow>({core::OnThrow::Info, fmt, ##__VA_ARGS__})

#define LOG_DEBUG(scene, fmt, ...) \
    (scene).GetDispatcher()->trigger<core::OnThrow>({core::OnThrow::Debug, fmt, ##__VA_ARGS__})

// dispatcher version
#define LOG_ERROR_D(dispatcher, fmt, ...) \
    (dispatcher).trigger<core::OnThrow>({core::OnThrow::Error, fmt, ##__VA_ARGS__})

#define LOG_WARN_D(dispatcher, fmt, ...) \
    (dispatcher).trigger<core::OnThrow>({core::OnThrow::Warn, fmt, ##__VA_ARGS__})

#define LOG_INFO_D(dispatcher, fmt, ...) \
    (dispatcher).trigger<core::OnThrow>({core::OnThrow::Info, fmt, ##__VA_ARGS__})

#define LOG_DEBUG_D(dispatcher, fmt, ...) \
    (dispatcher).trigger<core::OnThrow>({core::OnThrow::Debug, fmt, ##__VA_ARGS__})