#ifndef PRISMOS_APPS_APP_LOADER_H
#define PRISMOS_APPS_APP_LOADER_H

#include <stdint.h>

#include "apps/app_format.h"

typedef struct {
    const uint8_t* image;
    uint32_t image_size;
    uint32_t entry_offset;
    prism_app_header_t header;
} app_loader_image_t;

int app_loader_load_image(const char* abs_path, app_loader_image_t* out_image);

#endif
