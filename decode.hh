#pragma once
#include <cstdint>     // WHY: For uint8_t type.
#include <functional>  // WHY: For std::move_only_function needed by smart pointer type.
#include <memory>      // WHY: For std::unique_ptr.
#include <string_view> // WHY: Efficiently pass filename without copying string data.

// Type alias for a smart pointer managing the raw pixel buffer (uint8_t array).
// - `std::unique_ptr<uint8_t[]...>`: Owns an array allocated with new uint8_t[...] or compatible C allocator.
// - `std::move_only_function<void(uint8_t *)>`: Custom deleter, stores *any* callable
//   (lambda, function pointer) that takes uint8_t* and returns void, suitable for different
//   library free functions (e.g., WebPFree, stbi_image_free, custom lambda for AVIF).
//   `move_only` is efficient as the deleter itself doesn't need to be copied.
using smart_pixels_ptr = std::unique_ptr<uint8_t[], std::move_only_function<void(uint8_t *)>>;

// Decodes an image file specified by filename into an RGB pixel buffer.
// Automatically detects format (AVIF, WebP, Other) and calls the appropriate decoder.
// Returns a smart pointer managing the pixel buffer, or a null smart pointer on failure.
// Updates width and height output parameters on success.
smart_pixels_ptr decode_image(std::string_view filename, int &width, int &height);
