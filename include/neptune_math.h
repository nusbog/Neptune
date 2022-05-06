#ifndef NEPTUNE_MATH_H
#define NEPTUNE_MATH_H

// Positivt värde returnerar 1, negativt returnerar -1, 0 returnerar 0
template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

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

#endif