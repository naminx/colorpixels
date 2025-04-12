#pragma once
#include <functional>
#include <memory>
#include <string_view>

// Type: unique_ptr + deleter (function pointer or lambda)
using SmartPixelsPtr =
    std::unique_ptr<uint8_t[], std::function<void(uint8_t *)>>;

SmartPixelsPtr decode_image(std::string_view fname, int &width, int &height);
