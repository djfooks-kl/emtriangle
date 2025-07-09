#include <GLES2/gl2.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <array>
#include <cstdio>

struct GLUserData
{
    GLuint m_Program;
    GLuint m_VBO;
    GLuint m_Texture;
};

const char* vertexShaderStr = R"(#version 300 es
    precision mediump float;

    in vec2 position;
    in float inTextureIndex;
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
        vec4 color = texture(ourTexture, vec2(0.0, 0.0));
        FragColor = vec4(color.x, textureIndex, color.z, 1.0);
    }
)";

GLuint compile_shader(GLenum type, const char* src) {
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
        printf("Error compiling shader %s\n", buffer.data());
    }
    return shader;
}

void draw(void* userData) {
    GLUserData& glUserData = *static_cast<GLUserData*>(userData);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(glUserData.m_Program);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    const GLint pos = glGetAttribLocation(glUserData.m_Program, "position");
    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(pos);

    const GLint textureIndex = glGetAttribLocation(glUserData.m_Program, "inTextureIndex");
    glVertexAttribPointer(textureIndex, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(textureIndex);

    glBindTexture(GL_TEXTURE_2D, glUserData.m_Texture);

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

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertexShaderStr);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderStr);
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

    float vertices[] = {
        // positions    texture index
        0.0f,  0.5f,    0.f,
       -0.5f, -0.5f,    1.f,
        0.5f, -0.5f,    0.f
    };

    glGenBuffers(1, &glUserData.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, glUserData.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, keyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, keyCallback);
    
    glGenTextures(1, &glUserData.m_Texture);
    glBindTexture(GL_TEXTURE_2D, glUserData.m_Texture);

    std::array<uint8_t, 3> data = { 0, 0, 255 };

    const GLsizei width = 1; 
    const GLsizei height = 1;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D); // TODO try removing this

    printf("Hello world!\n");

    emscripten_set_main_loop_arg(draw, static_cast<void*>(&glUserData), 0, 1);

    return 0;
}
