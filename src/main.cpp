#include <iostream>
#include <windows.h>
#include <bits/stdc++.h>

#include <GLFW/glfw3.h>

#include <neptune_core.h>
#include <neptune_math.h>

#include <resourcemanager.h>
#include <rundata.h>

#define TOOL_COUNT 2

#define PEN_TOOL 0
#define SELECT_TOOL 1

Shader backgroundShader;
Shader basicShader;
Shader texShader;
Shader colorSelectionShader;
Shader rainbowRectShader;

NRectangle toolBar;
NRectangle selectedPixel;
NRectangle background;
NRectangle colorWheel;

NImage img;

struct ColorSelection {
    NRectangle base;
    NRectangle colorQuad;
    NRectangle colorSlider;

    NRectangle hexQuad;
    NRectangle rQuad, gQuad, bQuad, aQuad;

    NImage eyeDropperIcon;
};
ColorSelection colorSelection;

struct Tool {
    NRectangle icon;
    void (*callbackFunc)();
};
Tool tools[TOOL_COUNT];
Tool *currTool;

// Real pixel size
int resolutionx = 32;
int resolutiony = 32;

float currMousePosX = 0.0;
float currMousePosY = 0.0;

// Mouse position last frame
float lastMousePosX = 0.0;
float lastMousePosY = 0.0;

// Mouse position mapped within the canvas resolution where 0,0 is canvas' bottom left
int canvasposx = 0;
int canvasposy = 0;

// Mouse position mapped within the canvas resolution where 0,0 is window's bottom left 
int screenSpacePixelMousePosX = 0;
int screenSpacePixelMousePosY = 0;

// The mouse position where the latest left mouse button was pressed at
float lmbdownposx = 0.0;
float lmbdownposy = 0.0;

bool lmbdown = false;
bool rmbdown = false;

// Canvas will be rendered at the size of resolution * zoom
float zoom = 2.0;

NColor currColor(50, 0, 0);
NColor selectedPixelColor(100, 120, 140, 160);
NColor selectionColor(42, 41, 50, 0);

// CALLBACKS
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

void zoomCanvas(int dir) {
    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    zoom += dir;
    float fixedResolutionx = resolutionx * zoom;
    float fixedResolutiony = resolutiony * zoom;

    background.updateCoords(screenWidth / 2.0f - fixedResolutionx / 2.0, screenHeight / 2.0f - fixedResolutiony / 2.0, fixedResolutionx, fixedResolutiony);
    img.updateCoords(screenWidth / 2.0f - fixedResolutionx / 2.0, screenHeight / 2.0f - fixedResolutiony / 2.0, fixedResolutionx, fixedResolutiony);
}

// RENDERING
void drawPixel(int x, int y, NColor color) {
    img.blit(x, y, 1, 1, color.cdata);
}

void drawline(int x0, int y0, int x1, int y1) {
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

void penToolUpdate() {
    if(lmbdown) {
        drawPixel(canvasposx, canvasposy, NColor(255, 255, 255));
    }

    // Delete Pixel if right mouse button is pressed
    if(rmbdown) {
        NColor color(0, 0, 0, 0);
        drawPixel(canvasposx, canvasposy, color);
    }
}

void selectToolUpdate() {
    if(!lmbdown)
        return;

    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    selectedPixel.updatePositions(lmbdownposx, screenHeight - lmbdownposy, currMousePosX, screenHeight - currMousePosY);
}

void setSelectedPixelLocation() {
    double mousePosX, mousePosY;
    float screenWidth, screenHeight;
    float currCanvasPosX, currCanvasPosY, currCanvasWidth, currCanvasHeight;

    neptune_getMousePosition(&mousePosX, &mousePosY);
    neptune_getScreenSize(&screenWidth, &screenHeight);
    background.getCoords(&currCanvasPosX, &currCanvasPosY, &currCanvasWidth, &currCanvasHeight);

    currMousePosX = mousePosX;
    currMousePosY = mousePosY;

    canvasposx = floor((currMousePosX - currCanvasPosX) / (zoom));
    canvasposy = floor((currMousePosY - currCanvasPosY) / (zoom));

    if(lastMousePosX != currMousePosX || lastMousePosY != currMousePosY) {
        if(!lmbdown) {
            screenSpacePixelMousePosX = canvasposx * zoom;
            screenSpacePixelMousePosX += currCanvasPosX;

            screenSpacePixelMousePosY = canvasposy * zoom + zoom;
            screenSpacePixelMousePosY += currCanvasPosY;

            selectedPixel.updateCoords(screenSpacePixelMousePosX, screenHeight - screenSpacePixelMousePosY, zoom, zoom);
        }
    }

    currTool->callbackFunc();

    lastMousePosX = currMousePosX;
    lastMousePosY = currMousePosY;
}

void colorWheelUpdate() {
    if(!colorWheel.mouseIsOver())
        return;
    if(!lmbdown)
        return;

    std::cout << "yo\n";
}

void colorSelectionUpdate() {
    colorSelection.base.draw();
    colorSelection.colorQuad.draw();
    colorSelection.colorSlider.draw();
}

void onPaint() {
    neptune_updateProjectionMatrix();
    setSelectedPixelLocation();

    backgroundShader.pushMatricesToShader();
    basicShader.pushMatricesToShader();
    texShader.pushMatricesToShader();
    colorSelectionShader.pushMatricesToShader();
    rainbowRectShader.pushMatricesToShader();

    background.draw();
    img.draw();
    toolBar.draw();

    for(int i = 0; i < TOOL_COUNT; i++) {
        tools[i].icon.draw();

        if(tools[i].icon.mouseIsOver() == true) {
            tools[i].icon.updateColor(NColor(32, 31, 40));
            if(lmbdown) {
                currTool = &tools[i];
            }
        } else {
            tools[i].icon.updateColor(NColor(32, 31, 40));
        }

        if(&tools[i] == currTool) {
            tools[i].icon.updateColor(NColor(32, 31, 40));
        }
    }

    selectedPixel.draw();

    colorWheelUpdate();
    colorWheel.draw();

    colorSelectionUpdate();
}

// SETUP
void setupShaders() {
    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    char path1[] = "resources/shaders/vertexshader.vs";
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

    char path7[] = "resources/shaders/vertexshader.vs";
    char path8[] = "resources/shaders/colorselectionshader.fs";
    colorSelectionShader = neptune_loadShader(path7, path8);
    colorSelectionShader.getShaderUniformLocations();
    colorSelectionShader.pushMatricesToShader();

    char path9 [] = "resources/shaders/vertexshader.vs";
    char path10[] = "resources/shaders/rainbowrectshader.fs";
    rainbowRectShader = neptune_loadShader(path9, path10);
    rainbowRectShader.getShaderUniformLocations();
    rainbowRectShader.pushMatricesToShader();

    img.init(0, 0, 200, 200);
    img.setShader(&texShader);
    img.updateColor(NColor(45, 50, 80));
    img.createImage();

    selectedPixel.init(-100, -100, 200, 200);
    selectedPixel.setShader(&basicShader);
    selectedPixel.updateColor(selectedPixelColor);

    background.init(-100, -100, 200, 200);
    background.setShader(&backgroundShader);
    zoomCanvas(0.0);

    toolBar.init(0.0, 0.0, 40.0, 1000.0);
    toolBar.setShader(&basicShader);
    toolBar.updateColor(NColor(42, 41, 50));

    colorWheel.init(10.0, 50.0, 40.0, 40.0);
    colorWheel.setShader(&basicShader);
    colorWheel.updateColor(currColor);
}

void setupToolbarGUI() {
    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    for(int i = 0; i < TOOL_COUNT; i++) {
        Tool tool;

        tool.icon.init(
            2.0,
            screenHeight - 50.0 - i * 30.0, 
            36.0, 
            25.0);

        tool.icon.setShader(&basicShader);
        tool.icon.updateColor(NColor(35, 40, 45));

        tools[i] = tool;
    }
}

void setupTools() {
    tools[PEN_TOOL]   .callbackFunc = penToolUpdate;
    tools[SELECT_TOOL].callbackFunc = selectToolUpdate;

    currTool = &tools[PEN_TOOL];
}

void setupColorSelection() {
    // Base
    float baseWidth = 300.0;
    float baseHeight = 250.0;
    float padding = 5.0;

    colorSelection.base.init(100.0, 200.0, baseWidth, baseHeight);
    colorSelection.base.setShader(&basicShader);
    colorSelection.base.updateColor(NColor(53, 48, 61, 150));

    // Color Quad
    float colorQuadWidth = baseWidth;
    float colorQuadHeight = 150.0;

    colorSelection.colorQuad.init(
        padding, 
        baseHeight - padding - colorQuadHeight, 
        colorQuadWidth - padding * 2.0, 
        colorQuadHeight);

    colorSelection.colorQuad.setShader(&colorSelectionShader);
    colorSelection.colorQuad.parent = &colorSelection.base;
    colorSelection.colorQuad.updateColor(NColor(100, 0, 255));
    colorSelection.colorQuad.updateRadiuses(3.0, 0.0, 0.0, 3.0);

    // Color Slider
    
    float colorSliderHeight = 25.0;
    colorSelection.colorSlider.init(
        padding, 
        baseHeight - padding - colorQuadHeight - colorSliderHeight, 
        colorQuadWidth - padding * 2.0, 
        colorSliderHeight);

    colorSelection.colorSlider.setShader(&rainbowRectShader);
    colorSelection.colorSlider.parent = &colorSelection.base;
    colorSelection.colorSlider.updateColor(NColor(0, 255, 0));
    colorSelection.colorSlider.updateRadiuses(0.0, 3.0, 3.0, 0.0);
}

int main() {
    neptune_init();

    neptune_onPaintCallback(onPaint);
    neptune_onScrollCallback(zoomCanvas);
    neptune_onClickCallback(mouseClickCallback);

    setupShaders();
    setupToolbarGUI();
    setupTools();
    setupColorSelection();

    neptune_start();

    return 0;
}