#ifndef PRISMOS_PSF_FONT_H
#define PRISMOS_PSF_FONT_H

#include <stdint.h>

int psf_font_init(const uint8_t* data, uint32_t size);
uint32_t psf_font_get_width(void);
uint32_t psf_font_get_height(void);
const uint8_t* psf_font_get_glyph(uint32_t codepoint);

#endif
