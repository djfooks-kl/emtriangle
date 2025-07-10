#include <GLES2/gl2.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <array>
#include <cstdio>

constexpr GLuint s_AttributePosition = 0;
constexpr GLuint s_AttributeTextureIndex = 1;
const char* vertexShaderStr = R"(#version 300 es
    precision mediump float;

    layout (location = 0) in vec2 position;
    layout (location = 1) in float inTextureIndex;
    out float textureIndex;

    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        textureIndex = inTextureIndex;
    }
)";

const char* fragmentShaderStr = R"(#version 300 es
    precision mediump float;

    in float textureIndex;
    out vec4 FragColor;

    uniform sampler2D ourTexture;

    void main() {
        vec4 color = texture(ourTexture, vec2(textureIndex, 0.0));
        FragColor = color;
    }
)";

struct GLUserData
{
    GLuint m_Program;
    GLuint m_VBO;
    GLuint m_Texture;
};

GLuint compile_shader(GLenum type, const char* name, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        std::array<char, 4096> buffer;
        GLsizei length;
        glGetShaderInfoLog(shader, buffer.size(), &length, buffer.data());
        printf("Error compiling shader \"%s\": %s\n", name, buffer.data());
    }
    return shader;
}

void draw(void* userData) {
    GLUserData& glUserData = *static_cast<GLUserData*>(userData);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(glUserData.m_Program);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    glVertexAttribPointer(s_AttributePosition, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(s_AttributePosition);

    glVertexAttribPointer(s_AttributeTextureIndex, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(s_AttributeTextureIndex);

    glBindTexture(GL_TEXTURE_2D, glUserData.m_Texture);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* /*userData*/) {
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

    GLuint vs = compile_shader(GL_VERTEX_SHADER, "vertex", vertexShaderStr);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, "fragment", fragmentShaderStr);
    glUserData.m_Program = glCreateProgram();
    glAttachShader(glUserData.m_Program, vs);
    glAttachShader(glUserData.m_Program, fs);
    glLinkProgram(glUserData.m_Program);
    GLint linked;
    glGetProgramiv(glUserData.m_Program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        std::array<char, 4096> buffer;
        GLsizei length;
        glGetProgramInfoLog(glUserData.m_Program, buffer.size(), &length, buffer.data());
        printf("Error linking program %s\n", buffer.data());
    }

    glGenTextures(1, &glUserData.m_Texture);
    glBindTexture(GL_TEXTURE_2D, glUserData.m_Texture);

    std::array<uint8_t, 6> data = { 0, 0, 255, 255, 0, 0 };

    const GLsizei width = 2;
    const GLsizei height = 1;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    const float fWidth = width;
    float vertices[] = {
        // positions    texture index
        0.0f,  0.5f,    0.f / fWidth,
       -0.5f, -0.5f,    1.f / fWidth,
        0.5f, -0.5f,    2.f / fWidth
    };

    glGenBuffers(1, &glUserData.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, keyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, keyCallback);

    printf("Hello world!\n");

    emscripten_set_main_loop_arg(draw, static_cast<void*>(&glUserData), 0, 1);

    return 0;
}
