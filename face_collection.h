// Copyright 2019 <Andrea Cognolato>
#ifndef SRC_FACE_COLLECTION_H_
#define SRC_FACE_COLLECTION_H_

#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "./constants.h"
//#include "./shaping_cache.h"

// Face structures
typedef struct FaceEntry {
    FT_Face face;
    GLsizei width;
    GLsizei height;
} FaceEntry;

typedef struct FaceCollection {
    FaceEntry faces[256];
    size_t count;
    size_t capacity = 256;
} FaceCollection;

// Function declarations
static inline FaceCollection LoadFaces(FT_Library ft, const char** face_names, size_t face_count, FT_UInt kFontPixelWidth);
// Implementation

static inline FaceCollection LoadFaces(FT_Library ft, const char** face_names, size_t face_count, FT_UInt kFontPixelWidth) {
    FaceCollection faces = {0};
	FT_UInt kFontPixelHeight = kFontPixelWidth - 1;
    faces.capacity = face_count;
    faces.faces = (FaceEntry*)malloc(face_count * sizeof(FaceEntry));
    if (!faces.faces) {
        fprintf(stderr, "Failed to allocate memory for faces\n");
        exit(EXIT_FAILURE);
    }
    faces.count = 0;


    for (size_t i = 0; i < face_count; i++) {
        const char* face_name = face_names[i];
        FT_Face face;
        if (FT_New_Face(ft, face_name, 0, &face)) {
            fprintf(stderr, "Could not load font: %s\n", face_name);
            exit(EXIT_FAILURE);
        }

        if (FT_HAS_COLOR(face)) {
            if (FT_Select_Size(face, 0)) {
                fprintf(stderr, "Could not request the font size (fixed): %s\n", face_name);
                exit(EXIT_FAILURE);
            }
        } else {
            if (FT_Set_Pixel_Sizes(face, kFontPixelWidth, kFontPixelHeight)) {
                fprintf(stderr, "Could not request the font size (in pixels): %s\n", face_name);
                exit(EXIT_FAILURE);
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
        faces.faces[faces.count].face = face;
        faces.faces[faces.count].width = width;
        faces.faces[faces.count].height = height;
        faces.count++;
    }

    return faces;
}


#endif  // SRC_FACE_COLLECTION_H_
