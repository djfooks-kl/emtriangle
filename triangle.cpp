#include <array>
#include <chrono>
#include <cstdio>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

constexpr int s_TextureWidth = 2;
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

    std::chrono::steady_clock::time_point m_LastFrameTime;
    double m_Time = 0.0;
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

void setTextureData(const double time)
{
    const uint8_t animation = static_cast<uint8_t>(std::abs(static_cast<uint8_t>(time * 100.f) - 128));
    std::array<uint8_t, 6> data = { animation, 0, 255, static_cast<uint8_t>(255 - animation), 0, 0 };

    const GLsizei height = 1;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, s_TextureWidth, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

void update(void* userData) {
    GLUserData& glUserData = *static_cast<GLUserData*>(userData);

    const std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
    const std::chrono::duration<float> deltaTimeDuration = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - glUserData.m_LastFrameTime);
    const float deltaTime = deltaTimeDuration.count();
    glUserData.m_LastFrameTime = currentTime;

    glUserData.m_Time += deltaTime;

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(glUserData.m_Program);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    glVertexAttribPointer(s_AttributePosition, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(s_AttributePosition);

    glVertexAttribPointer(s_AttributeTextureIndex, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(s_AttributeTextureIndex);

    glBindTexture(GL_TEXTURE_2D, glUserData.m_Texture);
    setTextureData(glUserData.m_Time);

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
    glUserData.m_LastFrameTime = std::chrono::steady_clock::now();

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
    setTextureData(0.0);

    const float fWidth = s_TextureWidth;
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

    emscripten_set_main_loop_arg(update, static_cast<void*>(&glUserData), 0, 1);

    return 0;
}
