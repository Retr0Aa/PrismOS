#include <stdint.h>

#include "psf_font.h"
#include "font8x8.h"

#define PSF1_MAGIC0 0x36U
#define PSF1_MAGIC1 0x04U
#define PSF1_MODE_512 0x01U
#define PSF1_HEADER_SIZE 4U
#define PSF_MAX_GLYPHS 512U
#define PSF_MAX_GLYPH_HEIGHT 32U

typedef struct {
    const uint8_t* glyphs;
    uint32_t glyph_count;
    uint32_t glyph_height;
    uint32_t glyph_width;
} PsfFontState;

static PsfFontState g_font = {0};
static uint8_t g_psf_storage[PSF_MAX_GLYPHS * PSF_MAX_GLYPH_HEIGHT];

static uint8_t reverse_bits8(uint8_t value) {
    // PSF1 glyph rows are stored least-significant-bit first, while the console renderer
    // expects the leftmost pixel in the most-significant bit.
    value = (uint8_t)(((value & 0xF0U) >> 4) | ((value & 0x0FU) << 4));
    value = (uint8_t)(((value & 0xCCU) >> 2) | ((value & 0x33U) << 2));
    value = (uint8_t)(((value & 0xAAU) >> 1) | ((value & 0x55U) << 1));
    return value;
}

static void psf_font_use_builtin_fallback(void) {
    g_font.glyphs = (const uint8_t*)font8x8;
    g_font.glyph_count = 256U;
    g_font.glyph_height = 8U;
    g_font.glyph_width = 8U;
}

int psf_font_init(const uint8_t* data, uint32_t size) {
    uint32_t glyph_count;
    uint32_t glyph_height;
    uint32_t glyph_bytes;

    psf_font_use_builtin_fallback();

    if (data == 0 || size < PSF1_HEADER_SIZE) {
        return 0;
    }

    if (data[0] != PSF1_MAGIC0 || data[1] != PSF1_MAGIC1) {
        return 0;
    }

    glyph_count = (data[2] & PSF1_MODE_512) != 0U ? 512U : 256U;
    glyph_height = data[3];

    if (glyph_height == 0U || glyph_height > PSF_MAX_GLYPH_HEIGHT) {
        return 0;
    }

    glyph_bytes = glyph_count * glyph_height;
    if (size < PSF1_HEADER_SIZE + glyph_bytes) {
        return 0;
    }

    for (uint32_t index = 0; index < glyph_bytes; index++) {
        g_psf_storage[index] = reverse_bits8(data[PSF1_HEADER_SIZE + index]);
    }

    g_font.glyphs = g_psf_storage;
    g_font.glyph_count = glyph_count;
    g_font.glyph_height = glyph_height;
    g_font.glyph_width = 8U;
    return 1;
}

uint32_t psf_font_get_width(void) {
    return g_font.glyph_width;
}

uint32_t psf_font_get_height(void) {
    return g_font.glyph_height;
}

const uint8_t* psf_font_get_glyph(uint32_t codepoint) {
    if (g_font.glyphs == 0 || g_font.glyph_height == 0U || g_font.glyph_count == 0U) {
        psf_font_use_builtin_fallback();
    }

    if (codepoint >= g_font.glyph_count) {
        codepoint = (uint32_t)'?';
    }

    if (codepoint >= g_font.glyph_count) {
        codepoint = 0U;
    }

    return g_font.glyphs + (codepoint * g_font.glyph_height);
}
