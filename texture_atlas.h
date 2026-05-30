/* 
 * Texture Atlas - C Header-Only Version
 */

#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
    #include <GL/glext.h>
	// Force definition
	GLAPI void APIENTRY glTexStorage3D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include <harfbuzz/hb.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define ATLAS_TEXTURE_DEPTH 1024
#define ATLAS_MIP_LEVEL_COUNT 1


// Character struct (converted from C++ with glm types)
typedef struct Character {
    size_t texture_array_index;
    float texture_coordinates[2];  // u, v (was glm::vec2)
    size_t texture_id;
    int size[2];                   // width, height (was glm::ivec2) 
    int bearing[2];                // bearing x, y (was glm::ivec2)
    uint32_t advance;
    bool colored;
} Character;

// Cache element (was private struct in C++)
typedef struct CacheElement {
    Character character;
    bool fresh;
    bool valid;  // whether this slot is occupied
} CacheElement;

// Character + bitmap pair (replaces std::pair<Character, std::vector<unsigned char>>)
typedef struct CharacterBitmap {
    Character character;
    unsigned char* bitmap_buffer;
    size_t bitmap_size;
} CharacterBitmap;

// TextureAtlas struct (converted from C++ class)
typedef struct TextureAtlas {
    uint32_t index;                     // current insertion index
    GLuint texture;                     // OpenGL texture ID
    int texture_width, texture_height;  // texture dimensions
    
    // Cache (replaces unordered_map)
    CacheElement cache[ATLAS_TEXTURE_DEPTH];
    hb_codepoint_t cache_keys[ATLAS_TEXTURE_DEPTH];
    size_t cache_size;                  // current number of cached elements
    
    GLenum format;                      // texture format
} TextureAtlas;

// =============================================================================
// IMPLEMENTATION
// =============================================================================

// Create texture atlas (replaces constructor)
static inline TextureAtlas* atlas_create(int texture_width, int texture_height,
                                        uint32_t shader_program_id, const char* texture_uniform_location,
                                        GLenum internal_format, GLenum format,
                                        int shader_texture_index) {
    TextureAtlas* atlas = (TextureAtlas*)calloc(1, sizeof(TextureAtlas));
    if (!atlas) return NULL;
    
    atlas->texture_width = texture_width;
    atlas->texture_height = texture_height;
    atlas->format = format;
    atlas->index = 0;
    atlas->cache_size = 0;
    
    // Initialize cache
    for (size_t i = 0; i < ATLAS_TEXTURE_DEPTH; i++) {
        atlas->cache[i].valid = false;
        atlas->cache[i].fresh = false;
    }
    
    // Create OpenGL texture
    glUseProgram(shader_program_id);
    glGenTextures(1, &atlas->texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->texture);
    
    // Use glTexStorage3D for proper texture array allocation (modern approach)
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, ATLAS_MIP_LEVEL_COUNT, internal_format,
                   texture_width, texture_height, ATLAS_TEXTURE_DEPTH);
                   
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    // Set shader uniform
    glUniform1i(glGetUniformLocation(shader_program_id, texture_uniform_location), shader_texture_index);
    
    printf("Created texture atlas: %dx%d, format=%d, texture_id=%u\n", 
           texture_width, texture_height, format, atlas->texture);
    
    return atlas;
}

// Destroy texture atlas (replaces destructor)
static inline void atlas_destroy(TextureAtlas* atlas) {
    if (!atlas) return;
    glDeleteTextures(1, &atlas->texture);
    free(atlas);
}

// Find cache entry by codepoint (replaces unordered_map lookup)
static inline int atlas_find_cache_entry(const TextureAtlas* atlas, hb_codepoint_t codepoint) {
    for (size_t i = 0; i < ATLAS_TEXTURE_DEPTH; i++) {
        if (atlas->cache[i].valid && atlas->cache_keys[i] == codepoint) {
            return (int)i;
        }
    }
    return -1;
}

// Find first empty cache slot
static inline int atlas_find_empty_slot(const TextureAtlas* atlas) {
    for (size_t i = 0; i < ATLAS_TEXTURE_DEPTH; i++) {
        if (!atlas->cache[i].valid) {
            return (int)i;
        }
    }
    return -1;
}

// Find first stale cache entry
static inline int atlas_find_stale_entry(const TextureAtlas* atlas) {
    for (size_t i = 0; i < ATLAS_TEXTURE_DEPTH; i++) {
        if (atlas->cache[i].valid && !atlas->cache[i].fresh) {
            return (int)i;
        }
    }
    return -1;
}

// Insert bitmap into texture (replaces private Insert method)
static inline void atlas_insert_bitmap(TextureAtlas* atlas, const unsigned char* bitmap_buffer,
                                      int width, int height, Character* ch, uint32_t offset) {
    assert(offset < ATLAS_TEXTURE_DEPTH);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->texture);
    
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, offset, 
                   width, height, 1, atlas->format, GL_UNSIGNED_BYTE, bitmap_buffer);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    // Each glyph gets its own layer, so coordinates are just the size ratios
    ch->texture_coordinates[0] = (float)width / (float)atlas->texture_width;
    ch->texture_coordinates[1] = (float)height / (float)atlas->texture_height;
    ch->texture_id = atlas->texture;
    ch->texture_array_index = offset;
    
    printf("Inserted glyph bitmap: %dx%d at layer %u, tex_coords=(%.3f,%.3f)\n", 
           width, height, offset, ch->texture_coordinates[0], ch->texture_coordinates[1]);
}

// Append new glyph (replaces Append method)
static inline void atlas_append(TextureAtlas* atlas, CharacterBitmap* char_bitmap, hb_codepoint_t codepoint) {
    atlas_insert_bitmap(atlas, char_bitmap->bitmap_buffer, 
                       char_bitmap->character.size[0], char_bitmap->character.size[1],
                       &char_bitmap->character, atlas->index++);
    
    // Find empty cache slot
    int slot = atlas_find_empty_slot(atlas);
    if (slot >= 0) {
        atlas->cache[slot].character = char_bitmap->character;
        atlas->cache[slot].fresh = true;
        atlas->cache[slot].valid = true;
        atlas->cache_keys[slot] = codepoint;
        atlas->cache_size++;
    }
}

// Replace stale glyph (replaces Replace method)
static inline void atlas_replace(TextureAtlas* atlas, CharacterBitmap* char_bitmap, 
                                int stale_slot, hb_codepoint_t codepoint) {
    uint32_t reuse_index = atlas->cache[stale_slot].character.texture_array_index;
    
    atlas_insert_bitmap(atlas, char_bitmap->bitmap_buffer,
                       char_bitmap->character.size[0], char_bitmap->character.size[1], 
                       &char_bitmap->character, reuse_index);
    
    // Replace the cache entry
    atlas->cache[stale_slot].character = char_bitmap->character;
    atlas->cache[stale_slot].fresh = true;
    atlas->cache[stale_slot].valid = true;
    atlas->cache_keys[stale_slot] = codepoint;
    
    printf("Replaced stale glyph at slot %d with codepoint %u\n", stale_slot, codepoint);
}

// Check if atlas contains codepoint (replaces Contains method)
static inline bool atlas_contains(const TextureAtlas* atlas, hb_codepoint_t codepoint) {
    return atlas_find_cache_entry(atlas, codepoint) >= 0;
}

// Get character by codepoint (replaces Get method)
static inline Character* atlas_get(TextureAtlas* atlas, hb_codepoint_t codepoint) {
    int slot = atlas_find_cache_entry(atlas, codepoint);
    if (slot >= 0) {
        atlas->cache[slot].fresh = true;  // Mark as recently used
        return &atlas->cache[slot].character;
    }
    return NULL;
}

// Check if atlas is full (replaces IsFull method)
static inline bool atlas_is_full(const TextureAtlas* atlas) {
    return atlas->cache_size >= ATLAS_TEXTURE_DEPTH;
}

// Check if atlas contains stale entries (replaces Contains_stale method)
static inline bool atlas_contains_stale(const TextureAtlas* atlas) {
    return atlas_find_stale_entry(atlas) >= 0;
}

// Mark all entries as stale (replaces Invalidate method)
static inline void atlas_invalidate(TextureAtlas* atlas) {
    for (size_t i = 0; i < ATLAS_TEXTURE_DEPTH; i++) {
        if (atlas->cache[i].valid) {
            atlas->cache[i].fresh = false;
        }
    }
}

// Get OpenGL texture ID (replaces GetTexture method)
static inline GLuint atlas_get_texture(const TextureAtlas* atlas) {
    return atlas->texture;
}

// Insert new glyph (replaces Insert method)
static inline void atlas_insert(TextureAtlas* atlas, hb_codepoint_t codepoint, CharacterBitmap* char_bitmap) {
    assert(!atlas_is_full(atlas) || atlas_contains_stale(atlas));
    
    if (!atlas_is_full(atlas)) {
        atlas_append(atlas, char_bitmap, codepoint);
    } else {
        // Find first stale entry
        int stale_slot = atlas_find_stale_entry(atlas);
        assert(stale_slot >= 0);
        atlas_replace(atlas, char_bitmap, stale_slot, codepoint);
    }
}


#ifdef __cplusplus
}
#endif

#endif // TEXTURE_ATLAS_H
