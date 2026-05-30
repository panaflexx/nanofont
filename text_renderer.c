
#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include "text_renderer.h"
#include "shader.h"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
    #include <unistd.h>
#endif

// Window dimensions
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

static const char* CURSOR_VERTEX_SHADER = 
    "#version 330 core\n"
    "\n"
    "layout (location = 0) in vec2 position;\n"
    "uniform mat4 projection;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position.xy, 0.0, 1.0);\n"
    "}\n";

static const char* CURSOR_FRAGMENT_SHADER = 
    "#version 330 core\n"
    "\n"
    "uniform vec4 cursor_color;\n"
    "out vec4 diffuseColor;\n"
    "\n"
    "void main() {\n"
    "    diffuseColor = cursor_color;\n"
    "}\n";

static const char* TEXT_VERTEX_SHADER = 
    "#version 460\n"
    "\n"
    "layout (location=0) in vec4 in_vertex;\n"
    "layout (location=1) in ivec2 in_texture_ids;\n"
    "layout (location=2) in vec4 in_char_color;\n"
    "layout (location=3) in float in_packed_color;\n"
    "\n"
    "uniform mat4 projection;\n"
    "\n"
    "out vec2 ex_texCoords;\n"
    "flat out ivec2 ex_texture_ids;\n"
    "flat out vec4 ex_char_color;\n"
    "flat out float ex_packed_color;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(in_vertex.xy, 0.0, 1.0);\n"
    "    ex_texCoords = in_vertex.zw;\n"
    "    ex_texture_ids = in_texture_ids;\n"
    "    ex_char_color = in_char_color;\n"
    "    ex_packed_color = in_packed_color;\n"
    "}\n";

static const char* TEXT_FRAGMENT_SHADER = 
    "#version 330\n"
    "in vec2 ex_texCoords;\n"
    "flat in ivec2 ex_texture_ids;\n"
    "flat in vec4 ex_char_color;\n"
    "flat in float ex_packed_color;\n"
    "uniform sampler2DArray monochromatic_texture_array;\n"
    "uniform sampler2DArray colored_texture_array;\n"
    "layout(location = 0, index = 0) out vec4 color;\n"
    "layout(location = 0, index = 1) out vec4 colorMask;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 alpha_map;\n"
    "    if (ex_texture_ids.y == 1) {\n"
    "        alpha_map = texture(colored_texture_array, vec3(ex_texCoords, ex_texture_ids.x));\n"
    "        color = alpha_map;\n"
    "    } else if (ex_texture_ids.y == 0) {\n"
    "        alpha_map = texture(monochromatic_texture_array, vec3(ex_texCoords, ex_texture_ids.x));\n"
    "        color = ex_char_color;\n"
    "    } else if (ex_texture_ids.y == 2) {\n"
    "        alpha_map = vec4(1,1,1,1);\n"
    "        color = ex_char_color;\n"
    "    }\n"
    "    colorMask = ex_char_color.a * alpha_map;\n"
    "}\n";

// Global variables for shaders
static Shader* text_shader = NULL;
static Shader* cursor_shader = NULL;

static RenderData* cached_render_data = NULL;
static bool last_draw_cursor = false;
static bool text_updated = false; // Equivalent to state.Updated

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
	// If user presses keys, mark text as updated (example)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        text_updated = true; // Mark that text content changed
    }
}

// Cross-platform sleep function (milliseconds)
static inline void sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}


int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Text Renderer Debug", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

    // Enable blending for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Set the viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Create shaders from static strings
    printf("Creating text shader...\n");
    text_shader = shader_create_simple(TEXT_VERTEX_SHADER, TEXT_FRAGMENT_SHADER);
    if (!text_shader) {
        printf("Failed to create text shader\n");
        glfwTerminate();
        return -1;
    }

    printf("Creating cursor shader...\n");
    cursor_shader = shader_create_simple(CURSOR_VERTEX_SHADER, CURSOR_FRAGMENT_SHADER);
    if (!cursor_shader) {
        printf("Failed to create cursor shader\n");
        shader_destroy(text_shader);
        glfwTerminate();
        return -1;
    }

    // Create text renderer
    printf("Creating text renderer...\n");
    TextRenderer* renderer = tr_create_renderer();
    if (!renderer) {
        printf("Failed to create text renderer\n");
        shader_destroy(text_shader);
        shader_destroy(cursor_shader);
        glfwTerminate();
        return -1;
    }
	tr_set_view_size(renderer, SCR_WIDTH, SCR_HEIGHT);

    printf("Text renderer created successfully\n");


    printf("Initializing texture atlases...\n");
    if (!tr_init_atlases(renderer, text_shader)) {
        printf("Failed to initialize texture atlases\n");
        tr_destroy_renderer(renderer);
        shader_destroy(text_shader);
        glfwTerminate();
        return -1;
    }

    // Try to add fonts with better error reporting
    printf("Attempting to load fonts...\n");
    
    // Try different font paths
    const char* font_paths[] = {
        "assets/fonts/FiraCode-Retina.ttf",
        "assets/fonts/NotoColorEmoji.ttf",
        "assets/fonts/Roboto-Regular.ttf",
        "assets/fonts/UbuntuMono.ttf",
        NULL
    };

	int fonts_loaded = tr_load_faces(renderer, font_paths, 5, 16);
/* 
    for (int i = 0; font_paths[i] != NULL; i++) {
        printf("Trying font: %s\n", font_paths[i]);
        int font_id = tr_add_font(renderer, font_paths[i]);
        if (font_id >= 0) {
            printf("Successfully loaded font %d: %s\n", font_id, font_paths[i]);
            fonts_loaded = true;
            break;
        } else {
            printf("Failed to load font: %s\n", font_paths[i]);
        }
    }
*/
    
    if (fonts_loaded < 1) {
        printf("WARNING: No fonts could be loaded! Using fallback rendering.\n");
    } else {
        printf("Loaded %d FONTS\n", fonts_loaded);
	}

    // Test data
    const char* default_lines[] = {
        "This is red text and normal text",
        "✅ Hello, World!",
        "🐺💺💆🐡🐛🕞🍰🐽🍣🍫🔂🏆🍩 tempus laoreet 💭🏤🔚🔋🔈💡📉🎆💲👽 🔽🎊📺💅 🌋🍷🍲📜🔯📄📗🍡📈🐰🔤🍖👝 et 📯🔀🍴💇",
        "Another line for testing",
        "Press ESC to exit"
    };
    
    const char** lines = default_lines;
    size_t line_count = 5;

    printf("Processing %zu lines of text...\n", line_count);

    // Initialize cursor state
    bool cursor_visible = true;
    double last_cursor_toggle = 0.0;
    const double cursor_blink_rate = 0.5;
	double last_activity_time = glfwGetTime();
    double last_print_time = 0.0;
    int frame_count = 0;

    // Main render loop
    printf("Starting main loop...\n");
	while (!glfwWindowShouldClose(window)) {
        // Poll events to detect user activity
        //glfwPollEvents();
        processInput(window);

        // Check for user activity and update last_activity_time
        if (text_updated) {
            last_activity_time = glfwGetTime();
			printf("text_updated TRUE\n");
        }

        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // ✅ Same as C++
        glClear(GL_COLOR_BUFFER_BIT);

        bool draw_cursor = fmod(glfwGetTime(), 1.0) < 0.5;

        if (draw_cursor != last_draw_cursor || text_updated) {
            // Free old render data
            if (cached_render_data) {
                tr_free_render_data(cached_render_data);
            }
            
            // Generate new render data
            cached_render_data = tr_render_prep_lines(renderer, lines, line_count,
                                                    0.0f, 0.0f,    // startX, startY
                                                    0, 0,           // cursorX, cursorY  
                                                    draw_cursor);
            
            last_draw_cursor = draw_cursor;
        }
		
        text_updated = false;

        // Perform rendering with cached data
        if (cached_render_data) {
            tr_render(renderer, cached_render_data, text_shader);
        }
        
        // Swap buffers
        glfwSwapBuffers(window);

        // FPS calculation and printing
        double current_time = glfwGetTime();
		double elapsed = current_time - last_activity_time;
        double target_fps = (elapsed < 5.0) ? 60.0 : 4.0; // 60 FPS or 4 FPS
        double timeout = 1.0 / target_fps;

        frame_count++;
        if (last_print_time == 0.0) {
            last_print_time = current_time;
        } else if (current_time - last_print_time >= 1.0) {
            double fps = frame_count / (current_time - last_print_time);
            printf("FPS: %.2f  ELAPSED: %.1f  TIMEOUT: %.2f  TARGET: %.2f   \r", fps, elapsed, timeout, target_fps);
            fflush(stdout);
            frame_count = 0;
            last_print_time = current_time;
        }
		
        // Wait for events or timeout
        glfwWaitEventsTimeout(timeout);
    }

    // Cleanup
    printf("Cleaning up...\n");
    tr_destroy_renderer(renderer);
    shader_destroy(text_shader);
    shader_destroy(cursor_shader);
    glfwTerminate();

    printf("Program exited successfully\n");
    return 0;
}
