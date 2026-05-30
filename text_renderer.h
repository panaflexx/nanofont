
/* 
 * Header-Only Text Renderer for NanoVG Integration
 * Copyright 2019 <Andrea Cognolato>
 * Converted to C for easier integration - Fixed version
 */

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "shader.h"
#include "texture_atlas.h"  // ✅ Using new texture atlas

// Use standard OpenGL headers
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define TR_MAX_FONTS 16
#define TR_MAX_TEXTURE_ATLASES 4
#define TR_CACHE_SIZE 1024
#define TR_FONT_PIXEL_WIDTH 16
#define TR_FONT_PIXEL_HEIGHT 16
#define TR_MAX_TEXT_OBJECTS 1024

// OpenGL function pointers (same as before)
#ifndef GL_VERSION_3_0
#define GL_VERSION_3_0 1
#endif

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#endif

typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (*PFNGLBINDVERTEXARRAYSPROC)(GLuint array);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (*PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (*PFNGLVERTEXATTRIBIPOINTERPROC)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);

static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays_ptr = NULL;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray_ptr = NULL;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays_ptr = NULL;
static PFNGLGENBUFFERSPROC glGenBuffers_ptr = NULL;
static PFNGLBINDBUFFERPROC glBindBuffer_ptr = NULL;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers_ptr = NULL;
static PFNGLBUFFERDATAPROC glBufferData_ptr = NULL;
static PFNGLBUFFERSUBDATAPROC glBufferSubData_ptr = NULL;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_ptr = NULL;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_ptr = NULL;
static PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer_ptr = NULL;

#define glGenVertexArrays glGenVertexArrays_ptr
#define glBindVertexArray glBindVertexArray_ptr
#define glDeleteVertexArrays glDeleteVertexArrays_ptr
#define glGenBuffers glGenBuffers_ptr
#define glBindBuffer glBindBuffer_ptr
#define glDeleteBuffers glDeleteBuffers_ptr
#define glBufferData glBufferData_ptr
#define glBufferSubData glBufferSubData_ptr
#define glVertexAttribPointer glVertexAttribPointer_ptr
#define glEnableVertexAttribArray glEnableVertexAttribArray_ptr
#define glVertexAttribIPointer glVertexAttribIPointer_ptr

// Forward declarations
typedef struct TextRenderer TextRenderer;
typedef struct TextObject TextObject;
typedef struct RenderData RenderData;

// Text object for document-like rendering
typedef struct TextObject {
    char* text;           // Text content (null-terminated)
    float fontSize;       // Font size
    int fontFace;         // Font face index
    float x, y;           // Position
    float w, h;           // Dimensions (calculated)
    float color[4];       // RGBA color
    int lineIndex;        // Original line index for multi-line documents
    bool visible;         // Whether this text object should be rendered
} TextObject;

// Collection of text objects
typedef struct TextObjectArray {
    TextObject* objects;
    size_t count;
    size_t capacity;
} TextObjectArray;

// Render data structure (matches C++ version layout)
typedef struct RenderData {
    // Quads: each quad is 6 vertices * 4 floats (x, y, u, v)
    float* quads;           
    size_t quad_count;
    
    // Texture IDs: 6 vertices per quad * 2 uints (array_index, type)
    uint32_t* texture_ids;  
    size_t texture_id_count;
    
    // Colors: 6 vertices per quad * 4 floats (r, g, b, a)
    float* colors;          
    size_t color_count;
} RenderData;

// Face collection entry
typedef struct FaceEntry {
    FT_Face face;
    int width;
    int height;
} FaceEntry;

typedef struct FaceCollection {
    FaceEntry faces[ TR_MAX_FONTS ];
    size_t count;
    size_t capacity;
} FaceCollection;

// Font metrics structure
typedef struct FontMetrics {
    float ascender;    // Distance from baseline to top of font
    float descender;   // Distance from baseline to bottom of font (negative)
    float line_height; // Recommended line spacing
} FontMetrics;

// Text bounds structure
typedef struct TextBounds {
    float x1, y1, x2, y2;  // Bounding box coordinates
    float advance;         // Horizontal advance
} TextBounds;

// Glyph position structure
typedef struct GlyphPosition {
    const char* str;    // Pointer to character in original string
    float x;            // X position
    float min_x, max_x; // Glyph bounds
} GlyphPosition;

// Text row structure (matches NanoVG's NVGtextRow)
typedef struct TextRow {
    const char* start;    // Pointer to the start of the row
    const char* end;      // Pointer to the end of the row
    const char* next;     // Pointer to the start of the next row
    float width;          // Logical width of the row
    float min_x, max_x;   // Actual bounds of the row
} TextRow;

// Shaping cache entry
typedef struct ShapingCacheEntry {
    uint64_t hash;
    hb_codepoint_t* codepoints;
    size_t* face_indices;
    size_t count;
    bool valid;
} ShapingCacheEntry;

// Shaping cache
typedef struct ShapingCache {
    ShapingCacheEntry entries[TR_CACHE_SIZE];
    size_t size;
} ShapingCache;

// Main text renderer context - ✅ UPDATED to use TextureAtlas pointers
typedef struct TextRenderer {
    FT_Library ft_library;
    FaceCollection face_collection;
    TextureAtlas* texture_atlases[TR_MAX_TEXTURE_ATLASES];  // ✅ Now using pointers
    size_t atlas_count;
    ShapingCache shaping_cache;
    hb_buffer_t* hb_buffer;
    
    // Render state
    float view_width;
    float view_height;
    float line_height;
    
    // OpenGL state
    GLuint vao;
    GLuint vbo;
    bool gl_functions_loaded;
    
    // Shader state
    Shader* current_shader;
} TextRenderer;

// Color lookup table
static const float TR_COLOR_TABLE[8][4] = {
    {1.0f, 1.0f, 1.0f, 1.0f}, // 0: Reset (white)
    {0.0f, 0.0f, 0.0f, 1.0f}, // 30: Black
    {1.0f, 0.0f, 0.0f, 1.0f}, // 31: Red
    {0.0f, 1.0f, 0.0f, 1.0f}, // 32: Green
    {1.0f, 1.0f, 0.0f, 1.0f}, // 33: Yellow
    {0.0f, 0.0f, 1.0f, 1.0f}, // 34: Blue
    {1.0f, 0.0f, 1.0f, 1.0f}, // 35: Magenta
    {0.0f, 1.0f, 1.0f, 1.0f}  // 36: Cyan
};

// =============================================================================
// IMPLEMENTATION
// =============================================================================

// Load OpenGL function pointers
static inline bool tr_load_gl_functions(void) {
    if (glGenVertexArrays_ptr != NULL) return true;
    
    #include <GLFW/glfw3.h>
    
    glGenVertexArrays_ptr = (PFNGLGENVERTEXARRAYSPROC)glfwGetProcAddress("glGenVertexArrays");
    glBindVertexArray_ptr = (PFNGLBINDVERTEXARRAYSPROC)glfwGetProcAddress("glBindVertexArray");
    glDeleteVertexArrays_ptr = (PFNGLDELETEVERTEXARRAYSPROC)glfwGetProcAddress("glDeleteVertexArrays");
    glGenBuffers_ptr = (PFNGLGENBUFFERSPROC)glfwGetProcAddress("glGenBuffers");
    glBindBuffer_ptr = (PFNGLBINDBUFFERPROC)glfwGetProcAddress("glBindBuffer");
    glDeleteBuffers_ptr = (PFNGLDELETEBUFFERSPROC)glfwGetProcAddress("glDeleteBuffers");
    glBufferData_ptr = (PFNGLBUFFERDATAPROC)glfwGetProcAddress("glBufferData");
    glBufferSubData_ptr = (PFNGLBUFFERSUBDATAPROC)glfwGetProcAddress("glBufferSubData");
    glVertexAttribPointer_ptr = (PFNGLVERTEXATTRIBPOINTERPROC)glfwGetProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArray_ptr = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glfwGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribIPointer_ptr = (PFNGLVERTEXATTRIBIPOINTERPROC)glfwGetProcAddress("glVertexAttribIPointer");
    
    return (glGenVertexArrays_ptr && glBindVertexArray_ptr && glDeleteVertexArrays_ptr &&
            glGenBuffers_ptr && glBindBuffer_ptr && glDeleteBuffers_ptr &&
            glBufferData_ptr && glBufferSubData_ptr && glVertexAttribPointer_ptr &&
            glEnableVertexAttribArray_ptr && glVertexAttribIPointer_ptr);
}

static inline CharacterBitmap tr_render_glyph(FT_Face face, hb_codepoint_t codepoint) {
    FT_Int32 flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_LCD;

    if (FT_HAS_COLOR(face)) {
        flags |= FT_LOAD_COLOR;
    }

    if (FT_Load_Glyph(face, codepoint, flags)) {
        fprintf(stderr, "Could not load glyph with codepoint: %u\n", codepoint);
        exit(EXIT_FAILURE);
    }

    if (!FT_HAS_COLOR(face)) {
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD)) {
            fprintf(stderr, "Could not render glyph with codepoint: %u\n", codepoint);
            exit(EXIT_FAILURE);
        }
    }

    CharacterBitmap result = {0};
    size_t buffer_size;
    if (!FT_HAS_COLOR(face)) {
        buffer_size = (size_t)face->glyph->bitmap.rows * face->glyph->bitmap.width;
        result.bitmap_buffer = (unsigned char*)malloc(buffer_size);
        if (!result.bitmap_buffer) {
            fprintf(stderr, "Failed to allocate bitmap buffer\n");
            exit(EXIT_FAILURE);
        }
        result.bitmap_size = buffer_size;

        unsigned char* src = face->glyph->bitmap.buffer;
        unsigned char* dst = result.bitmap_buffer;

        for (unsigned int i = 0; i < face->glyph->bitmap.rows; i++) {
            memcpy(dst, src, face->glyph->bitmap.width);
            src += face->glyph->bitmap.pitch;
            dst += face->glyph->bitmap.width;
        }
		printf("rendered COLOR glyph\n");
    } else {
        buffer_size = (size_t)face->glyph->bitmap.rows * face->glyph->bitmap.width * 4;
        result.bitmap_buffer = (unsigned char*)malloc(buffer_size);
        if (!result.bitmap_buffer) {
            fprintf(stderr, "Failed to allocate bitmap buffer\n");
            exit(EXIT_FAILURE);
        }
        result.bitmap_size = buffer_size;

        unsigned char* src = face->glyph->bitmap.buffer;
        unsigned char* dst = result.bitmap_buffer;
        int pixel_bytes = 4;

        for (unsigned int i = 0; i < face->glyph->bitmap.rows; i++) {
            memcpy(dst, src, face->glyph->bitmap.width * pixel_bytes);
            src += face->glyph->bitmap.pitch;
            dst += face->glyph->bitmap.width * pixel_bytes;
        }
		printf("rendered MONO glyph\n");
    }

    GLsizei texture_width;
    GLsizei texture_height;
    if (FT_HAS_COLOR(face)) {
        texture_width = face->glyph->bitmap.width;
        texture_height = face->glyph->bitmap.rows;
    } else {
        texture_width = face->glyph->bitmap.width / 3;
        texture_height = face->glyph->bitmap.rows;
    }

    result.character.size[0] = texture_width;
    result.character.size[1] = texture_height;
    result.character.bearing[0] = face->glyph->bitmap_left;
    result.character.bearing[1] = face->glyph->bitmap_top;
    result.character.advance = (GLuint)face->glyph->advance.x;
    result.character.colored = (bool)FT_HAS_COLOR(face);
    // Note: texture_array_index, texture_coordinates, and texture_id will be set in the atlas insertion

    return result;
}

// Assign codepoints to faces using HarfBuzz (same as before)
static inline void tr_assign_codepoints_faces(const char* text, const FaceCollection* faces, 
                                            hb_codepoint_t** codepoints_out, size_t** face_indices_out, 
                                            size_t* count_out, hb_buffer_t* buffer) {
    if (!text || !faces || !codepoints_out || !face_indices_out || !count_out || !buffer || faces->count == 0) {
        *codepoints_out = NULL;
        *face_indices_out = NULL;
        *count_out = 0;
        return;
    }
    
    size_t text_len = strlen(text);
    if (text_len == 0) {
        *codepoints_out = NULL;
        *face_indices_out = NULL;
        *count_out = 0;
        return;
    }
    
    const uint32_t CODEPOINT_MISSING_FACE = UINT32_MAX;
    const uint32_t CODEPOINT_MISSING = UINT32_MAX;
    
    hb_feature_t features[3];
    assert(hb_feature_from_string("kern=1", -1, &features[0]));
    assert(hb_feature_from_string("liga=1", -1, &features[1]));
    assert(hb_feature_from_string("clig=1", -1, &features[2]));
    
    bool all_codepoints_have_a_face = true;
    size_t glyph_count = 0;
    hb_codepoint_t* codepoints = NULL;
    size_t* face_indices = NULL;
    
    for (size_t face_idx = 0; (face_idx < faces->count) && all_codepoints_have_a_face; face_idx++) {
        all_codepoints_have_a_face = false;
        
        // Clear buffer and add text
        hb_buffer_clear_contents(buffer);
        hb_buffer_add_utf8(buffer, text, (int)text_len, 0, -1);
        hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
        hb_buffer_set_script(buffer, HB_SCRIPT_LATIN);
        hb_buffer_set_language(buffer, hb_language_from_string("en", -1));
        
        // Create HarfBuzz font from current FreeType face
        hb_font_t* hb_font = hb_ft_font_create(faces->faces[face_idx].face, NULL);
        if (!hb_font) {
            continue;
        }
        
        // Shape the text
        hb_shape(hb_font, buffer, features, 3);
        
        // Get glyph information
        unsigned int temp_glyph_count;
        hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &temp_glyph_count);
        
        if (temp_glyph_count == 0) {
            hb_font_destroy(hb_font);
            continue;
        }
        
        // On first iteration, allocate arrays
        if (face_idx == 0) {
            glyph_count = temp_glyph_count;
            codepoints = (hb_codepoint_t*)malloc(glyph_count * sizeof(hb_codepoint_t));
            face_indices = (size_t*)malloc(glyph_count * sizeof(size_t));
            if (!codepoints || !face_indices) {
                free(codepoints);
                free(face_indices);
                hb_font_destroy(hb_font);
                *codepoints_out = NULL;
                *face_indices_out = NULL;
                *count_out = 0;
                return;
            }
            for (size_t j = 0; j < glyph_count; j++) {
                codepoints[j] = CODEPOINT_MISSING;
                face_indices[j] = CODEPOINT_MISSING_FACE;
            }
        } else if (temp_glyph_count != glyph_count) {
            printf("WARNING: glyph_count mismatch: %u vs %zu\n", temp_glyph_count, glyph_count);
            hb_font_destroy(hb_font);
            continue;
        }
        
        // Assign faces and codepoints
        for (size_t j = 0; j < glyph_count; j++) {
            hb_codepoint_t codepoint = glyph_info[j].codepoint;
            
            if (codepoint != 0 && face_indices[j] == CODEPOINT_MISSING_FACE) {
                face_indices[j] = face_idx;
                codepoints[j] = codepoint;
            }
            
            if (codepoint == 0 && codepoints[j] == CODEPOINT_MISSING) {
                all_codepoints_have_a_face = true;
            }
        }
        
        hb_font_destroy(hb_font);
    }
    
    // Handle remaining missing codepoints with replacement character
    for (size_t j = 0; j < glyph_count; j++) {
        if (face_indices[j] == CODEPOINT_MISSING_FACE && codepoints[j] == CODEPOINT_MISSING) {
            const hb_codepoint_t REPLACEMENT_CHARACTER = 0x0000FFFD;
            face_indices[j] = 0;
            codepoints[j] = FT_Get_Char_Index(faces->faces[0].face, REPLACEMENT_CHARACTER);
        }
    }
    
    *codepoints_out = codepoints;
    *face_indices_out = face_indices;
    *count_out = glyph_count;
}

// ✅ FIXED: Create text renderer - defer atlas creation
static inline TextRenderer* tr_create_renderer(void) {
    TextRenderer* renderer = (TextRenderer*)calloc(1, sizeof(TextRenderer));
    if (!renderer) return NULL;
    
    if (!tr_load_gl_functions()) {
        printf("Warning: Failed to load OpenGL functions\n");
        free(renderer);
        return NULL;
    }
    renderer->gl_functions_loaded = true;
    
    if (FT_Init_FreeType(&renderer->ft_library)) {
        free(renderer);
        return NULL;
    }
    
    FT_Library_SetLcdFilter(renderer->ft_library, FT_LCD_FILTER_DEFAULT);
    
    renderer->hb_buffer = hb_buffer_create();
    if (!renderer->hb_buffer) {
        FT_Done_FreeType(renderer->ft_library);
        free(renderer);
        return NULL;
    }
    
    // ✅ FIXED: Initialize atlas pointers to NULL - will be created later
    renderer->atlas_count = 2;
    renderer->texture_atlases[0] = NULL;
    renderer->texture_atlases[1] = NULL;
    
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    
    renderer->view_width = 1280.0f;
    renderer->view_height = 720.0f;
    renderer->line_height = 16.0f;
    
    return renderer;
}

// ✅ NEW: Initialize texture atlases after shader is created
static inline bool tr_init_atlases(TextRenderer* renderer, Shader* shader) {
    if (!renderer || !shader) return false;
    
    printf("Initializing texture atlases with shader program %u\n", shader_get_program_id(shader));
    
    renderer->texture_atlases[0] = atlas_create(512, 512, shader_get_program_id(shader), 
                                              "monochromatic_texture_array", GL_RGB8, GL_RGB, 0);
    renderer->texture_atlases[1] = atlas_create(512, 512, shader_get_program_id(shader),
                                              "colored_texture_array", GL_RGBA8, GL_RGBA, 1);
    
    if (!renderer->texture_atlases[0] || !renderer->texture_atlases[1]) {
        printf("Failed to create texture atlases\n");
        return false;
    }
    
    printf("Texture atlases initialized successfully\n");
    return true;
}

static inline int tr_load_faces(TextRenderer* renderer, const char** face_names, size_t face_count, FT_UInt kFontPixelWidth) {
	FT_UInt kFontPixelHeight = kFontPixelWidth - 1;
	int loaded_faces = 0;

    for (size_t i = 0; i < face_count; i++) {
        const char* face_name = face_names[i];
        FT_Face face;
        if (FT_New_Face(renderer->ft_library, face_name, 0, &face)) {
            fprintf(stderr, "Could not load font: %s\n", face_name);
			continue;
        }

        if (FT_HAS_COLOR(face)) {
            if (FT_Select_Size(face, 0)) {
                fprintf(stderr, "Could not request the font size (fixed): %s\n", face_name);
				continue;
            }
        } else {
            if (FT_Set_Pixel_Sizes(face, kFontPixelWidth, kFontPixelHeight)) {
                fprintf(stderr, "Could not request the font size (in pixels): %s\n", face_name);
				continue;
            }
        }

        // The face's size and bbox are populated only after set pixel
        // sizes/select size have been called
        GLsizei width, height;
        if (FT_IS_SCALABLE(face)) {
            width = FT_MulFix(face->bbox.xMax - face->bbox.xMin,
                              face->size->metrics.x_scale) >>
                    6;
            height = FT_MulFix(face->bbox.yMax - face->bbox.yMin,
                               face->size->metrics.y_scale) >>
                     6;
        } else {
            width = (face->available_sizes[0].width);
            height = (face->available_sizes[0].height);
        }

		int font_id = (int)renderer->face_collection.count;
		renderer->face_collection.faces[font_id].face = face;
		renderer->face_collection.faces[font_id].width = width;
		renderer->face_collection.faces[font_id].height = height;
		renderer->face_collection.count++;

		loaded_faces++;
    }

    return loaded_faces;
}


// Add font (same as before)
static inline int tr_add_font(TextRenderer* renderer, const char* font_path) {
    if (!renderer || !font_path || renderer->face_collection.count >= TR_MAX_FONTS) {
        return -1;
    }
    
    FT_Face face;
    if (FT_New_Face(renderer->ft_library, font_path, 0, &face)) {
        return -1;
    }
    
    if (FT_HAS_COLOR(face)) {
        if (FT_Select_Size(face, 0)) {
            FT_Done_Face(face);
            return -1;
        }
    } else {
        if (FT_Set_Pixel_Sizes(face, TR_FONT_PIXEL_WIDTH, TR_FONT_PIXEL_HEIGHT)) {
            FT_Done_Face(face);
            return -1;
        }
    }
    
    int width, height;
    if (FT_IS_SCALABLE(face)) {
        width = FT_MulFix(face->bbox.xMax - face->bbox.xMin, face->size->metrics.x_scale) >> 6;
        height = FT_MulFix(face->bbox.yMax - face->bbox.yMin, face->size->metrics.y_scale) >> 6;
    } else {
        width = face->available_sizes[0].width;
        height = face->available_sizes[0].height;
    }
    
    int font_id = (int)renderer->face_collection.count;
    renderer->face_collection.faces[font_id].face = face;
    renderer->face_collection.faces[font_id].width = width;
    renderer->face_collection.faces[font_id].height = height;
    renderer->face_collection.count++;
    
    return font_id;
}

// Get font metrics for a specific font and size
static inline FontMetrics tr_get_font_metrics(TextRenderer* renderer, int font_id, float font_size) {
    FontMetrics metrics = {0};
    
    if (!renderer || font_id < 0 || font_id >= (int)renderer->face_collection.count) {
        return metrics;
    }
    
    FT_Face face = renderer->face_collection.faces[font_id].face;
    
    // Set font size for measurements
    if (FT_HAS_COLOR(face)) {
        if (FT_Select_Size(face, 0)) {
            return metrics;
        }
    } else {
        if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)font_size)) {
            return metrics;
        }
    }
    
    // Get metrics from face
    FT_Size_Metrics size_metrics = face->size->metrics;
    
    metrics.ascender = (float)(size_metrics.ascender >> 6);
    metrics.descender = (float)(size_metrics.descender >> 6);
    metrics.line_height = (float)(size_metrics.height >> 6);
    
    return metrics;
}

// Measure single line text bounds
static inline TextBounds tr_measure_text(TextRenderer* renderer, const char* text, 
                                       int font_id, float font_size) {
    TextBounds bounds = {0};
    
    if (!renderer || !text || font_id < 0 || font_id >= (int)renderer->face_collection.count) {
        return bounds;
    }
    
    // Shape the text
    hb_codepoint_t* codepoints = NULL;
    size_t* face_indices = NULL;
    size_t glyph_count = 0;
    
    tr_assign_codepoints_faces(text, &renderer->face_collection, 
                             &codepoints, &face_indices, &glyph_count, 
                             renderer->hb_buffer);
    
    if (glyph_count == 0) {
        return bounds;
    }
    
    float min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    float x = 0;
    bool first_glyph = true;
    
    for (size_t i = 0; i < glyph_count; i++) {
        hb_codepoint_t codepoint = codepoints[i];
        size_t face_idx = face_indices[i];
        
        if (face_idx >= renderer->face_collection.count) continue;
        
        FT_Face face = renderer->face_collection.faces[face_idx].face;
        
        // Set font size
        if (FT_HAS_COLOR(face)) {
            FT_Select_Size(face, 0);
        } else {
            FT_Set_Pixel_Sizes(face, 0, (FT_UInt)font_size);
        }
        
        // Load glyph
        if (FT_Load_Glyph(face, codepoint, FT_LOAD_DEFAULT)) {
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        
        // Calculate glyph bounds
        float glyph_x1 = x + (float)glyph->bitmap_left;
        float glyph_y1 = -(float)glyph->bitmap_top;
        float glyph_x2 = glyph_x1 + (float)glyph->bitmap.width;
        float glyph_y2 = glyph_y1 + (float)glyph->bitmap.rows;
        
        if (first_glyph) {
            min_x = glyph_x1;
            min_y = glyph_y1;
            max_x = glyph_x2;
            max_y = glyph_y2;
            first_glyph = false;
        } else {
            if (glyph_x1 < min_x) min_x = glyph_x1;
            if (glyph_y1 < min_y) min_y = glyph_y1;
            if (glyph_x2 > max_x) max_x = glyph_x2;
            if (glyph_y2 > max_y) max_y = glyph_y2;
        }
        
        x += (float)(glyph->advance.x >> 6);
    }
    
    bounds.x1 = min_x;
    bounds.y1 = min_y;
    bounds.x2 = max_x;
    bounds.y2 = max_y;
    bounds.advance = x;
    
    // Clean up if not cached
    // (The codepoints might be cached, so check before freeing)
    
    return bounds;
}


// Get individual glyph positions
static inline int tr_get_glyph_positions(TextRenderer* renderer, const char* text, 
                                       int font_id, float font_size, float x, float y,
                                       GlyphPosition* positions, int max_positions) {
    if (!renderer || !text || !positions || max_positions <= 0) {
        return 0;
    }
    
    if (font_id < 0 || font_id >= (int)renderer->face_collection.count) {
        return 0;
    }
    
    // Shape the text
    hb_codepoint_t* codepoints = NULL;
    size_t* face_indices = NULL;
    size_t glyph_count = 0;
    
    tr_assign_codepoints_faces(text, &renderer->face_collection, 
                             &codepoints, &face_indices, &glyph_count, 
                             renderer->hb_buffer);
    
    if (glyph_count == 0) {
        return 0;
    }
    
    int pos_count = 0;
    float current_x = x;
    const char* str_pos = text;
    
    for (size_t i = 0; i < glyph_count && pos_count < max_positions; i++) {
        hb_codepoint_t codepoint = codepoints[i];
        size_t face_idx = face_indices[i];
        
        if (face_idx >= renderer->face_collection.count) continue;
        
        FT_Face face = renderer->face_collection.faces[face_idx].face;
        
        // Set font size
        if (FT_HAS_COLOR(face)) {
            FT_Select_Size(face, 0);
        } else {
            FT_Set_Pixel_Sizes(face, 0, (FT_UInt)font_size);
        }
        
        // Load glyph
        if (FT_Load_Glyph(face, codepoint, FT_LOAD_DEFAULT)) {
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        
        // Fill position info
        positions[pos_count].str = str_pos;
        positions[pos_count].x = current_x;
        positions[pos_count].min_x = current_x + (float)glyph->bitmap_left;
        positions[pos_count].max_x = positions[pos_count].min_x + (float)glyph->bitmap.width;
        
        current_x += (float)(glyph->advance.x >> 6);
        pos_count++;
        
        // Advance string pointer (this is approximate - you might need better UTF-8 handling)
        str_pos++;
    }
    
    return pos_count;
}

// Break text into lines with proper word wrapping
static inline int tr_break_lines(TextRenderer* renderer, const char* text, const char* end,
                               int font_id, float font_size, float break_row_width, 
                               TextRow* rows, int max_rows) {
    if (!renderer || !text || !rows || max_rows <= 0) {
        return 0;
    }
    
    if (font_id < 0 || font_id >= (int)renderer->face_collection.count) {
        return 0;
    }
    
    if (end == NULL) {
        end = text + strlen(text);
    }
    
    if (text == end) {
        return 0;
    }
    
    int nrows = 0;
    const char* row_start = NULL;
    const char* row_end = NULL;
    const char* word_start = NULL;
    const char* break_end = NULL;
    float row_width = 0;
    float row_min_x = 0, row_max_x = 0;
    float word_start_x = 0;
    float break_width = 0;
    float break_max_x = 0;
    
    const char* p = text;
    float x = 0;
    
    while (p < end && nrows < max_rows) {
        // Get current character
        unsigned int codepoint = 0;
        int char_len = 1;
        
        // Simple UTF-8 decoding (you might want to use a proper UTF-8 library)
        if ((*p & 0x80) == 0) {
            codepoint = *p;
            char_len = 1;
        } else if ((*p & 0xE0) == 0xC0) {
            if (p + 1 < end) {
                codepoint = ((*p & 0x1F) << 6) | (*(p+1) & 0x3F);
                char_len = 2;
            }
        } else if ((*p & 0xF0) == 0xE0) {
            if (p + 2 < end) {
                codepoint = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
                char_len = 3;
            }
        } else if ((*p & 0xF8) == 0xF0) {
            if (p + 3 < end) {
                codepoint = ((*p & 0x07) << 18) | ((*(p+1) & 0x3F) << 12) | ((*(p+2) & 0x3F) << 6) | (*(p+3) & 0x3F);
                char_len = 4;
            }
        }
        
        // Determine character type
        enum { CHAR_SPACE, CHAR_NEWLINE, CHAR_REGULAR } char_type = CHAR_REGULAR;
        
        if (codepoint == ' ' || codepoint == '\t' || codepoint == 0x00a0) {
            char_type = CHAR_SPACE;
        } else if (codepoint == '\n' || codepoint == '\r') {
            char_type = CHAR_NEWLINE;
        }
        
        if (char_type == CHAR_NEWLINE) {
            // Handle newline
            if (row_start != NULL) {
                rows[nrows].start = row_start;
                rows[nrows].end = row_end != NULL ? row_end : p;
                rows[nrows].width = row_width;
                rows[nrows].min_x = row_min_x;
                rows[nrows].max_x = row_max_x;
                rows[nrows].next = p + char_len;
                nrows++;
            }
            
            // Reset for next row
            row_start = NULL;
            row_end = NULL;
            row_width = 0;
            row_min_x = row_max_x = 0;
            break_end = NULL;
            break_width = 0;
            break_max_x = 0;
            x = 0;
            
        } else {
            if (row_start == NULL) {
                if (char_type == CHAR_REGULAR) {
                    // Start new row
                    row_start = p;
                    row_end = p + char_len;
                    word_start = p;
                    word_start_x = x;
                    break_end = row_start;
                    break_width = 0;
                    break_max_x = 0;
                }
            }
            
            if (row_start != NULL) {
                // Measure character width
                float char_width = 0;
                FT_Face face = renderer->face_collection.faces[font_id].face;
                
                // Set font size
                if (FT_HAS_COLOR(face)) {
                    FT_Select_Size(face, 0);
                } else {
                    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)font_size);
                }
                
                FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
                if (glyph_index && !FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
                    char_width = (float)(face->glyph->advance.x >> 6);
                }
                
                float next_x = x + char_width;
                
                // Update row bounds
                if (char_type == CHAR_REGULAR) {
                    row_end = p + char_len;
                    row_width = next_x;
                    row_max_x = next_x;
                }
                
                // Track word boundaries
                if (char_type == CHAR_SPACE && row_end != NULL) {
                    break_end = row_end;
                    break_width = row_width;
                    break_max_x = row_max_x;
                } else if (char_type == CHAR_REGULAR && (p == text || *(p-1) == ' ')) {
                    word_start = p;
                    word_start_x = x;
                }
                
                // Check for line break
                if (char_type == CHAR_REGULAR && next_x > break_row_width) {
                    if (break_end == row_start) {
                        // No good break point, break here
                        rows[nrows].start = row_start;
                        rows[nrows].end = p;
                        rows[nrows].width = x;
                        rows[nrows].min_x = 0;
                        rows[nrows].max_x = x;
                        rows[nrows].next = p;
                        nrows++;
                        
                        row_start = p;
                        row_end = p + char_len;
                        row_width = char_width;
                        row_min_x = 0;
                        row_max_x = char_width;
                        x = char_width;
                    } else {
                        // Break at word boundary
                        rows[nrows].start = row_start;
                        rows[nrows].end = break_end;
                        rows[nrows].width = break_width;
                        rows[nrows].min_x = 0;
                        rows[nrows].max_x = break_max_x;
                        rows[nrows].next = word_start;
                        nrows++;
                        
                        // Start new row from word start
                        row_start = word_start;
                        row_end = p + char_len;
                        float word_width = next_x - word_start_x;
                        row_width = word_width;
                        row_min_x = 0;
                        row_max_x = word_width;
                        x = word_width;
                    }
                    
                    break_end = NULL;
                    break_width = 0;
                    break_max_x = 0;
                } else {
                    x = next_x;
                }
            }
        }
        
        p += char_len;
    }
    
    // Handle remaining text
    if (row_start != NULL && nrows < max_rows) {
        rows[nrows].start = row_start;
        rows[nrows].end = row_end != NULL ? row_end : end;
        rows[nrows].width = row_width;
        rows[nrows].min_x = row_min_x;
        rows[nrows].max_x = row_max_x;
        rows[nrows].next = end;
        nrows++;
    }
    
    return nrows;
}


// Measure text box bounds with line wrapping
static inline TextBounds tr_measure_text_box(TextRenderer* renderer, const char* text, const char* end,
                                           int font_id, float font_size, float break_row_width) {
    TextBounds bounds = {0};
    
    if (!renderer || !text) {
        return bounds;
    }
    
    if (end == NULL) {
        end = text + strlen(text);
    }
    
    // Break into lines
    TextRow rows[256];  // Max 256 lines
    int nrows = tr_break_lines(renderer, text, end, font_id, font_size, break_row_width, rows, 256);
    
    if (nrows == 0) {
        return bounds;
    }
    
    // Get font metrics
    FontMetrics metrics = tr_get_font_metrics(renderer, font_id, font_size);
    
    float min_x = 0, max_x = 0;
    float min_y = 0, max_y = 0;
    bool first = true;
    
    for (int i = 0; i < nrows; i++) {
        float line_y = i * metrics.line_height;
        
        if (first) {
            min_x = rows[i].min_x;
            max_x = rows[i].max_x;
            min_y = line_y + metrics.descender;
            max_y = line_y + metrics.ascender;
            first = false;
        } else {
            if (rows[i].min_x < min_x) min_x = rows[i].min_x;
            if (rows[i].max_x > max_x) max_x = rows[i].max_x;
            if (line_y + metrics.descender < min_y) min_y = line_y + metrics.descender;
            if (line_y + metrics.ascender > max_y) max_y = line_y + metrics.ascender;
        }
    }
    
    bounds.x1 = min_x;
    bounds.y1 = min_y;
    bounds.x2 = max_x;
    bounds.y2 = max_y;
    bounds.advance = max_x;  // Total width
    
    return bounds;
}

// Get line bounds for baseline positioning
static inline void tr_get_line_bounds(TextRenderer* renderer, int font_id, float font_size, 
                                    float* miny, float* maxy) {
    if (!renderer || !miny || !maxy) {
        return;
    }
    
    FontMetrics metrics = tr_get_font_metrics(renderer, font_id, font_size);
    
    *miny = metrics.descender;
    *maxy = metrics.ascender;
}

// Hash function and escape code parsing (same as before)
static inline uint64_t tr_hash_string(const char* str) {
    uint64_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static inline bool tr_decode_escape_code(const char* text, size_t* pos, float color[4]) {
    size_t p = *pos;
    size_t len = strlen(text);
    
    if (p >= len || text[p] != '\033' || p + 1 >= len || text[p + 1] != '[') {
        return false;
    }
    
    size_t start = p + 2;
    size_t end = start;
    while (end < len && text[end] != 'm') {
        end++;
    }
    if (end >= len) {
        return false;
    }
    
    int code = 0;
    for (size_t i = start; i < end; ++i) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
            code = code * 10 + (c - '0');
        } else {
            return false;
        }
    }
    
    *pos = end + 1;
    
    if (code == 0 || code == 37) {
        memcpy(color, TR_COLOR_TABLE[0], sizeof(float) * 4);
        return true;
    } else if (code >= 30 && code <= 36) {
        memcpy(color, TR_COLOR_TABLE[code - 29], sizeof(float) * 4);
        return true;
    }
    return false;
}

// Parse document into text objects (same as before - keeping this for simplicity)
static inline TextObjectArray* tr_parse_document(TextRenderer* renderer, const char** lines, size_t line_count, 
                                                float start_x, float start_y, float line_height, float default_font_size) {
    if (!renderer || !lines || line_count == 0) return NULL;
    
    TextObjectArray* objects = (TextObjectArray*)malloc(sizeof(TextObjectArray));
    if (!objects) return NULL;
    
    objects->capacity = line_count * 4; // Estimate 4 text objects per line on average
    objects->objects = (TextObject*)calloc(objects->capacity, sizeof(TextObject));
    objects->count = 0;
    
    if (!objects->objects) {
        free(objects);
        return NULL;
    }
    
    float current_y = start_y;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white
    
    for (size_t line_idx = 0; line_idx < line_count; line_idx++) {
        const char* line = lines[line_idx];
        if (!line) continue;
        
        size_t line_len = strlen(line);
        if (line_len == 0) {
            current_y += line_height;
            continue;
        }
        
        float current_x = start_x;
        size_t pos = 0;
        
        while (pos < line_len) {
            // Find the next segment (until escape code or end of line)
            size_t segment_start = pos;
            size_t segment_end = pos;
            float segment_color[4];
            memcpy(segment_color, color, sizeof(float) * 4);
            
            // Build the current segment
            char* segment_text = (char*)malloc(line_len + 1);
            if (!segment_text) break;
            
            size_t segment_pos = 0;
            bool found_escape = false;
            
            while (pos < line_len && !found_escape) {
                if (line[pos] == '\t') {
                    // Replace tabs with 4 spaces
                    for (int i = 0; i < 4; i++) {
                        segment_text[segment_pos++] = ' ';
                    }
                    pos++;
                } else if (tr_decode_escape_code(line, &pos, color)) {
                    found_escape = true;
                    // If we found an escape code and have text, create a text object
                    if (segment_pos > 0) {
                        segment_text[segment_pos] = '\0';
                        
                        // Resize array if needed
                        if (objects->count >= objects->capacity) {
                            objects->capacity *= 2;
                            TextObject* new_objects = (TextObject*)realloc(objects->objects, 
                                                                         objects->capacity * sizeof(TextObject));
                            if (!new_objects) {
                                free(segment_text);
                                break;
                            }
                            objects->objects = new_objects;
                        }
                        
                        // Create text object
                        TextObject* obj = &objects->objects[objects->count++];
                        obj->text = strdup(segment_text);
                        obj->fontSize = default_font_size;
                        obj->fontFace = 0; // Default font face
                        obj->x = current_x;
                        obj->y = current_y;
                        obj->w = segment_pos * default_font_size * 0.6f; // Rough estimate
                        obj->h = default_font_size;
                        memcpy(obj->color, segment_color, sizeof(float) * 4);
                        obj->lineIndex = (int)line_idx;
                        obj->visible = true;
                        
                        current_x += obj->w;
                        segment_pos = 0; // Reset for next segment
                    }
                } else {
                    segment_text[segment_pos++] = line[pos++];
                }
            }
            
            // Handle remaining text in segment
            if (segment_pos > 0) {
                segment_text[segment_pos] = '\0';
                
                // Resize array if needed
                if (objects->count >= objects->capacity) {
                    objects->capacity *= 2;
                    TextObject* new_objects = (TextObject*)realloc(objects->objects, 
                                                                 objects->capacity * sizeof(TextObject));
                    if (!new_objects) {
                        free(segment_text);
                        break;
                    }
                    objects->objects = new_objects;
                }
                
                // Create text object
                TextObject* obj = &objects->objects[objects->count++];
                obj->text = strdup(segment_text);
                obj->fontSize = default_font_size;
                obj->fontFace = 0; // Default font face
                obj->x = current_x;
                obj->y = current_y;
                obj->w = segment_pos * default_font_size * 0.6f; // Rough estimate
                obj->h = default_font_size;
                memcpy(obj->color, segment_color, sizeof(float) * 4);
                obj->lineIndex = (int)line_idx;
                obj->visible = true;
                
                current_x += obj->w;
            }
            
            free(segment_text);
        }
        
        current_y += line_height;
    }
    
    return objects;
}

typedef struct ColorChange {
    size_t pos;
    float color[4];
} ColorChange;

static inline RenderData* tr_render_prep_lines(TextRenderer* renderer, const char** lines, size_t line_count,
                                             float start_x, float start_y, uint32_t cursor_x, uint32_t cursor_y,
                                             bool draw_cursor) {
    if (!renderer || !lines || line_count == 0) return NULL;
    
    // Check if atlases are initialized
    if (!renderer->texture_atlases[0] || !renderer->texture_atlases[1]) {
        printf("Error: Texture atlases not initialized!\n");
        return NULL;
    }
    
    RenderData* data = (RenderData*)calloc(1, sizeof(RenderData));
    if (!data) return NULL;
    
    // Temporary storage for quads
    size_t quad_capacity = 1024;
    float* temp_quads = (float*)malloc(quad_capacity * 6 * 4 * sizeof(float));
    uint32_t* temp_texture_ids = (uint32_t*)malloc(quad_capacity * 6 * 2 * sizeof(uint32_t));
    float* temp_colors = (float*)malloc(quad_capacity * 6 * 4 * sizeof(float));
    
    if (!temp_quads || !temp_texture_ids || !temp_colors) {
        free(temp_quads);
        free(temp_texture_ids);
        free(temp_colors);
        free(data);
        return NULL;
    }
    
    size_t total_quads = 0;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white

    for (size_t line_idx = 0; line_idx < line_count; line_idx++) {
        const char* line = lines[line_idx];
        if (!line) continue;
        
        // Save initial color for this line
        float line_initial_color[4];
        memcpy(line_initial_color, color, 4 * sizeof(float));
        
        // Preprocess line to handle tabs and escape codes
        char* clean_line = (char*)malloc(strlen(line) * 4 + 1);
        if (!clean_line) continue;
        
        ColorChange* color_changes = NULL;
        size_t color_change_count = 0;
        size_t color_change_capacity = 0;
        
        char* clean_pos_ptr = clean_line;
        size_t pos = 0;
        size_t line_len = strlen(line);
        
        while (pos < line_len) {
            if (line[pos] == '\t') {
                strcpy(clean_pos_ptr, "    ");
                clean_pos_ptr += 4;
                pos++;
            } else if (tr_decode_escape_code(line, &pos, color)) {
                // Add color change
                size_t current_pos = clean_pos_ptr - clean_line;
                if (color_change_count >= color_change_capacity) {
                    color_change_capacity = color_change_capacity ? color_change_capacity * 2 : 4;
                    color_changes = (ColorChange*)realloc(color_changes, color_change_capacity * sizeof(ColorChange));
                }
                color_changes[color_change_count].pos = current_pos;
                memcpy(color_changes[color_change_count].color, color, 4 * sizeof(float));
                color_change_count++;
            } else {
                *clean_pos_ptr++ = line[pos++];
            }
        }
        *clean_pos_ptr = '\0';
        
        size_t clean_len = clean_pos_ptr - clean_line;
        
        if (clean_len == 0) {
            free(clean_line);
            free(color_changes);
            continue;
        }
        
        // Shape the text using HarfBuzz
        uint64_t line_hash = tr_hash_string(clean_line);
        
        // Check shaping cache
        ShapingCacheEntry* cached_entry = NULL;
        for (size_t i = 0; i < TR_CACHE_SIZE; i++) {
            if (renderer->shaping_cache.entries[i].valid && 
                renderer->shaping_cache.entries[i].hash == line_hash) {
                cached_entry = &renderer->shaping_cache.entries[i];
                break;
            }
        }
        
        hb_codepoint_t* codepoints = NULL;
        size_t* face_indices = NULL;
        size_t glyph_count = 0;
        
        if (cached_entry) {
            codepoints = cached_entry->codepoints;
            face_indices = cached_entry->face_indices;
            glyph_count = cached_entry->count;
        } else {
            // Shape the text
            tr_assign_codepoints_faces(clean_line, &renderer->face_collection, 
                                     &codepoints, &face_indices, &glyph_count, 
                                     renderer->hb_buffer);
            
            // Cache the result
            for (size_t i = 0; i < TR_CACHE_SIZE; i++) {
                if (!renderer->shaping_cache.entries[i].valid) {
                    renderer->shaping_cache.entries[i].hash = line_hash;
                    renderer->shaping_cache.entries[i].codepoints = codepoints;
                    renderer->shaping_cache.entries[i].face_indices = face_indices;
                    renderer->shaping_cache.entries[i].count = glyph_count;
                    renderer->shaping_cache.entries[i].valid = true;
                    break;
                }
            }
        }
        
        free(clean_line);
        
        if (glyph_count == 0) {
            free(color_changes);
            continue;
        }
        
        float x = start_x;
        float y = start_y + (line_idx * renderer->line_height);
        
        // Add cursor if applicable
        if (draw_cursor && line_idx == cursor_y) {
            float cursor_x_pos = start_x;
            float cursor_width = 8.0f;
            for (size_t k = 0; k <= cursor_x && k < glyph_count; ++k) {
                if (k < glyph_count) {
                    hb_codepoint_t codepoint = codepoints[k];
                    Character* ch = atlas_get(renderer->texture_atlases[0], codepoint);
                    if (!ch) ch = atlas_get(renderer->texture_atlases[1], codepoint);
                    if (ch) {
                        uint32_t adv = ch->colored 
                            ? (uint32_t)(ch->size[0] * ((float)TR_FONT_PIXEL_WIDTH / ch->size[0])) 
                            : (ch->advance >> 6);
                        if (k == cursor_x) {
                            cursor_width = (float)adv;
                        } else {
                            cursor_x_pos += (float)adv;
                        }
                    } else {
                        if (k == cursor_x) cursor_width = 8.0f;
                        else cursor_x_pos += 8.0f;
                    }
                }
            }

            float cursor_height = renderer->line_height;
            
            // Expand storage if needed
            if (total_quads >= quad_capacity) {
                quad_capacity *= 2;
                temp_quads = (float*)realloc(temp_quads, quad_capacity * 6 * 4 * sizeof(float));
                temp_texture_ids = (uint32_t*)realloc(temp_texture_ids, quad_capacity * 6 * 2 * sizeof(uint32_t));
                temp_colors = (float*)realloc(temp_colors, quad_capacity * 6 * 4 * sizeof(float));
                
                if (!temp_quads || !temp_texture_ids || !temp_colors) {
                    free(color_changes);
                    goto cleanup_error;
                }
            }
            
            float* quad_base = &temp_quads[total_quads * 6 * 4];
            uint32_t* tex_id_base = &temp_texture_ids[total_quads * 6 * 2];
            float* color_base = &temp_colors[total_quads * 6 * 4];
            
            // Cursor quad vertices
            quad_base[0] = cursor_x_pos;     quad_base[1] = y;               quad_base[2] = 0.0f; quad_base[3] = 0.0f;
            quad_base[4] = cursor_x_pos;     quad_base[5] = y + cursor_height; quad_base[6] = 0.0f; quad_base[7] = 0.0f;
            quad_base[8] = cursor_x_pos + cursor_width; quad_base[9] = y;    quad_base[10] = 0.0f; quad_base[11] = 0.0f;
            
            quad_base[12] = cursor_x_pos;     quad_base[13] = y + cursor_height; quad_base[14] = 0.0f; quad_base[15] = 0.0f;
            quad_base[16] = cursor_x_pos + cursor_width; quad_base[17] = y;   quad_base[18] = 0.0f; quad_base[19] = 0.0f;
            quad_base[20] = cursor_x_pos + cursor_width; quad_base[21] = y + cursor_height; quad_base[22] = 0.0f; quad_base[23] = 0.0f;
            
            // Cursor texture ID {0, 2} and red color
            float cursor_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
            uint32_t cursor_texture_array_idx = 0;
            uint32_t cursor_texture_type = 2;
            
            for (int v = 0; v < 6; v++) {
                tex_id_base[v * 2] = cursor_texture_array_idx;
                tex_id_base[v * 2 + 1] = cursor_texture_type;
                
                color_base[v * 4 + 0] = cursor_color[0];
                color_base[v * 4 + 1] = cursor_color[1];
                color_base[v * 4 + 2] = cursor_color[2];
                color_base[v * 4 + 3] = cursor_color[3];
            }
            
            total_quads++;
        }
        
        // Process each glyph
        size_t i = 0;
        size_t color_index = 0;
        float current_color[4];
        memcpy(current_color, line_initial_color, 4 * sizeof(float));
       
        while (i < glyph_count) {
            // Collect characters for this batch
            Character characters[256]; // Batch buffer
            size_t batch_size = 0;
            
            for (; i < glyph_count && batch_size < 256; ++i) {
                // Apply color change if applicable
                if (color_index < color_change_count && i == color_changes[color_index].pos) {
                    memcpy(current_color, color_changes[color_index].color, 4 * sizeof(float));
                    color_index++;
                }
                
                hb_codepoint_t codepoint = codepoints[i];
                size_t face_idx = face_indices[i];
                
                Character* ch = atlas_get(renderer->texture_atlases[0], codepoint);
                if (!ch) {
                    ch = atlas_get(renderer->texture_atlases[1], codepoint);
                }
                
                if (ch) {
                    characters[batch_size++] = *ch;
                    continue;
                }
                
                if ((atlas_contains_stale(renderer->texture_atlases[0]) || !atlas_is_full(renderer->texture_atlases[0])) &&
                    (atlas_contains_stale(renderer->texture_atlases[1]) || !atlas_is_full(renderer->texture_atlases[1]))) {
                    
                    if (face_idx < renderer->face_collection.count) {
                        CharacterBitmap char_bitmap = tr_render_glyph(renderer->face_collection.faces[face_idx].face, codepoint);
                        
                        if (char_bitmap.character.colored) {
                            atlas_insert(renderer->texture_atlases[1], codepoint, &char_bitmap);
                        } else {
                            atlas_insert(renderer->texture_atlases[0], codepoint, &char_bitmap);
                        }
                        
                        characters[batch_size++] = char_bitmap.character;
                        
                        if (char_bitmap.bitmap_buffer) {
                            free(char_bitmap.bitmap_buffer);
                        }
                    } else {
						printf("ERROR: face_idx out of range %ld\n", face_idx);
					}
                } else {
					printf("ERROR: No space in atlases\n");
                    break; // No space in atlases
                }
            }
            
            // Render the batch
            for (size_t j = 0; j < batch_size; j++) {
                Character* ch = &characters[j];
                
                // Expand storage if needed
                if (total_quads >= quad_capacity) {
                    quad_capacity *= 2;
                    temp_quads = (float*)realloc(temp_quads, quad_capacity * 6 * 4 * sizeof(float));
                    temp_texture_ids = (uint32_t*)realloc(temp_texture_ids, quad_capacity * 6 * 2 * sizeof(uint32_t));
                    temp_colors = (float*)realloc(temp_colors, quad_capacity * 6 * 4 * sizeof(float));
                    
                    if (!temp_quads || !temp_texture_ids || !temp_colors) {
                        free(color_changes);
                        goto cleanup_error;
                    }
                }
                
                // Adjust bearing for flipped rendering
                int bearing_y = ch->size[1] - ch->bearing[1];
                
                // Different positioning for colored vs monochromatic
                float w, h, xpos, ypos;
                uint32_t advance;
                
                if (ch->colored) {
                    float ratio_x = (float)TR_FONT_PIXEL_WIDTH / (float)ch->size[0];
                    float ratio_y = (float)TR_FONT_PIXEL_HEIGHT / (float)ch->size[1];
                    w = ch->size[0] * ratio_x;
                    h = ch->size[1] * ratio_y;
                    xpos = x + ch->bearing[0] * ratio_x;
                    ypos = y - (ch->size[1] - bearing_y) * ratio_y;
                    advance = (uint32_t)w;
                } else {
                    w = (float)ch->size[0];
                    h = (float)ch->size[1];
                    xpos = x + (float)ch->bearing[0];
                    ypos = y - (float)(ch->size[1] - bearing_y);
                    advance = (ch->advance >> 6);
                }
                x += advance;
                
                float* quad_base = &temp_quads[total_quads * 6 * 4];
                uint32_t* tex_id_base = &temp_texture_ids[total_quads * 6 * 2];
                float* color_base = &temp_colors[total_quads * 6 * 4];
                
                float tc_x = ch->texture_coordinates[0];
                float tc_y = ch->texture_coordinates[1];
                
                // Quad vertices with standard texcoords for flipped bitmap
                quad_base[0] = xpos;     quad_base[1] = ypos;     quad_base[2] = 0.0f; quad_base[3] = 0.0f;
                quad_base[4] = xpos;     quad_base[5] = ypos + h; quad_base[6] = 0.0f; quad_base[7] = tc_y;
                quad_base[8] = xpos + w; quad_base[9] = ypos;     quad_base[10] = tc_x; quad_base[11] = 0.0f;
                
                quad_base[12] = xpos;     quad_base[13] = ypos + h; quad_base[14] = 0.0f; quad_base[15] = tc_y;
                quad_base[16] = xpos + w; quad_base[17] = ypos;     quad_base[18] = tc_x; quad_base[19] = 0.0f;
                quad_base[20] = xpos + w; quad_base[21] = ypos + h; quad_base[22] = tc_x; quad_base[23] = tc_y;
                
                // Texture ID
                uint32_t texture_array_idx = (uint32_t)ch->texture_array_index;
                uint32_t texture_type = ch->colored ? 1 : 0;
                
                for (int v = 0; v < 6; v++) {
                    tex_id_base[v * 2] = texture_array_idx;
                    tex_id_base[v * 2 + 1] = texture_type;
                    
                    color_base[v * 4 + 0] = current_color[0];
                    color_base[v * 4 + 1] = current_color[1];
                    color_base[v * 4 + 2] = current_color[2];
                    color_base[v * 4 + 3] = current_color[3];
                }
                
                total_quads++;
            }
        }
        
        free(color_changes);
    }
    
    // Copy data to final arrays
    if (total_quads > 0) {
        data->quad_count = total_quads;
        data->texture_id_count = total_quads * 6;
        data->color_count = total_quads * 6;
        
        data->quads = (float*)malloc(total_quads * 6 * 4 * sizeof(float));
        data->texture_ids = (uint32_t*)malloc(total_quads * 6 * 2 * sizeof(uint32_t));
        data->colors = (float*)malloc(total_quads * 6 * 4 * sizeof(float));
        
        if (data->quads && data->texture_ids && data->colors) {
            memcpy(data->quads, temp_quads, total_quads * 6 * 4 * sizeof(float));
            memcpy(data->texture_ids, temp_texture_ids, total_quads * 6 * 2 * sizeof(uint32_t));
            memcpy(data->colors, temp_colors, total_quads * 6 * 4 * sizeof(float));
        } else {
            goto cleanup_error;
        }
    }
    
    free(temp_quads);
    free(temp_texture_ids);
    free(temp_colors);
    return data;
    
cleanup_error:
    free(temp_quads);
    free(temp_texture_ids);
    free(temp_colors);
    free(data->quads);
    free(data->texture_ids);
    free(data->colors);
    free(data);
    return NULL;
}

static inline RenderData* tr_render_prep_lines_old(TextRenderer* renderer, const char** lines, size_t line_count,
                                             float start_x, float start_y, uint32_t cursor_x, uint32_t cursor_y,
                                             bool draw_cursor) {
    if (!renderer || !lines || line_count == 0) return NULL;
    
    // Check if atlases are initialized
    if (!renderer->texture_atlases[0] || !renderer->texture_atlases[1]) {
        printf("Error: Texture atlases not initialized!\n");
        return NULL;
    }
    
    RenderData* data = (RenderData*)calloc(1, sizeof(RenderData));
    if (!data) return NULL;
    
    // Temporary storage for quads
    size_t quad_capacity = 1024;
    float* temp_quads = (float*)malloc(quad_capacity * 6 * 4 * sizeof(float));
    uint32_t* temp_texture_ids = (uint32_t*)malloc(quad_capacity * 6 * 2 * sizeof(uint32_t));
    float* temp_colors = (float*)malloc(quad_capacity * 6 * 4 * sizeof(float));
    
    if (!temp_quads || !temp_texture_ids || !temp_colors) {
        free(temp_quads);
        free(temp_texture_ids);
        free(temp_colors);
        free(data);
        return NULL;
    }
    
    size_t total_quads = 0;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white
    
    for (size_t line_idx = 0; line_idx < line_count; line_idx++) {
        const char* line = lines[line_idx];
        if (!line) continue;
        
        // Preprocess line to handle tabs and escape codes
        char* clean_line = (char*)malloc(strlen(line) * 4 + 1);
        if (!clean_line) continue;
        
        size_t clean_pos = 0;
        size_t pos = 0;
        size_t line_len = strlen(line);
        
        while (pos < line_len) {
            if (line[pos] == '\t') {
                strcpy(&clean_line[clean_pos], "    ");
                clean_pos += 4;
                pos++;
            } else if (tr_decode_escape_code(line, &pos, color)) {
                // Color changed, continue processing
            } else {
                clean_line[clean_pos++] = line[pos++];
            }
        }
        clean_line[clean_pos] = '\0';
        
        if (clean_pos == 0) {
            free(clean_line);
            continue;
        }
        
        // Shape the text using HarfBuzz
        uint64_t line_hash = tr_hash_string(clean_line);
        
        // Check shaping cache
        ShapingCacheEntry* cached_entry = NULL;
        for (size_t i = 0; i < TR_CACHE_SIZE; i++) {
            if (renderer->shaping_cache.entries[i].valid && 
                renderer->shaping_cache.entries[i].hash == line_hash) {
                cached_entry = &renderer->shaping_cache.entries[i];
                break;
            }
        }
        
        hb_codepoint_t* codepoints = NULL;
        size_t* face_indices = NULL;
        size_t glyph_count = 0;
        
        if (cached_entry) {
            codepoints = cached_entry->codepoints;
            face_indices = cached_entry->face_indices;
            glyph_count = cached_entry->count;
        } else {
            // Shape the text
            tr_assign_codepoints_faces(clean_line, &renderer->face_collection, 
                                     &codepoints, &face_indices, &glyph_count, 
                                     renderer->hb_buffer);
            
            // Cache the result
            for (size_t i = 0; i < TR_CACHE_SIZE; i++) {
                if (!renderer->shaping_cache.entries[i].valid) {
                    renderer->shaping_cache.entries[i].hash = line_hash;
                    renderer->shaping_cache.entries[i].codepoints = codepoints;
                    renderer->shaping_cache.entries[i].face_indices = face_indices;
                    renderer->shaping_cache.entries[i].count = glyph_count;
                    renderer->shaping_cache.entries[i].valid = true;
                    break;
                }
            }
        }
        
        if (glyph_count == 0) {
            free(clean_line);
            continue;
        }
        
        float x = start_x;
        float y = start_y + (line_idx * renderer->line_height);
        
        // Process each glyph 
        size_t i = 0;
        while (i < glyph_count) {
            // Collect characters for this batch
            Character characters[256]; // Batch buffer
            size_t batch_size = 0;
            
            for (; i < glyph_count && batch_size < 256; ++i) {
                hb_codepoint_t codepoint = codepoints[i];
                size_t face_idx = face_indices[i];
                
                Character* ch = atlas_get(renderer->texture_atlases[0], codepoint);
                if (!ch) {
                    ch = atlas_get(renderer->texture_atlases[1], codepoint);
                }
                
                if (ch) {
                    characters[batch_size++] = *ch;
                    continue;
                }
                
                if ((atlas_contains_stale(renderer->texture_atlases[0]) || !atlas_is_full(renderer->texture_atlases[0])) &&
                    (atlas_contains_stale(renderer->texture_atlases[1]) || !atlas_is_full(renderer->texture_atlases[1]))) {
                    
                    if (face_idx < renderer->face_collection.count) {
                        CharacterBitmap char_bitmap = tr_render_glyph(renderer->face_collection.faces[face_idx].face, codepoint);
                        
                        // ✅ EXACTLY like C++: Insert based on colored flag
                        if (char_bitmap.character.colored) {
                            atlas_insert(renderer->texture_atlases[1], codepoint, &char_bitmap);
                        } else {
                            atlas_insert(renderer->texture_atlases[0], codepoint, &char_bitmap);
                        }
                        
                        characters[batch_size++] = char_bitmap.character;
                        
                        if (char_bitmap.bitmap_buffer) {
                            free(char_bitmap.bitmap_buffer);
                        }
                    }
                } else {
                    break; // No space in atlases
                }
            }
            
            // Render the batch
            for (size_t j = 0; j < batch_size; ++j) {
                Character* ch = &characters[j];
                
                // Expand storage if needed
                if (total_quads >= quad_capacity) {
                    quad_capacity *= 2;
                    temp_quads = (float*)realloc(temp_quads, quad_capacity * 6 * 4 * sizeof(float));
                    temp_texture_ids = (uint32_t*)realloc(temp_texture_ids, quad_capacity * 6 * 2 * sizeof(uint32_t));
                    temp_colors = (float*)realloc(temp_colors, quad_capacity * 6 * 4 * sizeof(float));
                    
                    if (!temp_quads || !temp_texture_ids || !temp_colors) {
                        free(clean_line);
                        goto cleanup_error;
                    }
                }
                
                // ✅ EXACTLY like C++: Different positioning for colored vs monochromatic
                float w, h, xpos, ypos;
                uint32_t advance;
                
                if (ch->colored) {
                    // Colored character logic (matches C++)
                    float ratio_x = (float)TR_FONT_PIXEL_WIDTH / (float)ch->size[0];
                    float ratio_y = (float)TR_FONT_PIXEL_HEIGHT / (float)ch->size[1];
                    w = ch->size[0] * ratio_x;
                    h = ch->size[1] * ratio_y;
                    xpos = x + ch->bearing[0] * ratio_x;
                    ypos = y - (ch->size[1] - ch->bearing[1]) * ratio_y;
                    advance = (uint32_t)w;
                } else {
                    // Monochromatic character logic (matches C++)
                    w = (float)ch->size[0];
                    h = (float)ch->size[1];
                    xpos = x + (float)ch->bearing[0];
                    ypos = y - (float)(ch->size[1] - ch->bearing[1]);
                    advance = (ch->advance >> 6);
                }
                x += advance;
                
                // ✅ EXACTLY like C++: Quad generation with proper texture coordinates
                float* quad_base = &temp_quads[total_quads * 6 * 4];
                uint32_t* tex_id_base = &temp_texture_ids[total_quads * 6 * 2];
                float* color_base = &temp_colors[total_quads * 6 * 4];
                
                float tc_x = ch->texture_coordinates[0]; // matches C++ tc.x
                float tc_y = ch->texture_coordinates[1]; // matches C++ tc.y
                
                // ✅ EXACTLY like C++: Quad vertices
                // {xpos, ypos, 0, tc.y},
                // {xpos, ypos + h, 0, 0},
                // {xpos + w, ypos, tc.x, tc.y},
                // {xpos, ypos + h, 0, 0},
                // {xpos + w, ypos, tc.x, tc.y},
                // {xpos + w, ypos + h, tc.x, 0}
                
                // Triangle 1
                quad_base[0] = xpos;     quad_base[1] = ypos;     quad_base[2] = 0.0f; quad_base[3] = tc_y;
                quad_base[4] = xpos;     quad_base[5] = ypos + h; quad_base[6] = 0.0f; quad_base[7] = 0.0f;
                quad_base[8] = xpos + w; quad_base[9] = ypos;     quad_base[10] = tc_x; quad_base[11] = tc_y;
                
                // Triangle 2  
                quad_base[12] = xpos;     quad_base[13] = ypos + h; quad_base[14] = 0.0f; quad_base[15] = 0.0f;
                quad_base[16] = xpos + w; quad_base[17] = ypos;     quad_base[18] = tc_x; quad_base[19] = tc_y;
                quad_base[20] = xpos + w; quad_base[21] = ypos + h; quad_base[22] = tc_x; quad_base[23] = 0.0f;
                
                // ✅ EXACTLY like C++: Texture ID assignment
                // {ch.texture_array_index, ch.colored}
                uint32_t texture_array_idx = (uint32_t)ch->texture_array_index;
                uint32_t texture_type = ch->colored ? 1 : 0; // boolean to int
                
                for (int v = 0; v < 6; v++) {
                    tex_id_base[v * 2] = texture_array_idx;
                    tex_id_base[v * 2 + 1] = texture_type;
                    
                    color_base[v * 4] = color[0];
                    color_base[v * 4 + 1] = color[1];
                    color_base[v * 4 + 2] = color[2];
                    color_base[v * 4 + 3] = color[3];
                }
                
                total_quads++;
            }
        }
        
        free(clean_line);
    }
    
    // Copy data to final arrays
    if (total_quads > 0) {
        data->quad_count = total_quads;
        data->texture_id_count = total_quads * 6;
        data->color_count = total_quads * 6;
        
        data->quads = (float*)malloc(total_quads * 6 * 4 * sizeof(float));
        data->texture_ids = (uint32_t*)malloc(total_quads * 6 * 2 * sizeof(uint32_t));
        data->colors = (float*)malloc(total_quads * 6 * 4 * sizeof(float));
        
        if (data->quads && data->texture_ids && data->colors) {
            memcpy(data->quads, temp_quads, total_quads * 6 * 4 * sizeof(float));
            memcpy(data->texture_ids, temp_texture_ids, total_quads * 6 * 2 * sizeof(uint32_t));
            memcpy(data->colors, temp_colors, total_quads * 6 * 4 * sizeof(float));
        } else {
            goto cleanup_error;
        }
    }
    
    free(temp_quads);
    free(temp_texture_ids);
    free(temp_colors);
    return data;
    
cleanup_error:
    free(temp_quads);
    free(temp_texture_ids);
    free(temp_colors);
    free(data->quads);
    free(data->texture_ids);
    free(data->colors);
    free(data);
    return NULL;
}


// Render function (must call tr_render_prep_lines first to generate the RenderData)
static inline void tr_render(TextRenderer* renderer, const RenderData* render_data, Shader* shader) {
    if (!renderer || !render_data || render_data->quad_count == 0) return;
    if (!renderer->gl_functions_loaded) return;
    if (!shader || !shader_is_valid(shader)) {
        printf("Error: Invalid shader provided to tr_render\n");
        return;
    }
    
    // ✅ EXACTLY like C++: Activate the shader
    shader_use(shader);
    renderer->current_shader = shader;
    
    // Set up projection matrix (orthographic)
    float projection[16];
    float left = 0.0f;
    float right = renderer->view_width;
    float bottom = renderer->view_height;
    float top = 0.0f;
    float near_plane = -1.0f;
    float far_plane = 1.0f;
    
    projection[0] = 2.0f / (right - left);
    projection[1] = 0.0f;
    projection[2] = 0.0f;
    projection[3] = 0.0f;
    
    projection[4] = 0.0f;
    projection[5] = 2.0f / (top - bottom);
    projection[6] = 0.0f;
    projection[7] = 0.0f;
    
    projection[8] = 0.0f;
    projection[9] = 0.0f;
    projection[10] = -2.0f / (far_plane - near_plane);
    projection[11] = 0.0f;
    
    projection[12] = -(right + left) / (right - left);
    projection[13] = -(top + bottom) / (top - bottom);
    projection[14] = -(far_plane + near_plane) / (far_plane - near_plane);
    projection[15] = 1.0f;
    
    shader_set_matrix4fv(shader, "projection", projection);
    
    glBindVertexArray(renderer->vao);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas_get_texture(renderer->texture_atlases[0]));
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas_get_texture(renderer->texture_atlases[1]));
    
    shader_set_1i(shader, "monochromatic_texture_array", 0);
    shader_set_1i(shader, "colored_texture_array", 1);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    size_t quad_size = render_data->quad_count * 6 * 4 * sizeof(float);
    size_t texture_size = render_data->texture_id_count * 2 * sizeof(uint32_t);
    size_t color_size = render_data->color_count * 4 * sizeof(float);
    size_t total_size = quad_size + texture_size + color_size;
    
    glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, quad_size, render_data->quads);
    glBufferSubData(GL_ARRAY_BUFFER, quad_size, texture_size, render_data->texture_ids);
    glBufferSubData(GL_ARRAY_BUFFER, quad_size + texture_size, color_size, render_data->colors);
    
    // Position and texture coordinates
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture IDs (to select between monochromatic and colored atlas)
    glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (void*)quad_size);
    glEnableVertexAttribArray(1);
    
    // Colors
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(quad_size + texture_size));
    glEnableVertexAttribArray(2);
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(render_data->quad_count * 6));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0); // Reset to default texture unit
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    for (size_t i = 0; i < renderer->atlas_count; i++) {
        if (renderer->texture_atlases[i]) {
            atlas_invalidate(renderer->texture_atlases[i]);
        }
    }
}

// ✅ FIXED: Destroy text renderer - use atlas_destroy
static inline void tr_destroy_renderer(TextRenderer* renderer) {
    if (!renderer) return;
    
    printf("Cleaning up text renderer...\n");
    
    // Clean up faces
    for (size_t i = 0; i < renderer->face_collection.count; i++) {
        if (renderer->face_collection.faces[i].face) {
            printf("Cleaning up face %zu\n", i);
            FT_Done_Face(renderer->face_collection.faces[i].face);
        }
    }
    
    // ✅ UPDATED: Clean up texture atlases using atlas_destroy
    for (size_t i = 0; i < renderer->atlas_count; i++) {
        if (renderer->texture_atlases[i]) {
            printf("Cleaning up texture atlas %zu\n", i);
            atlas_destroy(renderer->texture_atlases[i]);
            renderer->texture_atlases[i] = NULL;
        }
    }
    
    // Clean up shaping cache
    printf("Cleaning up shaping cache...\n");
    for (size_t i = 0; i < TR_CACHE_SIZE; i++) {
        if (renderer->shaping_cache.entries[i].valid) {
            if (renderer->shaping_cache.entries[i].codepoints) {
                free(renderer->shaping_cache.entries[i].codepoints);
            }
            if (renderer->shaping_cache.entries[i].face_indices) {
                free(renderer->shaping_cache.entries[i].face_indices);
            }
        }
    }
    
    // Clean up HarfBuzz and FreeType
    printf("Cleaning up HarfBuzz and FreeType...\n");
    if (renderer->hb_buffer) {
        hb_buffer_destroy(renderer->hb_buffer);
    }
    if (renderer->ft_library) {
        FT_Done_FreeType(renderer->ft_library);
    }
    
    // Clean up OpenGL objects
    if (renderer->gl_functions_loaded) {
        printf("Cleaning up OpenGL objects...\n");
        if (renderer->vbo != 0) {
            glDeleteBuffers(1, &renderer->vbo);
        }
        if (renderer->vao != 0) {
            glDeleteVertexArrays(1, &renderer->vao);
        }
    }
    
    printf("Freeing renderer structure...\n");
    free(renderer);
    printf("Cleanup complete.\n");
}

// Free render data (same as before)
static inline void tr_free_render_data(RenderData* data) {
    if (!data) return;
    
    free(data->quads);
    free(data->texture_ids);
    free(data->colors);
    free(data);
}

// Free text objects (same as before)
static inline void tr_free_text_objects(TextObjectArray* objects) {
    if (!objects) return;
    
    for (size_t i = 0; i < objects->count; i++) {
        if (objects->objects[i].text) {
            free(objects->objects[i].text);
        }
    }
    
    if (objects->objects) {
        free(objects->objects);
    }
    
    free(objects);
}

// Set view size helper (same as before)
static inline void tr_set_view_size(TextRenderer* renderer, float width, float height) {
    if (!renderer) return;
    renderer->view_width = width;
    renderer->view_height = height;
}

#ifdef __cplusplus
}
#endif

#endif // TEXT_RENDERER_H
