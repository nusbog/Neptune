#ifndef NEPTUNE_CORE_H
#define NEPTUNE_CORE_H

typedef struct {
    float topleftx, toplefty;
    float toprightx, toprighty;
    float bottomleftx, bottomlefty;
    float bottomrightx, bottomrighty;
} NBounds;

struct NColor {
    int r, g, b, a;
    float fr, fg, fb, fa;
    unsigned char *cdata;
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
    void draw();
    void setShader(Shader *shader);
    void updateCoords(float posx, float posy, float width, float height);
    void updatePositions(float posx0, float posy0, float posx1, float posy1);
    void getCoords(float *posx, float *posy, float *width, float *height);
    void updateColor(NColor color);
    void updateColorVertices(NColor color0, NColor color1, NColor color2, NColor color3);
    void updateRadius(float radius);
    void updateRadiuses(float radius0, float radius1, float radius2, float radius3);
    bool mouseIsOver();

    NPrimitive *parent = NULL;
    NBounds bounds;
    Shader *shader;
    unsigned int VAO, VBO, EBO;
    float posx, posy;
    float width, height;

    void generateBuffers();
    void bindBuffers();
    void setBuffers();
    void setVertexAttributes();
};

class NRectangle : public NPrimitive {
};

class NImage : public NPrimitive{
public:
    void createImage();
    void blit(int posX, int posY, int width, int height, unsigned char *data);
private:
    unsigned int texture;
};

void neptune_init();
void neptune_start();

// CALLBACKS
void neptune_getMousePosition(double *x, double *y);
void neptune_onPaintCallback(void (*_onPaint)());
void neptune_onScrollCallback(void (*_onScroll)(int dir));
void neptune_onClickCallback(void (*_onClick)(int button, int action));

void neptune_updateProjectionMatrix();
void neptune_getScreenSize(float *_scr_width, float *_scr_height);

Shader neptune_loadShader(char *vShaderPath, char *fShaderPath);

#endif