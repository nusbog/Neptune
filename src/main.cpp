// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#include <wx/event.h>
#include <wx/dcbuffer.h>
#include <wx/cursor.h>

#include <nmath.h>
#include <datasets.h>
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

///////////// Declarations

class Frame : public wxFrame
{
    public:
        Frame( wxWindow* parent, int id = wxID_ANY, wxString title = "Neptune",
                 wxPoint pos = wxDefaultPosition, wxSize size = wxDefaultSize,
                 int style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
        ~Frame();
        void RebuildBufferAndRefresh();

    private:
        // Event Handlers
        void OnTimer(wxTimerEvent& event);
        void OnResize(wxSizeEvent& event);
        void OnScroll(wxMouseEvent& event);
        void OnKeyDown(wxKeyEvent& event);

        void Zoom(int dir);
        void OnPaint();
        void CenterCanvas();
        void DeletePixel(wxPoint point);

        // Private data
        unsigned char* m_pixelData;
        wxWindow* backgroundSurface;
        wxWindow* m_renderSurface2;
        wxBitmap m_bitmapBuffer;
        int m_width;
        int m_height;

        bool resizing;

        wxTimer m_timer;
};

class MyApp : public wxApp
{
    public:
        virtual bool OnInit() wxOVERRIDE;
};

wxDECLARE_APP(MyApp);

///////////// Implementation

void AssignSettings() {
    Color c1;
    c1.r = 115; c1.g = 135; c1.b = 165;
    Color c2;
    c2.r = 100; c2.g = 115; c2.b = 150;
    settings.squareColors[0] = c1;
    settings.squareColors[1] = c2;

    Color currPixelColor;
    currPixelColor.r = 50, 
    currPixelColor.g = 50, 
    currPixelColor.b = 50, 
    currPixelColor.a = 1 , 
    settings.currPixelColor = currPixelColor;

    settings.currScrollAmount = 5;
    settings.scrollSpeed = 32;

    settings.startSizeX = 8;
    settings.startSizeY = 8;
}

Frame::Frame( wxWindow* parent, int id, wxString title, wxPoint pos,
                  wxSize size, int style )
        :wxFrame( parent, id, "Neptune", pos, wxSize(800, 600), style )
{
    SetBackgroundColour(wxColour(45, 50, 80, 255));
    AssignSettings();

    m_width  = settings.startSizeX;
    m_height = settings.startSizeY;
    m_pixelData = new unsigned char[3 * 2560 * 1080];

    backgroundSurface = new wxWindow(this, wxID_ANY, wxPoint(0, 0),
                                    wxSize(m_width, m_height));
    backgroundSurface->SetBackgroundStyle(wxBG_STYLE_PAINT);
    backgroundSurface->Bind(wxEVT_KEY_DOWN, &Frame::OnKeyDown, this);

    m_renderSurface2 = new wxWindow(this, wxID_ANY, wxPoint(0, 0),
                                    wxSize(4, 4));
    m_renderSurface2->SetBackgroundStyle(wxBG_STYLE_PAINT );

    this->Bind(wxEVT_SIZE, &Frame::OnResize, this);
    this->Bind(wxEVT_MOUSEWHEEL, &Frame::OnScroll, this);
    this->Bind(wxEVT_KEY_DOWN, &Frame::OnKeyDown, this);

    m_timer.Bind(wxEVT_TIMER, &Frame::OnTimer, this);
    m_timer.Start(5);
}

Frame::~Frame() // Exit
{
    delete[] m_pixelData;
}

void Frame::DeletePixel(wxPoint point) {
    int size = settings.currScrollAmount;
    wxBitmap replacement(size, size, 24);
    wxNativePixelData replacementdata(replacement);

    wxNativePixelData::Iterator replacementp(replacementdata);

    for (int y = 0; y < size; ++y)
    {
        wxNativePixelData::Iterator rowStart = replacementp;
        for (int x = 0; x < size; ++x, ++replacementp)
        {
            int index = 3 * (point.x + x) * m_width + 3 * (point.y + y);
            replacementp.Red()   = m_pixelData[index];
            replacementp.Green() = m_pixelData[index+1];
            replacementp.Blue()  = m_pixelData[index+2];
        }
        replacementp = rowStart;
        replacementp.OffsetY(replacementdata, 1);
    }
    wxWindowDC dc(m_renderSurface2);
    dc.DrawBitmap(replacement, 0, 0);
}

wxPoint oldMousePos = wxPoint(0, 0);
void Frame::OnTimer(wxTimerEvent& event) {
    wxPoint mousePos = GetFixedMousePosition(this);

    mousePos.x = ClosestInt(mousePos.x, settings.currScrollAmount);
    mousePos.y = ClosestInt(mousePos.y, settings.currScrollAmount);

    if(oldMousePos != mousePos) {
        bool inCanvas = PointInRect(mousePos, wxPoint(0, 0), wxPoint(m_width, m_height));
        if(inCanvas) {
            int size = settings.currScrollAmount;

            wxBitmap bmp(size, size, 32);
            bmp.UseAlpha(true);
            wxAlphaPixelData data(bmp);
            wxAlphaPixelData::Iterator p(data);

            for (int y = 0; y < size; ++y)
            {
                wxAlphaPixelData::Iterator rowStart = p;
                for (int x = 0; x < size; ++x, ++p)
                {
                    p.Red()   = settings.currPixelColor.r;
                    p.Green() = settings.currPixelColor.g;
                    p.Blue()  = settings.currPixelColor.b;
                    p.Alpha() = settings.currPixelColor.a;
                }
                p = rowStart;
                p.OffsetY(data, 1);
            }
            DeletePixel(oldMousePos);

            wxWindowDC dc(m_renderSurface2);
            m_renderSurface2->Move(mousePos.x, mousePos.y);
            m_renderSurface2->SetSize(size, size);
            dc.DrawBitmap(bmp, 0, 0);

            oldMousePos = mousePos;
        }
    }
}

void Frame::CenterCanvas() {
}

void Frame::OnPaint()
{
    wxWindowDC dc(backgroundSurface);
    backgroundSurface->SetSize(m_width, m_height);
    CenterCanvas();
    dc.DrawBitmap(m_bitmapBuffer, 0, 0);
}

void Frame::OnResize(wxSizeEvent& event) {
    resizing = true;
}

void Frame::Zoom(int dir) {
    settings.currScrollAmount += dir;
    m_width  = settings.startSizeX * settings.currScrollAmount;
    m_height = settings.startSizeY * settings.currScrollAmount;


    RebuildBufferAndRefresh();
}

void Frame::OnScroll(wxMouseEvent &event) {
    int scrollDir = sign(event.GetWheelRotation()); // 1 eller -1
    Zoom(scrollDir);
}

void Frame::OnKeyDown(wxKeyEvent &event) {
    wxChar key = event.GetUnicodeKey();

    if(key == 43) {
        Zoom(1);
    } 
    if(key == 45) {
        Zoom(-1);
    }
}

void Frame::RebuildBufferAndRefresh()
{
    int sqrSize   = 32;
    int sqrSizeD2 = sqrSize / 2;

    wxBitmap bmp(m_width, m_height, 24);
    wxNativePixelData data(bmp);
    wxNativePixelData::Iterator p(data);
    int curPixelDataLoc = 0;
    for ( int y = 0; y < m_height; ++y )
    {
        wxNativePixelData::Iterator rowStart = p;
        for ( int x = 0; x < m_width; ++x, ++p)
        {
            int checkered = ((x - (y % sqrSize >= sqrSizeD2) * sqrSizeD2 + sqrSizeD2) % sqrSize >= sqrSizeD2);
            int index = 3 * y * m_width + 3 * x;

            m_pixelData[  index] = settings.squareColors[checkered].r;
            m_pixelData[++index] = settings.squareColors[checkered].g;
            m_pixelData[++index] = settings.squareColors[checkered].b;

            p.Red()   = settings.squareColors[checkered].r;
            p.Green() = settings.squareColors[checkered].g;
            p.Blue()  = settings.squareColors[checkered].b;
        }
        p = rowStart;
        p.OffsetY(data, 1);
    }

    m_bitmapBuffer = bmp;
    SetDoubleBuffered(true); // Tar bort flicker vid zoom

    backgroundSurface->Refresh();
    backgroundSurface->Update();
    OnPaint();
}


bool MyApp::OnInit()
{
    Frame* frame = new Frame(NULL);
    frame->Show();

    frame->RebuildBufferAndRefresh();

    return true;
}

wxIMPLEMENT_APP(MyApp);