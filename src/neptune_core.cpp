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
float scr_width  = 800.0;
float scr_height = 600.0;
float scrollSpeed = 1.0;

GLFWwindow* window;

glm::mat4 proj;
glm::mat4 view;

int rectCount = 0;

void (*onPaint)();
void (*onScroll)(int dir);
void (*onClick)(int button, int action);

void neptune_updateProjectionMatrix() {
    proj = glm::ortho(-scr_width, scr_width, -scr_height, scr_height, 0.1f, 100.0f);
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
    glfwGetCursorPos(window, x, y);
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

// NRECTANGLE
void NRectangle::setVertexAttributes() {
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(12 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void NRectangle::bindBuffers() {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
}

void NRectangle::setBuffers() {
    float vertices[] = {
         posx + width, posy + height, // top right
         posx + width, posy         , // bottom right
         posx        , posy         , // bottom left

         posx        , posy + height, // top left 
         posx        , posy         , // bottom left
         posx + width, posy + height, // top right

         1.0, 1.0, 1.0, 1.0,
         1.0, 1.0, 1.0, 1.0,
         1.0, 1.0, 1.0, 1.0,
         1.0, 1.0, 1.0, 1.0,
         1.0, 1.0, 1.0, 1.0,
         1.0, 1.0, 1.0, 1.0,
    };

    this->bounds = (NBounds){ 
        posx        , posy + height, 
        posx + width, posy + height,
        posx        , posy,
        posx + width, posy };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void NRectangle::generateBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

void NRectangle::init(float posx, float posy, float width, float height) {
    this->posx = posx;
    this->posy = posy;
    this->width = width;
    this->height = height;

    generateBuffers();
    bindBuffers();
    setBuffers();

    setVertexAttributes();
}

void NRectangle::setShader(Shader *shader) {
    this->shader = shader;}

void NRectangle::draw() {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glUseProgram(shader->program);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void NRectangle::updateColor(NColor color) {
    float r, g, b, a;
    r = color.r / 255.0;
    g = color.g / 255.0;
    b = color.b / 255.0;
    a = color.a / 255.0;

    float colors[] = {
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(float), sizeof(colors), colors);
}

void NRectangle::updatePositions(float posx0, float posy0, float posx1, float posy1) {
    bindBuffers();

    float vertices[] = {
        posx1, posy1, // top right
        posx1, posy0, // bottom right
        posx0, posy0, // bottom left

        posx0, posy1, // top left 
        posx0, posy0, // bottom left
        posx1, posy1, // top right
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

void NRectangle::updateCoords(float posx, float posy, float width, float height) {
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
        posx        , posy         , // bottom left
        posx + width, posy + height, // top right
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

void NRectangle::getCoords(float *posx, float *posy, float *width, float *height) {
    *posx = this->posx;
    *posy = this->posy;
    *width = this->width;
    *height = this->height;
}

// NIMAGE
void NImage::generateBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenTextures(1, &texture);
}

void NImage::bindBuffers() {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
}

void NImage::setBuffers() {
    float vertices[] = {
         posx + width, posy + height, 1.0f, 0.0f, // top right
         posx + width, posy         , 1.0f, 1.0f, // bottom right
         posx        , posy         , 0.0f, 1.0f, // bottom left
         posx        , posy + height, 0.0f, 0.0f, // top left 
    };

    unsigned int indices[] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void NImage::setVertexAttributes() {
    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void NImage::bindTexture() {
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int datasize = width * height * 4;
    unsigned int w, h;
    unsigned char *data = (unsigned char *)calloc(datasize, datasize);

    // Texture Loading
    //int success = lodepng_decode32_file(&data, &w, &h, "idle.png");
    // TODO: Resize, ändra width & height
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void NImage::blit(int offsetx, int offsety, int width, int height, unsigned char *data) {
    bindBuffers();
    glTexSubImage2D(GL_TEXTURE_2D, 0, offsetx, offsety, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
}

void NImage::updateCoords(float posx, float posy, float width, float height) {
    this->posx = posx;
    this->posy = posy;
    this->width = width;
    this->height = height;

    bindBuffers();
    setBuffers();
}

void NImage::init(float posx, float posy, float width, float height) {
    this->posx = posx;
    this->posy = posy;
    this->width = width;
    this->height = height;

    generateBuffers();
    bindBuffers();
    setBuffers();
    setVertexAttributes();
    bindTexture();
}

void NImage::setShader(Shader *shader) {
    this->shader = shader;
}

void NImage::draw() {
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(shader->program);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void renderLoop() {
    while (!glfwWindowShouldClose(window))
    {

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

    glClearColor(45.0 / 255.0, 50.0 / 255.0, 80.0 / 255.0, 1.0f);

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
