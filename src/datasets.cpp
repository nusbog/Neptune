// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#include <wx/event.h>
#include <wx/dcbuffer.h>
#include <wx/cursor.h>
#include <nmath.h>
#include <math.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/timer.h>
#include <wx/dcclient.h>
#include <wx/rawbmp.h>

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