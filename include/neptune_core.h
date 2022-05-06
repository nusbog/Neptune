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
    unsigned char *cdata;
    NColor(int r, int g, int b, int a = 255) {
        cdata = (unsigned char *)malloc(4 * sizeof(char));
        cdata[0] = this->r = r;
        cdata[1] = this->g = g;
        cdata[2] = this->b = b;
        cdata[3] = this->a = a;
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

typedef class {
public:
    void init(float posx, float posy, float width, float height);
    void draw();
    void setShader(Shader *shader);
    void updateCoords(float posx, float posy, float width, float height);
    void updatePositions(float posx0, float posy0, float posx1, float posy1);
    void getCoords(float *posx, float *posy, float *width, float *height);
    void updateColor(NColor color);

    NBounds bounds;
private:
    Shader *shader;
    unsigned int VAO, VBO;
    float posx, posy;
    float width, height;
    void generateBuffers();
    void bindBuffers();
    void setBuffers();
    void setVertexAttributes();
} NRectangle;

typedef class {
public:
    void init(float posx, float posy, float width, float height);
    void draw();
    void updateCoords(float posx, float posy, float width, float height);
    void blit(int offsetx, int offsety, int width, int height, unsigned char *data);
    void setShader(Shader *shader);
private:
    void generateBuffers();
    void bindBuffers();
    void setBuffers();
    void setVertexAttributes();
    void bindTexture();
    Shader *shader;
    unsigned int VBO, VAO, EBO;
    unsigned int texture;
    float posx, posy;
    float width, height;
} NImage;

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