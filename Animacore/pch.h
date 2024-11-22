#ifndef CORE_PCH_H
#define CORE_PCH_H

#define WIN32_LEAN_AND_MEAN

#define SILENCE_ALL_CXX20_DEPRECATION_WARNINGS

// STL
#include <any>
#include <map>
#include <list>
#include <array>
#include <mutex>
#include <queue>
#include <format>
#include <ranges>
#include <memory>
#include <vector>
#include <variant>
#include <crtdbg.h>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <string_view>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

// WIN
#include <cassert>
#include <windows.h>

// INCLUDE
#include <entt/entt.hpp>
using namespace entt::literals;

#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

#include <physx/PxPhysicsAPI.h>

// USER
#include <directxtk/SimpleMath.h>

using Vector2 = DirectX::SimpleMath::Vector2;
using Vector3 = DirectX::SimpleMath::Vector3;
using Vector4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;
using Quaternion = DirectX::SimpleMath::Quaternion;
using Color = DirectX::SimpleMath::Color;

#include "Macros.h"

// robin_hood
#include <robin_hood.h>

#endif //CORE_PCH_H