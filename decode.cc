#include "decode.hh"

// --- Include necessary image format libraries ---
#include <avif/avif.h>   // For AVIF decoding
#include <webp/decode.h> // For WebP decoding

#include <cstddef>
#include <cstdint>
#include <cstring>    // For memcmp
#include <fstream>    // For reading files
#include <functional> // For std::move_only_function
#include <iostream>   // For std::cerr
#include <memory>     // For std::unique_ptr
#include <vector>     // For std::vector

// --- Include stb_image for other formats (JPEG, PNG, etc.) ---
// WHY define STB_IMAGE_IMPLEMENTATION here? Required by stb_image.h in exactly
// one .c or .cc file to create the implementation.
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h" // Path relative to this file assumed.

// Anonymous namespace limits visibility of helpers to this file only.
namespace {

// Represents the detected image file type based on header magic bytes.
enum class file_type { AVIF, WEBP, OTHER, UNKNOWN }; // Added UNKNOWN for clarity

// Detects image file type by inspecting the first few bytes (header/magic bytes).
file_type detect_file_type(const std::vector<uint8_t> &header_bytes)
{
    // WHY check size >= 12? Minimum size needed to potentially identify WebP or AVIF.
    if (header_bytes.size() >= 12) {
        // WHY memcmp for WebP? Checks for "RIFF" at offset 0 and "WEBP" at offset 8.
        if (memcmp(header_bytes.data(), "RIFF", 4) == 0 && memcmp(header_bytes.data() + 8, "WEBP", 4) == 0) {
            return file_type::WEBP;
        }

        // WHY check AVIF differently? AVIF uses ISO BMFF boxes (like MP4). Check 'ftyp' box.
        const uint8_t *data_ptr = header_bytes.data();
        // Read 32-bit box size (big-endian).
        // WHY bit shifts and OR? Standard way to construct uint32_t from big-endian bytes.
        uint32_t box_size = (static_cast<uint32_t>(data_ptr[0]) << 24) | (static_cast<uint32_t>(data_ptr[1]) << 16) |
                            (static_cast<uint32_t>(data_ptr[2]) << 8) | static_cast<uint32_t>(data_ptr[3]);

        // Check if the 'ftyp' box is present and indicates 'avif'.
        // WHY check box_size >= 16? Minimum size for an ftyp box containing 'avif'.
        // WHY check header_bytes.size() >= box_size? Ensure we have enough data to read the box.
        // WHY memcmp for AVIF? Checks for "ftyp" type at offset 4 and "avif" major brand at offset 8.
        if (box_size >= 16 && header_bytes.size() >= box_size) {
            if (memcmp(data_ptr + 4, "ftyp", 4) == 0 && memcmp(data_ptr + 8, "avif", 4) == 0) {
                return file_type::AVIF;
            }
        }
    }
    // If no specific magic bytes matched, assume it's another type (or unknown).
    return file_type::OTHER;
}

// Decodes an AVIF image buffer into RGB pixel data.
smart_pixels_ptr decode_avif(const std::vector<uint8_t> &image_buffer, int &image_width, int &image_height)
{
    // --- Setup AVIF Decoder ---
    // WHY unique_ptr with custom deleter? Ensures avifDecoderDestroy is called via RAII, even on errors.
    using decoder_ptr = std::unique_ptr<avifDecoder, decltype(&avifDecoderDestroy)>;
    decoder_ptr decoder(avifDecoderCreate(), avifDecoderDestroy);
    // WHY check decoder? Creation can fail (e.g., memory allocation error).
    if (!decoder) {
        std::cerr << "ERROR: avifDecoderCreate() failed\n";
        return {}; // Return null smart pointer.
    }

    // --- Setup AVIF Image Structure ---
    // WHY unique_ptr? Manages avifImage lifetime (holds YUV planes etc.) via RAII.
    using image_ptr = std::unique_ptr<avifImage, decltype(&avifImageDestroy)>;
    image_ptr image(avifImageCreateEmpty(), avifImageDestroy);
    // WHY check image? Creation can fail.
    if (!image) {
        std::cerr << "ERROR: avifImageCreateEmpty() failed\n";
        return {};
    }

    // --- Decode Image Header & Data ---
    // WHY check result? libavif functions return status codes; must check for OK.
    avifResult read_result = avifDecoderReadMemory(decoder.get(), image.get(), image_buffer.data(), image_buffer.size());
    if (read_result != AVIF_RESULT_OK) {
        std::cerr << "ERROR: avifDecoderReadMemory() failed: " << avifResultToString(read_result) << "\n";
        return {};
    }

    // Store output dimensions.
    image_width = image->width;
    image_height = image->height;

    // --- Prepare RGB Output Structure ---
    // WHY custom deleter lambda? avifRGBImage needs two-step cleanup: free pixels buffer, then delete struct.
    auto avif_rgb_image_deleter = [](avifRGBImage *rgb_img_ptr) {
        if (rgb_img_ptr) {
            // WHY avifRGBImageFreePixels? Frees the buffer allocated by avifRGBImageAllocatePixels.
            avifRGBImageFreePixels(rgb_img_ptr);
            // WHY delete? Frees the avifRGBImage struct itself allocated via 'new'.
            delete rgb_img_ptr;
        }
    };
    // WHY unique_ptr type with decltype? Manages avifRGBImage struct lifetime using the custom lambda deleter.
    using avif_rgb_image_ptr = std::unique_ptr<avifRGBImage, decltype(avif_rgb_image_deleter)>;

    // WHY allocate struct on heap (new)? Its lifetime is tied to the returned pixel buffer's deleter,
    // extending beyond this function's scope via the capture in the final smart_pixels_ptr deleter.
    // WHY std::nothrow? Avoids exceptions on allocation failure, returns null instead.
    avif_rgb_image_ptr rgb_image_struct(new (std::nothrow) avifRGBImage{}, avif_rgb_image_deleter);
    // WHY check rgb_image_struct? Allocation can fail.
    if (!rgb_image_struct) {
        std::cerr << "ERROR: Failed to allocate avifRGBImage struct\n";
        return {};
    }

    // Configure the RGB output format.
    avifRGBImageSetDefaults(rgb_image_struct.get(), image.get()); // Inherit color profile etc.
    rgb_image_struct->format = AVIF_RGB_FORMAT_RGB;               // Request 3-channel, 8-bit RGB.
    rgb_image_struct->depth = 8;
    // WHY static_cast? Ensure multiplication happens with sufficient width before potential overflow.
    // WHY width * 3? Stride calculation: pixels per row * channels per pixel * bytes per channel.
    rgb_image_struct->rowBytes = static_cast<uint32_t>(image_width) * 3;
    rgb_image_struct->pixels = nullptr; // Ensure null before allocation.

    // --- Allocate Pixel Buffer ---
    // WHY check result? Allocation can fail (e.g., out of memory).
    if (avifRGBImageAllocatePixels(rgb_image_struct.get()) != AVIF_RESULT_OK) {
        std::cerr << "ERROR: avifRGBImageAllocatePixels() failed\n";
        // rgb_image_struct's unique_ptr handles freeing the partially allocated struct on return.
        return {};
    }
    // WHY check pixels pointer? Paranoid check against library bugs.
    if (!rgb_image_struct->pixels) {
        std::cerr << "ERROR: avifRGBImageAllocatePixels succeeded but pixels pointer is still null.\n";
        return {};
    }

    // --- Convert YUV to RGB ---
    // WHY check result? Color conversion can fail.
    if (avifImageYUVToRGB(image.get(), rgb_image_struct.get()) != AVIF_RESULT_OK) {
        std::cerr << "ERROR: avifImageYUVToRGB() failed\n";
        // rgb_image_struct's unique_ptr handles cleanup on return.
        return {};
    }

    // --- Prepare Return Value ---
    // WHY separate pointer variable? Avoids undefined behavior: Guarantees reading `pixels` pointer *before*
    // `std::move(rgb_image_struct)` below. Direct use in constructor args has unspecified relative eval order.
    // For example, the code below segfaults when compile with g++ 14.2.1.
    // return smart_pixels_ptr(rgb_image_struct->pixels,
    //                        [rgb_owner = std::move(rgb_image_struct)](uint8_t *) {});
    uint8_t *pixel_data = rgb_image_struct->pixels;

    // WHY capture `std::move(rgb_image_struct)`? Transfers ownership of the `avifRGBImage` struct
    // and its cleanup logic (via `avif_rgb_image_deleter`) into the returned `smart_pixels_ptr`'s lifetime.
    // WHY empty deleter body `[](uint8_t*){}`? Its sole purpose is to own the captured `rgb_owner`;
    // the *destruction* of `rgb_owner` triggers the actual cleanup via `avif_rgb_image_deleter`.
    return smart_pixels_ptr(pixel_data, [rgb_owner = std::move(rgb_image_struct)](uint8_t *) { /* owns rgb_owner */ });
}

// Decodes a WebP image buffer into RGB pixel data.
smart_pixels_ptr decode_webp(const std::vector<uint8_t> &image_buffer, int &image_width, int &image_height)
{
    int temp_width{0};
    int temp_height{0};

    // WHY unique_ptr with lambda deleter? Manages buffer from WebPDecodeRGB using WebPFree via RAII.
    smart_pixels_ptr pixels{
        // WebPDecodeRGB allocates memory; returns null on failure.
        reinterpret_cast<uint8_t *>(WebPDecodeRGB(image_buffer.data(), image_buffer.size(), &temp_width, &temp_height)),
        // Deleter lambda calls the correct WebP free function.
        [](uint8_t *p) { WebPFree(p); }};

    // WHY check pixels? Decode function returns null on error.
    if (!pixels) {
        return {};
    }

    // Update output dimensions.
    image_width = temp_width;
    image_height = temp_height;
    return pixels; // Transfer ownership of the unique_ptr.
}

// Decodes other image formats (JPEG, PNG, etc.) using stb_image.
smart_pixels_ptr decode_other(const std::vector<uint8_t> &image_buffer, int &image_width, int &image_height)
{
    int temp_width{0};
    int temp_height{0};
    int channels_in_file{0}; // We don't use this but stb_image requires it.

    // WHY unique_ptr with lambda deleter? Manages buffer from stbi_load_from_memory using stbi_image_free via RAII.
    smart_pixels_ptr pixels{
        // Request 3 channels (RGB). stbi_load will convert if necessary (e.g., RGBA -> RGB). Returns null on failure.
        stbi_load_from_memory(image_buffer.data(), image_buffer.size(), &temp_width, &temp_height, &channels_in_file, 3),
        // Deleter lambda calls the correct stb_image free function.
        [](uint8_t *p) { stbi_image_free(p); }};

    // WHY check pixels? Load function returns null on error.
    if (!pixels) {
        return {};
    }

    // Update output dimensions.
    image_width = temp_width;
    image_height = temp_height;
    return pixels; // Transfer ownership.
}

} // namespace

// Decodes an image file (AVIF, WebP, or other) into an RGB pixel buffer.
// Automatically detects format and uses the appropriate decoder.
smart_pixels_ptr decode_image(std::string_view filename, int &width, int &height)
{
    // --- Read File Content ---
    // WHY ifstream? Standard C++ way to read files.
    // WHY binary | ate? Open in binary mode, start at the end (to easily get size).
    std::ifstream file_stream(std::string(filename), std::ios::binary | std::ios::ate);
    // WHY check stream? File might not exist or be readable.
    if (!file_stream) {
        std::cerr << "ERROR: Cannot open file: " << filename << "\n";
        return {};
    }

    // WHY tellg/seekg? Get file size efficiently.
    std::streamsize file_size{file_stream.tellg()};
    file_stream.seekg(0); // Go back to the beginning.

    // WHY check size? Avoid allocating huge buffer for potentially invalid size. Add a reasonable limit?
    if (file_size <= 0) {
        std::cerr << "ERROR: Invalid file size (" << file_size << ") for: " << filename << "\n";
        return {};
    }

    // WHY vector<uint8_t>? Convenient dynamic buffer to hold file content.
    std::vector<uint8_t> file_buffer(static_cast<size_t>(file_size));
    // WHY check read? Ensure the entire file content was read successfully.
    if (!file_stream.read(reinterpret_cast<char *>(file_buffer.data()), file_size)) {
        std::cerr << "ERROR: Failed to read file content: " << filename << "\n";
        return {};
    }
    // File stream is automatically closed when file_stream goes out of scope (RAII).

    // --- Detect Type and Decode ---
    file_type image_type{detect_file_type(file_buffer)};
    smart_pixels_ptr decoded_pixels;

    // WHY try AVIF first? If detected, attempt decoding.
    if (image_type == file_type::AVIF) {
        decoded_pixels = decode_avif(file_buffer, width, height);
        // WHY return early on success? Avoid trying other decoders unnecessarily.
        if (decoded_pixels)
            return decoded_pixels;
        // If AVIF decoding failed, continue to fallback (stb_image).
        std::cerr << "INFO: AVIF detected but decode failed, falling back: " << filename << "\n";
    }

    // WHY try WebP next? If detected, attempt decoding.
    if (image_type == file_type::WEBP) {
        decoded_pixels = decode_webp(file_buffer, width, height);
        if (decoded_pixels)
            return decoded_pixels;
        // If WebP decoding failed, continue to fallback.
        std::cerr << "INFO: WebP detected but decode failed, falling back: " << filename << "\n";
    }

    // --- Fallback Decoder ---
    // WHY fallback to stb_image? Handles common formats like JPEG, PNG, GIF, BMP etc.
    // It's attempted regardless of detected type if specific decoders failed or type was OTHER.
    decoded_pixels = decode_other(file_buffer, width, height);
    if (!decoded_pixels) {
        std::cerr << "ERROR: Failed to decode image using fallback (stb_image): " << filename << "\n";
        // Return the (null) pixels ptr.
    }
    return decoded_pixels;
}
