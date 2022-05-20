#ifndef NEPTUNE_MATH_H
#define NEPTUNE_MATH_H

#include <neptune_core.h>

// Positivt värde returnerar 1, negativt returnerar -1, 0 returnerar 0
template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

float mixf(float x, float y, float a) {
    return x * (1 - a) + y * a;
}

NHsvColor mixc(NHsvColor x, NHsvColor y, float a) {
    NHsvColor mix_color;

    mix_color.h = mixf(x.h, y.h, a);
    mix_color.s = mixf(x.s, y.s, a);
    mix_color.v = mixf(x.v, y.v, a);

    return mix_color;
}

NColor hsv2rgb(NHsvColor in)
{
    double      hh, p, q, t, ff;
    long        i;
    float r, g, b;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        r = in.v;
        g = in.v;
        b = in.v;
        return NColor(r, g, b);
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        r = in.v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = in.v;
        b = p;
        break;
    case 2:
        r = p;
        g = in.v;
        b = t;
        break;

    case 3:
        r = p;
        g = q;
        b = in.v;
        break;
    case 4:
        r = t;
        g = p;
        b = in.v;
        break;
    case 5:
    default:
        r = in.v;
        g = p;
        b = q;
        break;
    }
    return NColor(r, g, b);
}

// TODO: Make generic
int closestInt(int a, int b) {
    int c1 = a - (a % b);
    int c2 = (a + b) - (a % b);
    if (a - c1 > c2 - a) {
        return c2;
    } else {
        return c1;
    }
}

float clamp(float value, float min, float max)
{
    float result = (value < min)? min : value;

    if (result > max) result = max;

    return result;
}

bool compareColors(NColor color1, NColor color2) {
    if((color1.r == color2.r) && (color1.g == color2.g) && (color1.b == color2.b)) {
        return true;
    }
    return false;
}

/*
void bresenhamsLine(int x0, int y0, int x1, int y1, NColor color) {
    int dx, dy, p, x, y;
     
    dx = x1 - x0;
    dy = y1 - y0;
     
    x = x0;
    y = y0;
     
    p = 2 * dy - dx;
     
    while(x < x1) {
        if(p >= 0) {
            drawPixel(x, y, color);
            y = y + 1;
            p = p + 2 * dy - 2 * dx;
        }
        else {
            drawPixel(x, y, color);
            p = p + 2 * dy;
        }
        x = x + 1;
    }
}*/

#endif