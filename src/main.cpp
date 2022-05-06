#include <iostream>
#include <GLFW/glfw3.h>
#include <neptune_core.h>
#include <neptune_math.h>
#include <bits/stdc++.h>
#include <windows.h>

Shader backgroundShader;
Shader basicShader;
Shader texShader;

NRectangle selectedPixel;
NRectangle background;
NImage img;

int resolutionx = 32;
int resolutiony = 32;
float zoom = 2.0;

float lastMousePosX = 0.0;
float lastMousePosY = 0.0;

int canvasposx = 0;
int canvasposy = 0;

bool lmbdown = false;
bool rmbdown = false;

float lmbdownposx = 0.0;
float lmbdownposy = 0.0;

NColor currColor(255, 0, 0);
NColor selectedPixelColor(100, 100, 180, 100);
NColor selectionColor(100, 100, 180, 100);

void drawPixel(int x, int y, NColor color) {
    img.blit(x, y, 1, 1, color.cdata);
}

void drawline(int x0, int y0, int x1, int y1)
{
    int dx, dy, p, x, y;
     
    dx = x1 - x0;
    dy = y1 - y0;
     
    x = x0;
    y = y0;
     
    p = 2 * dy - dx;
     
    while(x < x1) {
        if(p >= 0) {
            drawPixel(x, y, currColor);
            y = y + 1;
            p = p + 2 * dy - 2 * dx;
        }
        else {
            drawPixel(x, y, currColor);
            p = p + 2 * dy;
        }
        x = x + 1;
    }
}

void setSelectedPixelLocation() {
    double mousePosX, mousePosY;
    float screenWidth, screenHeight;

    neptune_getMousePosition(&mousePosX, &mousePosY);
    neptune_getScreenSize(&screenWidth, &screenHeight);

    float x = mousePosX - (screenWidth / 2.0);
    float y = screenHeight - mousePosY - (screenHeight / 2.0);

    x *= 2.0;
    x = floor(x / zoom) * zoom;

    y *= 2.0;
    y = floor(y / zoom) * zoom;

    if(lastMousePosX != x || lastMousePosY != y) {
        canvasposx = x / zoom + resolutionx / 2.0;
        canvasposy = resolutiony - y / zoom + resolutiony / 2.0 - resolutiony - 1;

        if(!lmbdown) {
            selectedPixel.updateCoords(x, y, zoom, zoom);
        }
    }

    // Draw Pixel
    if(lmbdown) {
        // drawPixel(canvasposx, canvasposy, color);
        selectedPixel.updatePositions(lmbdownposx, lmbdownposy + zoom, x, y);
    }

    // Delete Pixel
    if(rmbdown) {
        NColor color(0, 0, 0, 0);
        drawPixel(canvasposx, canvasposy, color);
    }
 
    lastMousePosX = x;
    lastMousePosY = y;
}

void zoomCanvas(int dir) {
    zoom += dir;
    float fixedResolutionx = resolutionx * zoom;
    float fixedResolutiony = resolutiony * zoom;

    background.updateCoords(-fixedResolutionx / 2, -fixedResolutiony / 2, fixedResolutionx, fixedResolutiony);
    img.updateCoords(-fixedResolutionx / 2, -fixedResolutiony / 2, fixedResolutionx, fixedResolutiony);
}

void onPaint() {
    neptune_updateProjectionMatrix();

    backgroundShader.pushMatricesToShader();
    background.draw();

    texShader.pushMatricesToShader();
    img.draw();

    setSelectedPixelLocation();
    basicShader.pushMatricesToShader();
    selectedPixel.draw();
}

void setupShaders() {
    char path1[] = "resources/shaders/textureshader.vs";
    char path2[] = "resources/shaders/textureshader.fs";
    texShader = neptune_loadShader(path1, path2);
    texShader.getShaderUniformLocations();
    texShader.pushMatricesToShader();

    char path3[] = "resources/shaders/vertexshader.vs";
    char path4[] = "resources/shaders/backgroundshader.fs";
    backgroundShader = neptune_loadShader(path3, path4);
    backgroundShader.getShaderUniformLocations();
    backgroundShader.pushMatricesToShader();

    char path5[] = "resources/shaders/vertexshader.vs";
    char path6[] = "resources/shaders/fragmentshader.fs";
    basicShader = neptune_loadShader(path5, path6);
    basicShader.getShaderUniformLocations();
    basicShader.pushMatricesToShader();

    img.init(0, 0, resolutionx, resolutiony);
    img.setShader(&texShader);

    background.init(-100, -100, 200, 200);
    background.setShader(&backgroundShader);
    zoomCanvas(0.0);

    selectedPixel.init(-100, -100, 200, 200);
    selectedPixel.setShader(&basicShader);
    selectedPixel.updateColor(selectedPixelColor);
}

void mouseClickCallback(int button, int action) {
    if(button == GLFW_MOUSE_BUTTON_RIGHT) {
        if(action == GLFW_PRESS) {
            rmbdown = true;
        } else if(action == GLFW_RELEASE) {
            rmbdown = false;
        }    
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
        if(action == GLFW_PRESS) {
            lmbdownposx = lastMousePosX;
            lmbdownposy = lastMousePosY;
            lmbdown = true;
        } else if(action == GLFW_RELEASE) {
            lmbdown = false;
        }
    }
}

int main() {
    neptune_init();

    neptune_onPaintCallback(onPaint);
    neptune_onScrollCallback(zoomCanvas);
    neptune_onClickCallback(mouseClickCallback);

    setupShaders();

    neptune_start();

    return 0;
}