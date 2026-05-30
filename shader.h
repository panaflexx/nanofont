
/*
 * Header-Only Shader Library - C Version
 */

#ifndef SHADER_H
#define SHADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Use standard OpenGL headers
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// OpenGL constants (define if not available)
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER 0x8DD9
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif

// OpenGL function pointers for shader operations
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum type);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (*PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (*PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (*PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

// Global function pointers
static PFNGLCREATESHADERPROC glCreateShader_ptr = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource_ptr = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader_ptr = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv_ptr = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ptr = NULL;
static PFNGLDELETESHADERPROC glDeleteShader_ptr = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram_ptr = NULL;
static PFNGLATTACHSHADERPROC glAttachShader_ptr = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram_ptr = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv_ptr = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ptr = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram_ptr = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram_ptr = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ptr = NULL;
static PFNGLUNIFORM1FPROC glUniform1f_ptr = NULL;
static PFNGLUNIFORM2FPROC glUniform2f_ptr = NULL;
static PFNGLUNIFORM4FPROC glUniform4f_ptr = NULL;

// Macros to use function pointers
#define glCreateShader glCreateShader_ptr
#define glShaderSource glShaderSource_ptr
#define glCompileShader glCompileShader_ptr
#define glGetShaderiv glGetShaderiv_ptr
#define glGetShaderInfoLog glGetShaderInfoLog_ptr
#define glDeleteShader glDeleteShader_ptr
#define glCreateProgram glCreateProgram_ptr
#define glAttachShader glAttachShader_ptr
#define glLinkProgram glLinkProgram_ptr
#define glGetProgramiv glGetProgramiv_ptr
#define glGetProgramInfoLog glGetProgramInfoLog_ptr
#define glDeleteProgram glDeleteProgram_ptr
#define glUseProgram glUseProgram_ptr
#define glGetUniformLocation glGetUniformLocation_ptr
#define glUniform1f glUniform1f_ptr
#define glUniform2f glUniform2f_ptr
#define glUniform4f glUniform4f_ptr

// Shader structure
typedef struct Shader {
    GLuint programId;
    bool valid;
    bool gl_functions_loaded;
} Shader;

// =============================================================================
// IMPLEMENTATION
// =============================================================================

typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

static PFNGLUNIFORM1IPROC glUniform1i_ptr = NULL;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv_ptr = NULL;

#define glUniform1i glUniform1i_ptr
#define glUniformMatrix4fv glUniformMatrix4fv_ptr

// Load OpenGL shader function pointers
static inline bool shader_load_gl_functions(void) {
    if (glCreateShader_ptr != NULL) return true; // Already loaded
    
    // Use GLFW to get function pointers
    #include <GLFW/glfw3.h>
    
    printf("Loading OpenGL shader functions...\n");
    
    glCreateShader_ptr = (PFNGLCREATESHADERPROC)glfwGetProcAddress("glCreateShader");
    glShaderSource_ptr = (PFNGLSHADERSOURCEPROC)glfwGetProcAddress("glShaderSource");
    glCompileShader_ptr = (PFNGLCOMPILESHADERPROC)glfwGetProcAddress("glCompileShader");
    glGetShaderiv_ptr = (PFNGLGETSHADERIVPROC)glfwGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog_ptr = (PFNGLGETSHADERINFOLOGPROC)glfwGetProcAddress("glGetShaderInfoLog");
    glDeleteShader_ptr = (PFNGLDELETESHADERPROC)glfwGetProcAddress("glDeleteShader");
    glCreateProgram_ptr = (PFNGLCREATEPROGRAMPROC)glfwGetProcAddress("glCreateProgram");
    glAttachShader_ptr = (PFNGLATTACHSHADERPROC)glfwGetProcAddress("glAttachShader");
    glLinkProgram_ptr = (PFNGLLINKPROGRAMPROC)glfwGetProcAddress("glLinkProgram");
    glGetProgramiv_ptr = (PFNGLGETPROGRAMIVPROC)glfwGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog_ptr = (PFNGLGETPROGRAMINFOLOGPROC)glfwGetProcAddress("glGetProgramInfoLog");
    glDeleteProgram_ptr = (PFNGLDELETEPROGRAMPROC)glfwGetProcAddress("glDeleteProgram");
    glUseProgram_ptr = (PFNGLUSEPROGRAMPROC)glfwGetProcAddress("glUseProgram");
    glGetUniformLocation_ptr = (PFNGLGETUNIFORMLOCATIONPROC)glfwGetProcAddress("glGetUniformLocation");
    glUniform1f_ptr = (PFNGLUNIFORM1FPROC)glfwGetProcAddress("glUniform1f");
    glUniform2f_ptr = (PFNGLUNIFORM2FPROC)glfwGetProcAddress("glUniform2f");
    glUniform4f_ptr = (PFNGLUNIFORM4FPROC)glfwGetProcAddress("glUniform4f");
    
    // Add the new function pointers with error checking:
    glUniform1i_ptr = (PFNGLUNIFORM1IPROC)glfwGetProcAddress("glUniform1i");
    glUniformMatrix4fv_ptr = (PFNGLUNIFORMMATRIX4FVPROC)glfwGetProcAddress("glUniformMatrix4fv");
    
    // Check if critical functions loaded
    if (!glUniformMatrix4fv_ptr) {
        printf("Warning: glUniformMatrix4fv not available\n");
    }
    if (!glUniform1i_ptr) {
        printf("Warning: glUniform1i not available\n");
    }
    
    bool success = (glCreateShader_ptr && glShaderSource_ptr && glCompileShader_ptr &&
                    glGetShaderiv_ptr && glGetShaderInfoLog_ptr && glDeleteShader_ptr &&
                    glCreateProgram_ptr && glAttachShader_ptr && glLinkProgram_ptr &&
                    glGetProgramiv_ptr && glGetProgramInfoLog_ptr && glDeleteProgram_ptr &&
                    glUseProgram_ptr && glGetUniformLocation_ptr && glUniform1f_ptr &&
                    glUniform2f_ptr && glUniform4f_ptr);
    
    // Note: We don't require glUniform1i_ptr and glUniformMatrix4fv_ptr for basic functionality
    // They will be checked individually in their respective functions
    
    printf("OpenGL shader functions loaded: %s\n", success ? "SUCCESS" : "FAILED");
    return success;
}

// Set uniform 1i - with safety check
static inline void shader_set_1i(Shader* shader, const char* name, int v) {
    if (!shader || !shader->valid || !shader->gl_functions_loaded || !name) return;
    
    if (!glUniform1i_ptr) {
        printf("Warning: glUniform1i not available, skipping uniform '%s'\n", name);
        return;
    }
    
    GLint location = glGetUniformLocation(shader->programId, name);
    if (location >= 0) {
        glUniform1i(location, v);
    } else {
        printf("Warning: Uniform '%s' not found in shader\n", name);
    }
}

// Set uniform matrix4fv - with safety check
static inline void shader_set_matrix4fv(Shader* shader, const char* name, const float* matrix) {
    if (!shader || !shader->valid || !shader->gl_functions_loaded || !name || !matrix) return;
    
    if (!glUniformMatrix4fv_ptr) {
        printf("Warning: glUniformMatrix4fv not available, skipping uniform '%s'\n", name);
        return;
    }
    
    GLint location = glGetUniformLocation(shader->programId, name);
    if (location >= 0) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
    } else {
        printf("Warning: Uniform '%s' not found in shader\n", name);
    }
}

// Check shader compilation error
static inline void shader_check_gl_shader_error(GLuint shader_id) {
    GLint status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        printf("Shader didn't compile successfully\n");
        
        char buffer[512];
        glGetShaderInfoLog(shader_id, 512, NULL, buffer);
        printf("Shader error: %s\n", buffer);
        
        exit(EXIT_FAILURE);
    }
}

// Check program linking error
static inline void shader_check_gl_program_error(GLuint program_id) {
    GLint status;
    glGetProgramiv(program_id, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        printf("Program didn't link successfully\n");
        
        char buffer[512];
        glGetProgramInfoLog(program_id, 512, NULL, buffer);
        printf("Program error: %s\n", buffer);
        
        exit(EXIT_FAILURE);
    }
}

// Create shader from source code
static inline Shader* shader_create(const char* vertex_code, const char* fragment_code, const char* geometry_code) {
    if (!vertex_code || !fragment_code) return NULL;
    
    // Load OpenGL functions
    if (!shader_load_gl_functions()) {
        printf("Failed to load OpenGL shader functions\n");
        return NULL;
    }
    
    Shader* shader = (Shader*)calloc(1, sizeof(Shader));
    if (!shader) return NULL;
    
    shader->gl_functions_loaded = true;
    shader->valid = false;
    
    GLuint vertex_shader_id = 0, fragment_shader_id = 0, geometry_shader_id = 0;
    
    // Compile geometry shader if provided
    if (geometry_code && strlen(geometry_code) > 0) {
        printf("Compiling geometry shader\n");
        geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry_shader_id, 1, &geometry_code, NULL);
        glCompileShader(geometry_shader_id);
        shader_check_gl_shader_error(geometry_shader_id);
    }
    
    // Compile vertex shader
    printf("Compiling vertex shader\n");
    vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_id, 1, &vertex_code, NULL);
    glCompileShader(vertex_shader_id);
    shader_check_gl_shader_error(vertex_shader_id);
    
    // Compile fragment shader
    printf("Compiling fragment shader\n");
    fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader_id, 1, &fragment_code, NULL);
    glCompileShader(fragment_shader_id);
    shader_check_gl_shader_error(fragment_shader_id);
    
    // Create program from shaders
    shader->programId = glCreateProgram();
    glAttachShader(shader->programId, vertex_shader_id);
    glAttachShader(shader->programId, fragment_shader_id);
    if (geometry_code && strlen(geometry_code) > 0) {
        glAttachShader(shader->programId, geometry_shader_id);
    }
    
    glLinkProgram(shader->programId);
    shader_check_gl_program_error(shader->programId);
    
    // Delete shader objects now that the program is linked
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    if (geometry_code && strlen(geometry_code) > 0) {
        glDeleteShader(geometry_shader_id);
    }
    
    shader->valid = true;
    printf("Compiling shaders done. Program ID = %u\n", shader->programId);
    
    return shader;
}

// Create shader without geometry shader
static inline Shader* shader_create_simple(const char* vertex_code, const char* fragment_code) {
    return shader_create(vertex_code, fragment_code, NULL);
}

// Destroy shader
static inline void shader_destroy(Shader* shader) {
    if (!shader) return;
    
    if (shader->valid && shader->gl_functions_loaded && shader->programId != 0) {
        glDeleteProgram(shader->programId);
    }
    
    free(shader);
}

// Use/activate the shader
static inline void shader_use(Shader* shader) {
    if (!shader || !shader->valid || !shader->gl_functions_loaded) return;
    glUseProgram(shader->programId);
}

// Set uniform 1f
static inline void shader_set_1f(Shader* shader, const char* name, float v) {
    if (!shader || !shader->valid || !shader->gl_functions_loaded || !name) return;
    GLint location = glGetUniformLocation(shader->programId, name);
    if (location >= 0) {
        glUniform1f(location, v);
    }
}

// Set uniform 2f
static inline void shader_set_2f(Shader* shader, const char* name, float x, float y) {
    if (!shader || !shader->valid || !shader->gl_functions_loaded || !name) return;
    GLint location = glGetUniformLocation(shader->programId, name);
    if (location >= 0) {
        glUniform2f(location, x, y);
    }
}

// Set uniform 4f
static inline void shader_set_4f(Shader* shader, const char* name, float x, float y, float z, float w) {
    if (!shader || !shader->valid || !shader->gl_functions_loaded || !name) return;
    GLint location = glGetUniformLocation(shader->programId, name);
    if (location >= 0) {
        glUniform4f(location, x, y, z, w);
    }
}

// Get shader program ID
static inline GLuint shader_get_program_id(const Shader* shader) {
    if (!shader || !shader->valid) return 0;
    return shader->programId;
}

// Check if shader is valid
static inline bool shader_is_valid(const Shader* shader) {
    return shader && shader->valid && shader->gl_functions_loaded;
}

// Helper function to read file contents (utility function)
static inline char* shader_read_file(const char* filepath) {
    if (!filepath) return NULL;
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Failed to open shader file: %s\n", filepath);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer
    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    // Read file
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    
    fclose(file);
    return content;
}

// Create shader from files
static inline Shader* shader_create_from_files(const char* vertex_path, const char* fragment_path, const char* geometry_path) {
    char* vertex_code = shader_read_file(vertex_path);
    char* fragment_code = shader_read_file(fragment_path);
    char* geometry_code = geometry_path ? shader_read_file(geometry_path) : NULL;
    
    if (!vertex_code || !fragment_code) {
        printf("Failed to read shader files\n");
        free(vertex_code);
        free(fragment_code);
        free(geometry_code);
        return NULL;
    }
    
    Shader* shader = shader_create(vertex_code, fragment_code, geometry_code);
    
    // Clean up file contents
    free(vertex_code);
    free(fragment_code);
    free(geometry_code);
    
    return shader;
}

#ifdef __cplusplus
}
#endif

#endif // SHADER_H
