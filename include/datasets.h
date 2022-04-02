#ifndef DATASETS_H
#define DATASETS_H

struct Color {
    int r, g, b, a;
};

struct Settings {
    Color squareColors[2];
    Color currPixelColor;

    int scrollSpeed;
    int currScrollAmount;

    int startSizeX, startSizeY;
};
Settings settings;

#endif