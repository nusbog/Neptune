#ifndef NEPTUNE_CORE_H
#define NEPTUNE_CORE_H

#include <iostream>
extern "C" {
    #include <atlas.h>
}

typedef struct {
    float topleftx, toplefty;
    float toprightx, toprighty;
    float bottomleftx, bottomlefty;
    float bottomrightx, bottomrighty;
} NBounds;

class NColor {
public:
    int r, g, b, a;
    float fr, fg, fb, fa;
    unsigned char *cdata; // TODO: Free
    NColor(int r, int g, int b, int a = 255) {
        cdata = (unsigned char *)malloc(4 * sizeof(char));
        cdata[0] = this->r = r;
        cdata[1] = this->g = g;
        cdata[2] = this->b = b;
        cdata[3] = this->a = a;

        fr = r / 255.0;
        fg = g / 255.0;
        fb = b / 255.0;
        fa = a / 255.0;
    }

    NColor(float fr, float fg, float fb, float fa = 1.0) {
        cdata = (unsigned char *)malloc(4 * sizeof(char));
        cdata[0] = this->r = fr * 255.0;
        cdata[1] = this->g = fg * 255.0;
        cdata[2] = this->b = fb * 255.0;
        cdata[3] = this->a = fa * 255.0;

        this->fr = fr;
        this->fg = fg;
        this->fb = fb;
        this->fa = fa;
    }
};

class NHsvColor {
public:
    float h, s, v;
};

typedef class {
public:
    unsigned int vShader, fShader, program;
    void getShaderUniformLocations();
    void pushMatricesToShader();
private:
    // Uniform Locations
    int projLoc, viewLoc;
} Shader;

class NPrimitive {
public:
    void init(float posx, float posy, float width, float height);
    void setShader(Shader *shader);
    void updateCoords(float posx, float posy, float width, float height);
    void updatePositions(float posx0, float posy0, float posx1, float posy1);
    void getCoords(float *posx, float *posy, float *width, float *height);
    void updateColor(NColor color);
    NColor getColor();
    void updateColorVertices(NColor color0, NColor color1, NColor color2, NColor color3);
    bool mouseIsOver();
    void bindBuffers();

    NPrimitive *parent = NULL;
    Shader *shader;
    float posx, posy;
    float width, height;

private:
    void generateBuffers();
    void setBuffers();
    void setVertexAttributes();
    
    unsigned int VAO, VBO, EBO;
    NBounds bounds;
    NColor color = NColor(255, 255, 255);
};

class NRectangle : public NPrimitive {
public:
    void draw();
    void updateRadius(float radius);
    void updateRadiuses(float radius0, float radius1, float radius2, float radius3);
};

class NImage : public NPrimitive{
public:
    void draw();
    void blit(int posX, int posY, int width, int height, unsigned char *data);
    void bindTexture(Texture *texture);
private:
    Texture *texture = NULL;
};

void neptune_init();
void neptune_start();
void neptune_pushAtlasToGPU(Atlas *atlas);
void neptune_resizeModelCanvas(float x, float y);

// CALLBACKS
void neptune_getMousePosition(double *x, double *y);
void neptune_onPaintCallback(void (*_onPaint)());
void neptune_onScrollCallback(void (*_onScroll)(int dir));
void neptune_onClickCallback(void (*_onClick)(int button, int action));
void neptune_onKeyCallback(void (*_onKey)(int key, int action, int mods));
void neptune_onPixelatedCallback(void (*_onPixelatedPaint)());

void neptune_updateProjectionMatrix();
void neptune_getScreenSize(float *_scr_width, float *_scr_height);

Shader neptune_loadShader(char *vShaderPath, char *fShaderPath);

// 3D
void neptune_setModel(char *modelPath);
void neptune_drawModel(float x, float y, float rot = 0.5f, int xRot = 0, int yRot = 1, int zRot = 1);
void neptune_resizeModelCanvas(float w, float h);

#endif