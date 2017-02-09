#pragma once

#include "matrix.h"
#include "EasyBMP.h"

#include <tuple>

typedef Matrix<uint> Image;

Image load_image(const char*);
void save_image(const Image&, const char*);