#include "overlay.h"
#include <android/log.h>
#include <cmath>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Overlay", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Overlay", __VA_ARGS__)

const char* vertexShaderSrc = R"(
    attribute vec2 aPosition;
    uniform mat4 uProjection;
    void main() {
        gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    }
)";

const char* fragmentShaderSrc = R"(
    precision mediump float;
    uniform vec4 uColor;
    void main() {
        gl_FragColor = uColor;
    }
)";

Overlay::Overlay() : m_window(nullptr), m_display(EGL_NO_DISPLAY),
    m_surface(EGL_NO_SURFACE), m_context(EGL_NO_CONTEXT),
    m_width(0), m_height(0), m_program(0) {}

Overlay::~Overlay() {
    cleanup();
}

bool Overlay::init(ANativeWindow* window) {
    m_window = window;
    
    if (!initEGL()) {
        LOGE("Failed to init EGL");
        return false;
    }
    
    if (!initShaders()) {
        LOGE("Failed to init shaders");
        return false;
    }
    
    m_width = ANativeWindow_getWidth(window);
    m_height = ANativeWindow_getHeight(window);
    
    LOGI("Overlay initialized: %dx%d", m_width, m_height);
    return true;
}

bool Overlay::initEGL() {
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY) return false;
    
    if (!eglInitialize(m_display, nullptr, nullptr)) return false;
    
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs)) return false;
    
    EGLint format;
    eglGetConfigAttrib(m_display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(m_window, 0, 0, format);
    
    m_surface = eglCreateWindowSurface(m_display, config, m_window, nullptr);
    if (m_surface == EGL_NO_SURFACE) return false;
    
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttribs);
    if (m_context == EGL_NO_CONTEXT) return false;
    
    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) return false;
    
    return true;
}

bool Overlay::initShaders() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);
    
    GLint compiled;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        LOGE("Vertex shader compilation failed");
        return false;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        LOGE("Fragment shader compilation failed");
        return false;
    }
    
    // Link program
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);
    
    GLint linked;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        LOGE("Shader program linking failed");
        return false;
    }
    
    m_posAttrib = glGetAttribLocation(m_program, "aPosition");
    m_colorUniform = glGetUniformLocation(m_program, "uColor");
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void Overlay::cleanup() {
    if (m_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_context != EGL_NO_CONTEXT) eglDestroyContext(m_display, m_context);
        if (m_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_surface);
        eglTerminate(m_display);
    }
    m_display = EGL_NO_DISPLAY;
    m_context = EGL_NO_CONTEXT;
    m_surface = EGL_NO_SURFACE;
}

void Overlay::beginFrame() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(m_program);
    glEnableVertexAttribArray(m_posAttrib);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set orthographic projection
    GLint projLoc = glGetUniformLocation(m_program, "uProjection");
    float proj[16] = {
        2.0f/m_width, 0, 0, 0,
        0, -2.0f/m_height, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, proj);
}

void Overlay::endFrame() {
    glDisableVertexAttribArray(m_posAttrib);
    eglSwapBuffers(m_display, m_surface);
}

void Overlay::drawLine(float x1, float y1, float x2, float y2, Color color, float width) {
    float vertices[] = { x1, y1, x2, y2 };
    
    glLineWidth(width);
    glUniform4f(m_colorUniform, color.r, color.g, color.b, color.a);
    glVertexAttribPointer(m_posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glDrawArrays(GL_LINES, 0, 2);
}

void Overlay::drawRect(float x, float y, float w, float h, Color color, float width) {
    float vertices[] = {
        x, y,
        x + w, y,
        x + w, y + h,
        x, y + h,
        x, y
    };
    
    glLineWidth(width);
    glUniform4f(m_colorUniform, color.r, color.g, color.b, color.a);
    glVertexAttribPointer(m_posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glDrawArrays(GL_LINE_STRIP, 0, 5);
}

void Overlay::drawFilledRect(float x, float y, float w, float h, Color color) {
    float vertices[] = {
        x, y,
        x + w, y,
        x, y + h,
        x + w, y + h
    };
    
    glUniform4f(m_colorUniform, color.r, color.g, color.b, color.a);
    glVertexAttribPointer(m_posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Overlay::drawCircle(float x, float y, float radius, Color color, int segments) {
    std::vector<float> vertices;
    vertices.reserve((segments + 1) * 2);
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        vertices.push_back(x + radius * cos(angle));
        vertices.push_back(y + radius * sin(angle));
    }
    
    glUniform4f(m_colorUniform, color.r, color.g, color.b, color.a);
    glVertexAttribPointer(m_posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices.data());
    glDrawArrays(GL_LINE_STRIP, 0, segments + 1);
}

void Overlay::drawBox(float x, float y, float w, float h, Color color) {
    // Outer box
    drawRect(x, y, w, h, color, 2.0f);
    
    // Corner lines
    float cornerLen = w * 0.2f;
    
    // Top-left
    drawLine(x, y, x + cornerLen, y, color, 3.0f);
    drawLine(x, y, x, y + cornerLen, color, 3.0f);
    
    // Top-right
    drawLine(x + w - cornerLen, y, x + w, y, color, 3.0f);
    drawLine(x + w, y, x + w, y + cornerLen, color, 3.0f);
    
    // Bottom-left
    drawLine(x, y + h - cornerLen, x, y + h, color, 3.0f);
    drawLine(x, y + h, x + cornerLen, y + h, color, 3.0f);
    
    // Bottom-right
    drawLine(x + w - cornerLen, y + h, x + w, y + h, color, 3.0f);
    drawLine(x + w, y + h - cornerLen, x + w, y + h, color, 3.0f);
}

void Overlay::drawHealthBar(float x, float y, float w, float h, float health, float maxHealth) {
    float percent = health / maxHealth;
    if (percent > 1.0f) percent = 1.0f;
    if (percent < 0.0f) percent = 0.0f;
    
    // Background
    drawFilledRect(x - 1, y - 1, w + 2, h + 2, Color::Black());
    
    // Health color
    Color healthColor;
    if (percent > 0.6f) healthColor = Color::Green();
    else if (percent > 0.3f) healthColor = Color::Yellow();
    else healthColor = Color::Red();
    
    // Health bar
    drawFilledRect(x, y, w * percent, h, healthColor);
}

void Overlay::drawSnapLine(float x, float y, Color color) {
    drawLine(m_width / 2.0f, m_height, x, y, color, 1.5f);
}

// Global overlay instance
Overlay* g_overlay = nullptr;
