#include <iostream>
#include <windows.h>
#include <bits/stdc++.h>

#include <GLFW/glfw3.h>

#include <neptune_core.h>
#include <neptune_math.h>

#include <layer_manager.h>

extern "C" {
    #include <atlas.h>
}

#define TOOL_COUNT 2
#define PEN_TOOL 0
#define SELECT_TOOL 1

using namespace std;

Shader backgroundShader;
Shader basicShader;
Shader texShader;
Shader colorSelectionShader;
Shader rainbowRectShader;

NRectangle toolBar;
NRectangle selectedPixel;
NRectangle background;

NRectangle primaryColorIcon;
NRectangle secondaryColorIcon;

struct ColorSelection {
    bool isOpen = false;

    NRectangle base;
    NRectangle colorQuad;
    NRectangle colorSlider;

    NRectangle hexQuad;
    NRectangle rQuad, gQuad, bQuad, aQuad;

    NImage colorQuadMarker;
    Texture colorQuadMarkerTexture;
    float colorQuadMarkerPosX;
    float colorQuadMarkerPosY;

    // NImage eyeDropperIcon;
    // Texture eyeDropperTexture;
};
ColorSelection colorSelection;

vector<Layer> layers;
Layer *selectedLayer;
int layerCount = 0;

struct Tool {
    NRectangle icon;
    void (*callbackFunc)();

    bool inAction;
    int id;
};
Tool tools[TOOL_COUNT];
Tool *currTool;
Tool *hoverTool;

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

NColor primaryColor(50, 0, 0);
NColor secondaryColor(0, 0, 0, 100);

NColor selectedPixelColor(100, 120, 140, 160);
NColor selectionColor(42, 41, 50, 0);

Atlas atlas;
Texture drawArea;

// CALLBACKS
void zoomCanvas(int dir) {
    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    zoom += dir;
    float fixedResolutionx = resolutionx * zoom;
    float fixedResolutiony = resolutiony * zoom;

    background.updateCoords(screenWidth / 2.0f - fixedResolutionx / 2.0, screenHeight / 2.0f - fixedResolutiony / 2.0, fixedResolutionx, fixedResolutiony);
    for(int i = 0; i < layerCount; i++) {
        layers[i].updateCoords(screenWidth / 2.0f - fixedResolutionx / 2.0, screenHeight / 2.0f - fixedResolutiony / 2.0, fixedResolutionx, fixedResolutiony);
    }
}

// RENDERING
void penToolUpdate() {
    if(lmbdown) {
        currTool->inAction = true;
        selectedLayer->blit(canvasposx, canvasposy, 1, 1, primaryColor.cdata);
    }

    // Delete Pixel if right mouse button is pressed
    if(rmbdown) {
        currTool->inAction = true;
        NColor color(0, 0, 0, 0);
        selectedLayer->blit(canvasposx, canvasposy, 1, 1, color.cdata);
        // drawPixel(canvasposx, canvasposy, color);
    }
}

void selectToolUpdate() {
    if(!lmbdown)
        return;

    currTool->inAction = true;

    float screenWidth, screenHeight;
    float xBackground, yBackground, wBackground, hBackground;

    neptune_getScreenSize(&screenWidth, &screenHeight);
    background.getCoords(&xBackground, &yBackground, &wBackground, &hBackground);

    // Don't go out of bounds of the canvas
    float xDraw0 = clamp(lmbdownposx, xBackground, xBackground + wBackground);
    float yDraw0 = clamp(screenHeight - lmbdownposy, yBackground, yBackground + hBackground);
    float xDraw1 = clamp(currMousePosX, xBackground, xBackground + wBackground);
    float yDraw1 = clamp(screenHeight - currMousePosY, yBackground, yBackground + hBackground);

    selectedPixel.updatePositions(xDraw0, yDraw0, xDraw1, yDraw1);
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

    // Only update the pixel if needed
    if(lastMousePosX != currMousePosX || lastMousePosY != currMousePosY) {
        if(!lmbdown) {
            screenSpacePixelMousePosX = canvasposx * zoom;
            screenSpacePixelMousePosX += currCanvasPosX;

            screenSpacePixelMousePosY = canvasposy * zoom + zoom;
            screenSpacePixelMousePosY += currCanvasPosY;

            selectedPixel.updateCoords(screenSpacePixelMousePosX, screenHeight - screenSpacePixelMousePosY, zoom, zoom);
        }
    }
}

void checkColorWheelToggle() {
    if(colorSelection.isOpen && !colorSelection.base.mouseIsOver()) {
        colorSelection.isOpen = false;
        return;
    }

    if(!primaryColorIcon.mouseIsOver()) {
        return;
    }

    if(colorSelection.isOpen) {
        colorSelection.isOpen = false;
    } else {
        colorSelection.isOpen = true;
    }
}

NColor getColor(NColor src, float x, float y) {
    float r = 1.0 - (x * (1.0 - src.fr));
    float g = 1.0 - (x * (1.0 - src.fg));
    float b = 1.0 - (x * (1.0 - src.fb));

    r *= y;
    g *= y;
    b *= y;

    NColor color(r, g, b);

    return color;
}

NColor getRainbowRectColor(float x) {
    x *= 4.0;
    float v = x / 255.0 / 4.0f;

    NHsvColor hsvA = { 0.0, 1.0, 1.0 };
    NHsvColor hsvB = { 360.0, 1.0, 1.0 };

    NHsvColor hsv = mixc(hsvA, hsvB, v);

    NColor col = hsv2rgb(hsv);

    return col;
}

bool isHoldingColorQuad = false;
void updateColorQuadMarker() {
    if(!lmbdown)
        return;

    if(!isHoldingColorQuad)
        return;

    float c_posx, c_posy, c_width, c_height;
    colorSelection.colorQuad.getCoords(&c_posx, &c_posy, &c_width, &c_height);

    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    float posx = currMousePosX - 12.5f;
    float posy = screenHeight - currMousePosY - 12.5f;

    posx = clamp(posx, c_posx - 12.5f, c_posx + c_width - 12.5f);
    posy = clamp(posy, c_posy - 12.5f, c_posy + c_height - 12.5f);

    colorSelection.colorQuadMarker.updateCoords(posx, posy, 25, 25);

    colorSelection.colorQuadMarkerPosX = clamp((currMousePosX - c_posx) / c_width, 0.0, 1.0);
    colorSelection.colorQuadMarkerPosY = clamp((screenHeight - currMousePosY - c_posy) / c_height, 0.0, 1.0);

    NColor primaryColorRatio = colorSelection.colorQuad.getColor();
    primaryColor = getColor(primaryColorRatio, colorSelection.colorQuadMarkerPosX, colorSelection.colorQuadMarkerPosY);
    primaryColorIcon.updateColor(primaryColor);
}

bool isHoldingColorSlider = false;
void updateRainbowRect() {
    if(!lmbdown)
        return;

    if(!isHoldingColorSlider)
        return;

    float rainbowRectPosX, rainbowRectPosY, rainbowRectW, rainbowRectH;
    colorSelection.colorSlider.getCoords(&rainbowRectPosX, &rainbowRectPosY, &rainbowRectW, &rainbowRectH);

    float rainbowRectPos = (float)(currMousePosX - rainbowRectPosX) / (float)rainbowRectW;

    NColor color1 = getRainbowRectColor(clamp(rainbowRectPos * 255.0, 0.0, 255.0));
    colorSelection.colorQuad.updateColor(color1);

    NColor primaryColorRatio = colorSelection.colorQuad.getColor();
    primaryColor = getColor(primaryColorRatio, colorSelection.colorQuadMarkerPosX, colorSelection.colorQuadMarkerPosY);
    primaryColorIcon.updateColor(primaryColor);
}

void colorSelectionUpdate() {
    if(!colorSelection.isOpen)
        return;

    updateRainbowRect();
    updateColorQuadMarker();

    colorSelection.base.draw();
    colorSelection.colorQuad.draw();
    colorSelection.colorSlider.draw();
    colorSelection.colorQuadMarker.draw();
}

void drawLayers() {
    for(int i = 0; i < layerCount; i++) {
        layers[i].draw();
    }
}

void updateToolColors() {
    for(int i = 0; i < TOOL_COUNT; i++) {
        tools[i].icon.draw();

        NColor color = tools[i].icon.getColor();
        NColor finColor = color;

        if(tools[i].icon.mouseIsOver() == true) {
            finColor = NColor(32, 31, 40);
            hoverTool = &tools[i];
        } else {
            finColor = NColor(38, 38, 42);
        }

        if(&tools[i] == currTool) {
            finColor = NColor(25, 25, 25);
        }

        tools[i].icon.updateColor(finColor);
    }
}

void onPaint() {
    currTool->inAction = false;

    neptune_updateProjectionMatrix();
    setSelectedPixelLocation();

    if(!colorSelection.isOpen)
        currTool->callbackFunc();

    // Update shaders
    backgroundShader.pushMatricesToShader();
    basicShader.pushMatricesToShader();
    texShader.pushMatricesToShader();
    colorSelectionShader.pushMatricesToShader();
    rainbowRectShader.pushMatricesToShader();

    background.draw();
    drawLayers();
    toolBar.draw();
    updateToolColors();

    if((!lmbdown && background.mouseIsOver()) || (currTool->id == SELECT_TOOL && currTool->inAction)) {
        selectedPixel.draw();
    }

    secondaryColorIcon.draw();
    primaryColorIcon.draw();

    colorSelectionUpdate();

    lastMousePosX = currMousePosX;
    lastMousePosY = currMousePosY;
}


// Temporary control over 3D rendering
float rot = 0.0;
float rotSpeed = 0.0;
void onPixelatedPaint() {
    rot += rotSpeed;
    neptune_drawModel(100.0f, 60.0f, rot);
}

void setupGUIElements() {
    selectedPixel.init(-100, -100, 200, 200);
    selectedPixel.setShader(&basicShader);
    selectedPixel.updateColor(selectedPixelColor);

    background.init(-100, -100, 200, 200);
    background.setShader(&backgroundShader);
    zoomCanvas(0.0);
}

// SETUP
void setupShaders() {
    // Vertex Shader Paths
    char vertexSrcPath[] = "resources/shaders/vertexshader.vs";

    // Fragment Shader Paths
    char texSrcPath[] = "resources/shaders/textureshader.fs";
    char backgroundSrcPath[] = "resources/shaders/backgroundshader.fs";
    char fragmentSrcPath[] = "resources/shaders/fragmentshader.fs";
    char colorselectSrcPath[] = "resources/shaders/colorselectionshader.fs";
    char rainbowRectSrcPath[] = "resources/shaders/rainbowrectshader.fs";

    texShader = neptune_loadShader(vertexSrcPath, texSrcPath);
    texShader.getShaderUniformLocations();
    texShader.pushMatricesToShader();

    backgroundShader = neptune_loadShader(vertexSrcPath, backgroundSrcPath);
    backgroundShader.getShaderUniformLocations();
    backgroundShader.pushMatricesToShader();

    basicShader = neptune_loadShader(vertexSrcPath, fragmentSrcPath);
    basicShader.getShaderUniformLocations();
    basicShader.pushMatricesToShader();

    colorSelectionShader = neptune_loadShader(vertexSrcPath, colorselectSrcPath);
    colorSelectionShader.getShaderUniformLocations();
    colorSelectionShader.pushMatricesToShader();

    rainbowRectShader = neptune_loadShader(vertexSrcPath, rainbowRectSrcPath);
    rainbowRectShader.getShaderUniformLocations();
    rainbowRectShader.pushMatricesToShader();
}

void createLayer() {
    Layer layer;
    layer.create(resolutionx, resolutiony, &drawArea, &texShader);
    layers.insert(layers.begin(), layer);

    selectedLayer = &layers[0];

    layerCount++;
}

void setupToolbarGUI() {
    float screenWidth, screenHeight;
    neptune_getScreenSize(&screenWidth, &screenHeight);

    // Sets the toolbar position
    toolBar.init(0.0, 0.0, 40.0, 1000.0);
    toolBar.setShader(&basicShader);
    toolBar.updateColor(NColor(38, 38, 42));

    // Sets the position of all tools in the toolbar
    for(int i = 0; i < TOOL_COUNT; i++) {
        Tool tool;

        tool.icon.init(
            2.0,
            screenHeight - 36.0 - i * 32.0,
            36.0,
            25.0); 

        tool.icon.setShader(&basicShader);
        tool.icon.updateColor(NColor(35, 40, 45));
        tool.id = i;

        tools[i] = tool;
    }
}

void setupTools() {
    tools[PEN_TOOL]   .callbackFunc = penToolUpdate;
    tools[SELECT_TOOL].callbackFunc = selectToolUpdate;

    currTool = hoverTool = &tools[PEN_TOOL];
}

void setupColorSelection() {
    // Opener
    primaryColorIcon.init(10.0, 50.0, 40.0, 40.0);
    primaryColorIcon.setShader(&basicShader);
    primaryColorIcon.updateColor(primaryColor);

    secondaryColorIcon.init(30.0, 30.0, 40.0, 40.0);
    secondaryColorIcon.setShader(&basicShader);
    secondaryColorIcon.updateColor(secondaryColor);

    // Base
    float baseWidth = 300.0;
    float baseHeight = 250.0;
    float basePosX = 10.0;
    float basePosY = 90.0;
    float padding = 10.0;

    colorSelection.base.init(basePosX, basePosY, baseWidth, baseHeight);
    colorSelection.base.setShader(&basicShader);
    colorSelection.base.updateColor(NColor(38, 38, 42, 255));

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
    colorSelection.colorQuad.updateColor(NColor(255, 0, 0));
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

    // Color Quad Marker
    colorSelection.colorQuadMarker.init(basePosX, basePosY, 25.0, 25.0);
    colorSelection.colorQuadMarker.setShader(&texShader);
    colorSelection.colorQuadMarker.updateColor(NColor(53, 48, 61, 150));
    colorSelection.colorQuadMarker.bindTexture(&colorSelection.colorQuadMarkerTexture);
}

void loadImages() {
    atlas = createAtlas(1024, 1024, 256);

    // Drawing surface
    unsigned char *memDrawData = (unsigned char *)calloc(resolutionx * resolutiony * 4, 1);

    colorSelection.colorQuadMarkerTexture = loadTexture((char*)"resources/circle.png", &atlas);
    drawArea = loadTextureFromMemory(memDrawData, resolutionx, resolutiony, &atlas);
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

            isHoldingColorQuad = colorSelection.colorQuad.mouseIsOver();
            isHoldingColorSlider = colorSelection.colorSlider.mouseIsOver();

            if(hoverTool->icon.mouseIsOver()) {
                currTool = hoverTool;
            }

            checkColorWheelToggle();
        } else if(action == GLFW_RELEASE) {
            lmbdown = false;
            isHoldingColorQuad = false;
            isHoldingColorSlider = false;
        }
    }
}

void keyCallback(int key, int action, int mods) {
    if(mods == GLFW_MOD_CONTROL) {
        if(key == GLFW_KEY_S && action == GLFW_PRESS) {
            selectedLayer->saveToFile();
        }
    }

    if(mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)) {
        if(key == GLFW_KEY_S && action == GLFW_PRESS) {
            selectedLayer->saveToFile();

            // Send saved file to cloud database
            system("curl iphere/ -Tlayer0.png");
        }
    }

    // In GLFW, EQUAL is PLUS...
    if(key == GLFW_KEY_I && action == GLFW_PRESS) {
        zoomCanvas(1);
    }
    if(key == GLFW_KEY_O && action == GLFW_PRESS) {
        zoomCanvas(-1);
    }

    if(key == GLFW_KEY_LEFT) {
        rot -= 0.1f;
    }
    if(key == GLFW_KEY_RIGHT) {
        rot += 0.1f;
    }
}

void printGuide() {
    cout << "\n---GUIDE---\n";
    cout << "The tools of this program are towards the left of the window.\n";
    cout << "Your currently chosen tool is the pen tool, hover over and click the other tool to use it.\n";
    cout << "To draw a pixelated model, drag and drop a .obj model from the build folder.\n";
    cout << "Rotate the model with the arrow keys.\n";
    cout << "Save the canvas to a file with CTRL + S.\n";
    cout << "Save the canvas to the cloud database with CTRL + SHIFT + S.\n";
    cout << "Access the color wheel by pressing the square at the bottom left corner.\n";
}

int main() {
    // Load images before creating a window so there's no white screen
    loadImages();
    neptune_init();

    // Assign Callbacks
    neptune_pushAtlasToGPU(&atlas);
    neptune_onPaintCallback(onPaint);
    neptune_onScrollCallback(zoomCanvas);
    neptune_onClickCallback(mouseClickCallback);
    neptune_onKeyCallback(keyCallback);
    neptune_onPixelatedCallback(onPixelatedPaint);

    setupShaders();
    createLayer();
    setupToolbarGUI();
    setupGUIElements();
    setupTools();
    setupColorSelection();

    printGuide();

    neptune_start();

    return 0;
}