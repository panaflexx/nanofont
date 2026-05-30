#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "text_renderer.h"
#include "shader.h"

// Window dimensions
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Simple test shaders that don't use textures
static const char* SIMPLE_VERTEX_SHADER = 
    "#version 330 core\n"
    "\n"
    "layout (location = 0) in vec4 in_vertex;\n"
    "layout (location = 1) in ivec2 in_texture_ids;\n"
    "layout (location = 2) in vec4 in_char_color;\n"
    "\n"
    "uniform mat4 projection;\n"
    "\n"
    "out vec4 frag_color;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(in_vertex.xy, 0.0, 1.0);\n"
    "    frag_color = in_char_color;\n"
    "}\n";

static const char* SIMPLE_FRAGMENT_SHADER = 
    "#version 330 core\n"
    "\n"
    "in vec4 frag_color;\n"
    "out vec4 color;\n"
    "\n"
    "void main() {\n"
    "    color = frag_color;\n"
    "}\n";

// Global variables
static Shader* test_shader = NULL;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

// Create simple colored rectangles as fallback
RenderData* create_simple_test_data(void) {
    printf("Creating simple test data...\n");
    
    RenderData* data = (RenderData*)calloc(1, sizeof(RenderData));
    if (!data) return NULL;
    
    // Create 3 simple rectangles
    data->quad_count = 3;
    data->quads = (float*)malloc(data->quad_count * 6 * 4 * sizeof(float));
    data->texture_ids = (uint32_t*)malloc(data->quad_count * 6 * 2 * sizeof(uint32_t));
    data->colors = (float*)malloc(data->quad_count * 6 * 4 * sizeof(float));
    
    if (!data->quads || !data->texture_ids || !data->colors) {
        free(data->quads);
        free(data->texture_ids);
        free(data->colors);
        free(data);
        return NULL;
    }
    
    // Rectangle 1: Red
    float x1 = 100.0f, y1 = 100.0f, w1 = 200.0f, h1 = 50.0f;
    float* quad1 = &data->quads[0 * 6 * 4];
    // Triangle 1
    quad1[0] = x1;      quad1[1] = y1;      quad1[2] = 0.0f; quad1[3] = 1.0f;
    quad1[4] = x1;      quad1[5] = y1 + h1; quad1[6] = 0.0f; quad1[7] = 0.0f;
    quad1[8] = x1 + w1; quad1[9] = y1;      quad1[10] = 1.0f; quad1[11] = 1.0f;
    // Triangle 2
    quad1[12] = x1;      quad1[13] = y1 + h1; quad1[14] = 0.0f; quad1[15] = 0.0f;
    quad1[16] = x1 + w1; quad1[17] = y1;      quad1[18] = 1.0f; quad1[19] = 1.0f;
    quad1[20] = x1 + w1; quad1[21] = y1 + h1; quad1[22] = 1.0f; quad1[23] = 0.0f;
    
    // Rectangle 2: Green
    float x2 = 100.0f, y2 = 200.0f, w2 = 200.0f, h2 = 50.0f;
    float* quad2 = &data->quads[1 * 6 * 4];
    quad2[0] = x2;      quad2[1] = y2;      quad2[2] = 0.0f; quad2[3] = 1.0f;
    quad2[4] = x2;      quad2[5] = y2 + h2; quad2[6] = 0.0f; quad2[7] = 0.0f;
    quad2[8] = x2 + w2; quad2[9] = y2;      quad2[10] = 1.0f; quad2[11] = 1.0f;
    quad2[12] = x2;      quad2[13] = y2 + h2; quad2[14] = 0.0f; quad2[15] = 0.0f;
    quad2[16] = x2 + w2; quad2[17] = y2;      quad2[18] = 1.0f; quad2[19] = 1.0f;
    quad2[20] = x2 + w2; quad2[21] = y2 + h2; quad2[22] = 1.0f; quad2[23] = 0.0f;
    
    // Rectangle 3: Blue
    float x3 = 100.0f, y3 = 300.0f, w3 = 200.0f, h3 = 50.0f;
    float* quad3 = &data->quads[2 * 6 * 4];
    quad3[0] = x3;      quad3[1] = y3;      quad3[2] = 0.0f; quad3[3] = 1.0f;
    quad3[4] = x3;      quad3[5] = y3 + h3; quad3[6] = 0.0f; quad3[7] = 0.0f;
    quad3[8] = x3 + w3; quad3[9] = y3;      quad3[10] = 1.0f; quad3[11] = 1.0f;
    quad3[12] = x3;      quad3[13] = y3 + h3; quad3[14] = 0.0f; quad3[15] = 0.0f;
    quad3[16] = x3 + w3; quad3[17] = y3;      quad3[18] = 1.0f; quad3[19] = 1.0f;
    quad3[20] = x3 + w3; quad3[21] = y3 + h3; quad3[22] = 1.0f; quad3[23] = 0.0f;
    
    // Set colors and texture IDs for all rectangles
    for (size_t i = 0; i < data->quad_count; i++) {
        uint32_t* tex_ids = &data->texture_ids[i * 6 * 2];
        float* colors = &data->colors[i * 6 * 4];
        
        for (int v = 0; v < 6; v++) {
            tex_ids[v * 2] = 0;     // texture array index
            tex_ids[v * 2 + 1] = 2; // texture type (2 = solid color)
            
            // Different color per rectangle
            if (i == 0) {
                colors[v * 4] = 1.0f; colors[v * 4 + 1] = 0.0f; colors[v * 4 + 2] = 0.0f; // Red
            } else if (i == 1) {
                colors[v * 4] = 0.0f; colors[v * 4 + 1] = 1.0f; colors[v * 4 + 2] = 0.0f; // Green
            } else {
                colors[v * 4] = 0.0f; colors[v * 4 + 1] = 0.0f; colors[v * 4 + 2] = 1.0f; // Blue
            }
            colors[v * 4 + 3] = 1.0f; // Alpha
        }
    }
    
    data->texture_id_count = data->quad_count * 6;
    data->color_count = data->quad_count * 6;
    
    printf("Created simple test data: %zu quads\n", data->quad_count);
    return data;
}

int main(int argc, char** argv) {
    printf("Starting simple text renderer test...\n");
    
    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simple Test", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    // Disable blending for now to simplify
    glDisable(GL_BLEND);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Create simple shader
    printf("Creating simple shader...\n");
    test_shader = shader_create_simple(SIMPLE_VERTEX_SHADER, SIMPLE_FRAGMENT_SHADER);
    if (!test_shader) {
        printf("Failed to create shader\n");
        glfwTerminate();
        return -1;
    }

    // Create text renderer
    printf("Creating text renderer...\n");
    TextRenderer* renderer = tr_create_renderer();
    if (!renderer) {
        printf("Failed to create text renderer\n");
        shader_destroy(test_shader);
        glfwTerminate();
        return -1;
    }

    tr_set_view_size(renderer, SCR_WIDTH, SCR_HEIGHT);

    // Create test data
    RenderData* test_data = create_simple_test_data();
    if (!test_data) {
        printf("Failed to create test data\n");
        tr_destroy_renderer(renderer);
        shader_destroy(test_shader);
        glfwTerminate();
        return -1;
    }

    printf("Starting render loop...\n");
    int frame = 0;
    
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Clear with a different color
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark gray
        glClear(GL_COLOR_BUFFER_BIT);

        // Render test rectangles
        printf("Frame %d: Rendering...\n", frame);
        tr_render(renderer, test_data, test_shader);
        
        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            printf("OpenGL error: %d\n", error);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        frame++;
        if (frame > 10) break; // Stop after a few frames for debugging
    }

    printf("Cleaning up...\n");
    tr_free_render_data(test_data);
    tr_destroy_renderer(renderer);
    shader_destroy(test_shader);
    glfwTerminate();
    return 0;
}

