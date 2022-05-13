#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <neptune_filemanagment.h>
#include <neptune_core.h>

#include <iostream>
#include <lodepng.h>

// Window Events
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

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

double mousePosX;
double mousePosY;

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
        std::cout << "Uniform \"view\" was not found\n";
    }
    if(projLoc == -1) {
        std::cout << "Uniform \"proj\" was not found\n";
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

void NPrimitive::draw() {
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

void NPrimitive::updateColor(NColor color) {
    bindBuffers();

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

void NPrimitive::updateRadius(float radius) {
    bindBuffers();

    float radiuses[] = {
        radius,
        radius,
        radius,
        radius,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 40 * sizeof(float), sizeof(radiuses), radiuses);
}

void NPrimitive::updateRadiuses(float radius0, float radius1, float radius2, float radius3) {
    bindBuffers();

    float radiuses[] = {
        radius0,
        radius1,
        radius2,
        radius3,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 40 * sizeof(float), sizeof(radiuses), radiuses);
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

    if(mousePosX > posx && mousePosX < posx + width) {
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
}

// NIMAGE
void NImage::createImage() {
    bindBuffers();

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int datasize = 32 * 32 * 4;
    // unsigned int w, h;
    unsigned char *data = (unsigned char *)calloc(datasize, datasize);

    // Texture Loading
    //int success = lodepng_decode32_file(&data, &w, &h, "idle.png");
    // TODO: Resize, ändra width & height

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(data);
}

void NImage::blit(int posX, int posY, int width, int height, unsigned char *data) {
    bindBuffers();
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexSubImage2D(GL_TEXTURE_2D, 0, posX, posY, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
}

void renderLoop() {
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    while (!glfwWindowShouldClose(window))
    {
        glfwGetCursorPos(window, &mousePosX, &mousePosY);

        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT);

        if(onPaint != NULL) {
            onPaint();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void neptune_init()
{
    setupGLFW();
    setupGlad();
    glClearColor(32.0 / 255.0, 31.0 / 255.0, 40.0 / 255.0, 1.0f);

    neptune_updateProjectionMatrix();
    setZoom(-4.0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
}

void neptune_start() {
    renderLoop();
    glfwTerminate();
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(onScroll != NULL) {
        onScroll((int)yoffset);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    scr_width  = width;
    scr_height = height;
    glViewport(0, 0, width, height);
    neptune_updateProjectionMatrix();
}
