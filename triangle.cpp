#include <GLES2/gl2.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <cstdio>

struct GLUserData
{
    GLuint m_Program;
    GLuint m_VBO;
    GLuint m_Texture;
};

const char* vertex_shader_src =
    "attribute vec2 position;\n"
    "void main() {\n"
    "  gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

const char* fragment_shader_src =
    "void main() {\n"
    "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    return shader;
}

void draw(void* userData) {
    GLUserData& glUserData = *static_cast<GLUserData*>(userData);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(glUserData.m_Program);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    GLint pos = glGetAttribLocation(glUserData.m_Program, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
        printf("Key pressed: %d\n", keyEvent->keyCode);
    } else if (eventType == EMSCRIPTEN_EVENT_KEYUP) {
        printf("Key released: %d\n", keyEvent->keyCode);
    }
    return EM_TRUE; // Prevent default browser behavior
}

int main() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;
    attr.depth = 1;
    attr.stencil = 0;
    attr.antialias = 1;
    attr.majorVersion = 2;
    attr.minorVersion = 0;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    GLUserData glUserData;

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    glUserData.m_Program = glCreateProgram();
    glAttachShader(glUserData.m_Program, vs);
    glAttachShader(glUserData.m_Program, fs);
    glLinkProgram(glUserData.m_Program);

    float vertices[] = {
        0.0f,  0.5f,
       -0.5f, -0.5f,
        0.5f, -0.5f
    };

    glGenBuffers(1, &glUserData.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, keyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, keyCallback);
    
    //glGenTextures(1, &texture);

    printf("Hello world!\n");

    emscripten_set_main_loop_arg(draw, static_cast<void*>(&glUserData), 0, 1);

    return 0;
}
