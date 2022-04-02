#include <wx/wxprec.h>
#include <wx/event.h>
#include <wx/dcbuffer.h>
#include <wx/cursor.h>
#include <datasets.h>

#ifndef NMATH_H
#define NMATH_H

// Positivt värde returnerar 1, negativt returnerar -1, 0 returnerar 0
template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

int ClosestInt(int a, int b) {
    int c1 = a - (a % b);
    int c2 = (a + b) - (a % b);
    if (a - c1 > c2 - a) {
        return c2;
    } else {
        return c1;
    }
}

bool PointInRect(wxPoint p, wxPoint min, wxPoint max) {
    if((p.x >= min.x && p.x < max.x) &&
        (p.y >= min.y && p.y < max.y)) {

        return true;
    }
    return false;
}

wxPoint GetFixedMousePosition(wxWindow *window) {
    wxPoint pt = wxGetMousePosition();
    int mouseX = pt.x - window->GetScreenPosition().x;
    int mouseY = pt.y - window->GetScreenPosition().y;

    mouseY -= 32 + settings.currScrollAmount / 2;
    mouseX -= 8  + settings.currScrollAmount / 2;

    return wxPoint(mouseX, mouseY);
}

#endif