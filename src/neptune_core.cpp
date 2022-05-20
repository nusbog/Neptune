#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <models/shader_m.h>
#include <models/camera.h>
#include <models/model.h>

#include <neptune_filemanagment.h>
#include <neptune_core.h>

#include <iostream>
#include <limits>

extern "C" {
    #include <atlas.h>
}

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

// Window Events
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

// Settings
float scr_width  = 1200.0;
float scr_height = 800.0;
float scrollSpeed = 1.0;

GLFWwindow* window;

glm::mat4 proj;
glm::mat4 view;

int rectCount = 0;

void (*onPaint)();
void (*onScroll)(int dir);
void (*onClick)(int button, int action);
void (*onKey)(int key, int action, int mods);
void (*onPixelatedPaint)();

double mousePosX;
double mousePosY;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

void neptune_updateProjectionMatrix() {
    proj = glm::ortho(0.0f, scr_width, 0.0f, scr_height, 0.1f, 100.0f);
}

void setZoom(float zoomAmount) {
    view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, zoomAmount));
}

void neptune_getScreenSize(float *_scr_width, float *_scr_height) {
    *_scr_width = scr_width;
    *_scr_height = scr_height;
}

void neptune_getMousePosition(double *x, double *y) {
    *x = mousePosX;
    *y = mousePosY;
}

void neptune_onPaintCallback(void (*_onPaint)()) {
    onPaint = _onPaint;
}

void neptune_onScrollCallback(void (*_onScroll)(int dir)) {
    onScroll = _onScroll;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    onClick(button, action);
}

void neptune_onClickCallback(void (*_onClick)(int button, int action)) {
    onClick = _onClick;

    glfwSetMouseButtonCallback(window, mouse_button_callback);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    onKey(key, action, mods);
}

void neptune_onKeyCallback(void (*_onKey)(int key, int action, int mods)) {
    onKey = _onKey;

    glfwSetKeyCallback(window, key_callback);
}

void neptune_onPixelatedCallback(void (*_onPixelatedPaint)()) {
    onPixelatedPaint = _onPixelatedPaint;
}

// WINDOW MANAGMENT
int setupGLFW() {
    // Configure GLFW Settings
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    window = glfwCreateWindow(scr_width, scr_height, "Neptune", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    return 0;
}

int setupGlad() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    return 0;
}


// SHADER
void Shader::getShaderUniformLocations() {
    viewLoc = glGetUniformLocation(program, "view");
    projLoc = glGetUniformLocation(program, "proj");
    if(viewLoc == -1) {
        // std::cout << "Uniform \"view\" was not found\n";
    }
    if(projLoc == -1) {
        // std::cout << "Uniform \"proj\" was not found\n";
    }
}

void Shader::pushMatricesToShader() {
    glUseProgram(program);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
}

int shaderErrorCheck(unsigned int shader) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);

        std::cout << "ERROR: SHADER FAILED" << infoLog << std::endl;
        return -1;
    }
    return 0;
}

void createShader(unsigned int *shader, const char *shaderCode, int type) {
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &shaderCode, NULL);
    glCompileShader(*shader);
    shaderErrorCheck(*shader);
}

void createShaderProgram(Shader *shader) {
    shader->program = glCreateProgram();
    glAttachShader(shader->program, shader->vShader);
    glAttachShader(shader->program, shader->fShader);
    glLinkProgram(shader->program);
    shaderErrorCheck(shader->program);
}

Shader loadShaderFromMemory(const char *vShaderCode, const char *fShaderCode) {
    Shader shader;

    createShader(&shader.vShader, vShaderCode, GL_VERTEX_SHADER);
    createShader(&shader.fShader, fShaderCode, GL_FRAGMENT_SHADER);
    createShaderProgram(&shader);

    glDeleteShader(shader.vShader);
    glDeleteShader(shader.fShader);

    return shader;
}

Shader neptune_loadShader(char *vShaderPath, char *fShaderPath) {
    Shader shader;

    char *vShaderCode = ReadContentFromFile(vShaderPath);
    char *fShaderCode = ReadContentFromFile(fShaderPath);

    shader = loadShaderFromMemory(vShaderCode, fShaderCode);
    glUseProgram(shader.program);

    return shader;
}

// NPrimitive
void NPrimitive::setVertexAttributes() {
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(24 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(32 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)(40 * sizeof(float)));
    glEnableVertexAttribArray(4);
}

void NPrimitive::bindBuffers() {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
}

void NPrimitive::setBuffers() {
    float vertices[] = {
        // Vertex Positions
        posx + width, posy + height, // top right
        posx + width, posy         , // bottom right
        posx        , posy         , // bottom left
        posx        , posy + height, // top left 

        // Vertex Colors
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,

        // Texture Coordinates
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,

        // Dimensions
        width, height,
        width, height,
        width, height,
        width, height,

        // Radiuses
        0.0,
        0.0,
        0.0,
        0.0,
    };

    int indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    this->bounds = (NBounds){ 
        posx        , posy + height, 
        posx + width, posy + height,
        posx        , posy,
        posx + width, posy };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void NPrimitive::generateBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

void NPrimitive::init(float posx, float posy, float width, float height) {
    this->posx = posx;
    this->posy = posy;
    this->width = width;
    this->height = height;

    generateBuffers();
    bindBuffers();
    setBuffers();

    setVertexAttributes();
}

void NPrimitive::setShader(Shader *shader) {
    this->shader = shader;
} 

void NRectangle::draw() {
    bindBuffers();

    if(parent != NULL) {
        float vertices[] = {
            parent->posx + posx + width, parent->posy + posy + height, // top right
            parent->posx + posx + width, parent->posy + posy         , // bottom right
            parent->posx + posx        , parent->posy + posy         , // bottom left
            parent->posx + posx        , parent->posy + posy + height, // top left 
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shader->program);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void NRectangle::updateRadius(float radius) {
    bindBuffers();

    float radiuses[] = {
        radius,
        radius,
        radius,
        radius,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 40 * sizeof(float), sizeof(radiuses), radiuses);
}

void NRectangle::updateRadiuses(float radius0, float radius1, float radius2, float radius3) {
    bindBuffers();

    float radiuses[] = {
        radius0,
        radius1,
        radius2,
        radius3,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 40 * sizeof(float), sizeof(radiuses), radiuses);
}

void NImage::draw() {
    bindBuffers();

    if(parent != NULL) {
        float vertices[] = {
            parent->posx + posx + width, parent->posy + posy + height, // top right
            parent->posx + posx + width, parent->posy + posy         , // bottom right
            parent->posx + posx        , parent->posy + posy         , // bottom left
            parent->posx + posx        , parent->posy + posy + height, // top left 
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glUseProgram(shader->program);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

NColor NPrimitive::getColor() {
    return this->color;
}

void NPrimitive::updateColor(NColor color) {
    bindBuffers();
    this->color = color;

    float r, g, b, a;
    r = color.fr;
    g = color.fg;
    b = color.fb;
    a = color.fa;

    float colors[] = {
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 8 * sizeof(float), sizeof(colors), colors);
}

void NPrimitive::updateColorVertices(NColor color0, NColor color1, NColor color2, NColor color3) {
    bindBuffers();

    float colors[] = {
        color0.fr, color0.fg, color0.fb, color0.fa,
        color1.fr, color1.fg, color1.fb, color1.fa,
        color2.fr, color2.fg, color2.fb, color2.fa,
        color3.fr, color3.fg, color3.fb, color3.fa,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 8 * sizeof(float), sizeof(colors), colors);
}

void NPrimitive::updatePositions(float posx0, float posy0, float posx1, float posy1) {
    bindBuffers();

    float vertices[] = {
        posx1, posy1, // top right
        posx1, posy0, // bottom right
        posx0, posy0, // bottom left
        posx0, posy1, // top left 
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

void NPrimitive::updateCoords(float posx, float posy, float width, float height) {
    bindBuffers();

    this->posx = posx;
    this->posy = posy;
    this->width = width;
    this->height = height;

    float vertices[] = {
        posx + width, posy + height, // top right
        posx + width, posy         , // bottom right
        posx        , posy         , // bottom left
        posx        , posy + height, // top left 
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

bool NPrimitive::mouseIsOver() {
    bool isOver = false;

    float transformedPosY = scr_height - posy;
    float transformedPosX = posx;

    if(parent != NULL) {
        float p_posx, p_posy, p_width, p_height;
        parent->getCoords(&p_posx, &p_posy, &p_width, &p_height);

        transformedPosX += p_posx;
        transformedPosY -= p_posy;
    }

    if(mousePosX > transformedPosX && mousePosX < transformedPosX + width) {
        if(mousePosY < transformedPosY && mousePosY > transformedPosY - height) {
            isOver = true;
        }
    }

    return isOver;
}

void NPrimitive::getCoords(float *posx, float *posy, float *width, float *height) {
    *posx = this->posx;
    *posy = this->posy;
    *width = this->width;
    *height = this->height;

    if(parent != NULL) {
        float p_posx, p_posy, p_width, p_height;
        parent->getCoords(&p_posx, &p_posy, &p_width, &p_height);

        *posx += p_posx;
        *posy += p_posy;
    }
}

// NIMAGE
void NImage::blit(int posX, int posY, int width, int height, unsigned char *data) {
    bindBuffers();

    if(texture != NULL) {
        posX += texture->x_atlas_coord;
        posY += texture->y_atlas_coord;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, posX, posY, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
}

void NImage::bindTexture(Texture *texture) {
    this->texture = texture;

    bindBuffers();

    float texCoords[] = {
        texture->x_tex_coord + texture->w_tex_coord, texture->y_tex_coord,
        texture->x_tex_coord + texture->w_tex_coord, texture->y_tex_coord + texture->h_tex_coord,
        texture->x_tex_coord                       , texture->y_tex_coord + texture->h_tex_coord,
        texture->x_tex_coord                       , texture->y_tex_coord,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 24 * sizeof(float), sizeof(texCoords), texCoords);
}

Shader screenQuadShader;
Shader pixelScreenShader;
int fboCount = 0;
unsigned int quadVAO, quadVBO, framebuffer, textureColorbuffer, rbo;
unsigned int quadVAO0, quadVBO0, framebuffer0, textureColorbuffer0, rbo0;

void createScreenQuadObjectFramebuffer() {

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void createFramebuffer() {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // Create a color attachment texture
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scr_width, scr_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scr_width, scr_height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    fboCount++;
}

void beginFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    // make sure we clear the framebuffer's content
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST | GL_BLEND);
}

void endFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glDisable(GL_DEPTH_TEST);
}

void drawFramebuffer() {
    glUseProgram(screenQuadShader.program);

    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);   // use the color attachment texture as the texture of the quad plane
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void createScreenQuadObjectFramebuffer0() {
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        0.0f      , scr_height,  0.0f, 1.0f,
        0.0f, 0.0f, 0.0f     ,  0.0f,
        scr_width , 0.0f     ,  1.0f, 0.0f,

        0.0f     , scr_height,  0.0f, 1.0f,
        scr_width, 0.0f     ,  1.0f, 0.0f,
        scr_width, scr_height,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO0);
    glGenBuffers(1, &quadVBO0);
    glBindVertexArray(quadVAO0);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void createFramebuffer0() {
    glGenFramebuffers(1, &framebuffer0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer0);
    // Create a color attachment texture
    glGenTextures(1, &textureColorbuffer0);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scr_width / 6.0f, scr_height / 6.0f, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer0, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    glGenRenderbuffers(1, &rbo0);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo0);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scr_width / 6.0f, scr_height / 6.0f); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo0); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    fboCount++;
}
void beginFramebuffer0() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer0);
    // glBindRenderbuffer(GL_RENDERBUFFER, rbo0);

    // make sure we clear the framebuffer's content
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST | GL_BLEND);
}

void endFramebuffer0() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(quadVAO0);
    glDisable(GL_DEPTH_TEST);
}

void drawFramebuffer0() {
    glUseProgram(pixelScreenShader.program);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer0);   // use the color attachment texture as the texture of the quad plane
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

MShader modelShader;
Model currModel;
void neptune_setModel(char *modelPath) {
    currModel.create(modelPath);
}

void neptune_drawModel(float x, float y, float rot, int xRot, int yRot, int zRot) {
    if(!currModel.isLoaded) {
        return;
    }

    modelShader.use();

    glm::mat4 projection = glm::ortho(0.0f, (float)scr_width, 0.0f, (float)scr_height, 0.0f, numeric_limits<float>::max());
    glm::mat4 view = camera.GetViewMatrix();
    modelShader.setMat4("projection", projection);
    modelShader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f)); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(16.0f, 16.0f, 16.0f)); // it's a bit too big for our scene, so scale it down
    model = glm::rotate(model, rot, glm::vec3(xRot, yRot, zRot));
    modelShader.setMat4("model", model);
    currModel.Draw(modelShader);
}

void neptune_resizeModelCanvas(float w, float h) {
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo0);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo0); // now actually attach it
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int textureid;
void renderLoop() {
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    // Set active framebuffers
    GLenum bufs[] = { GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(2, bufs);

    // Init shader
    char path1[] = "resources/shaders/screenshader.vs";
    char path2[] = "resources/shaders/pixelscreenshader.vs";
    char path3[] = "resources/shaders/screenshader.fs";
    screenQuadShader = neptune_loadShader(path1, path3);
    screenQuadShader.getShaderUniformLocations();
    screenQuadShader.pushMatricesToShader();

    pixelScreenShader = neptune_loadShader(path2, path3);
    pixelScreenShader.getShaderUniformLocations();
    pixelScreenShader.pushMatricesToShader();
    modelShader.create("1.model_loading.vs", "1.model_loading.fs");

    createScreenQuadObjectFramebuffer();
    createFramebuffer();

    createScreenQuadObjectFramebuffer0();
    createFramebuffer0(); 

    while (!glfwWindowShouldClose(window))
    {
        glfwGetCursorPos(window, &mousePosX, &mousePosY);

        glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        beginFramebuffer();
        if(onPaint != NULL) {
            glBindTexture(GL_TEXTURE_2D, textureid);
            onPaint();
        }

        endFramebuffer();
        drawFramebuffer();
        beginFramebuffer0();

        if(onPixelatedPaint != NULL) {
            onPixelatedPaint();
        }

        endFramebuffer0();
        drawFramebuffer0();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

// When file is dropped on the window
void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if(paths[0] == NULL) {
        return;
    }

    std::string fileNameStr = std::string(paths[0]);
    std::string fileExt = fileNameStr.substr(fileNameStr.length() - 3);

    if(fileExt == "obj") {
        neptune_setModel((char*)paths[0]);
    } else if(fileExt == "png") {
        
    }
}

void neptune_init()
{  
    setupGLFW();
    setupGlad();
    glClearColor(32.0 / 255.0, 31.0 / 255.0, 40.0 / 255.0, 1.0f);

    neptune_updateProjectionMatrix();
    setZoom(-4.0);

    glfwSetDropCallback(window, drop_callback);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
}

void neptune_start() {
    renderLoop();
    glfwTerminate();
}

void neptune_pushAtlasToGPU(Atlas *atlas) {
    glGenTextures(1, &textureid);
    glBindTexture(GL_TEXTURE_2D, textureid);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->w, atlas->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(onScroll != NULL) {
        onScroll((int)yoffset);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    scr_width  = width;
    scr_height = height;
    glViewport(0, 0, width, height);
    neptune_updateProjectionMatrix();

    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}