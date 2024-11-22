#pragma once

#define MAKE_NVP(value) cereal::make_nvp(#value, data.##value)