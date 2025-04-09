#pragma once
#include <string_view>

bool decode_image(std::string_view fname, unsigned char*& pixels, int& w, int& h);
