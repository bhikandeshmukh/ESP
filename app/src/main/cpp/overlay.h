#ifndef OVERLAY_H
#define OVERLAY_H

#include "types.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>

struct Color {
    float r, g, b, a;
    Color(float _r = 1, float _g = 1, float _b = 1, float _a = 1) 
        : r(_r), g(_g), b(_b), a(_a) {}
    
    static Color Red() { return Color(1, 0, 0, 1); }
    static Color Green() { return Color(0, 1, 0, 1); }
    static Color Blue() { return Color(0, 0, 1, 1); }
    static Color Yellow() { return Color(1, 1, 0, 1); }
    static Color White() { return Color(1, 1, 1, 1); }
    static Color Black() { return Color(0, 0, 0, 1); }
};

class Overlay {
private:
    ANativeWindow* m_window;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
    
    int m_width;
    int m_height;
    
    GLuint m_program;
    GLuint m_posAttrib;
    GLuint m_colorUniform;
    
    bool initEGL();
    bool initShaders();
    void cleanup();

public:
    Overlay();
    ~Overlay();
    
    bool init(ANativeWindow* window);
    void beginFrame();
    void endFrame();
    
    // Drawing functions
    void drawLine(float x1, float y1, float x2, float y2, Color color, float width = 2.0f);
    void drawRect(float x, float y, float w, float h, Color color, float width = 2.0f);
    void drawFilledRect(float x, float y, float w, float h, Color color);
    void drawCircle(float x, float y, float radius, Color color, int segments = 32);
    
    // ESP specific
    void drawBox(float x, float y, float w, float h, Color color);
    void drawHealthBar(float x, float y, float w, float h, float health, float maxHealth);
    void drawSnapLine(float x, float y, Color color);
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
};

#endif // OVERLAY_H
