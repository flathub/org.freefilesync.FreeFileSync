// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef TRIPLE_SPLITTER_H_8257804292846842573942534254
#define TRIPLE_SPLITTER_H_8257804292846842573942534254

#include <cassert>
#include <memory>
#include <wx/window.h>
#include <wx/dcclient.h>

//a not-so-crappy splitter window

/* manage three contained windows:
    1. left and right window are stretched
    2. middle window is fixed size
    3. middle window position can be changed via mouse with two sash lines
    -----------------
    |      | |      |
    |      | |      |
    |      | |      |
    -----------------
*/

namespace zen
{
class TripleSplitter : public wxWindow
{
public:
    TripleSplitter(wxWindow* parent,
                   wxWindowID id      = wxID_ANY,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0);

    ~TripleSplitter();

    void setupWindows(wxWindow* winL, wxWindow* winC, wxWindow* winR)
    {
        assert(winL->GetParent() == this && winC->GetParent() == this && winR->GetParent() == this && !GetSizer());
        windowL = winL;
        windowC = winC;
        windowR = winR;
        updateWindowSizes();
    }

    int getSashOffset() const { return centerOffset; }
    void setSashOffset(int off) { centerOffset = off; updateWindowSizes(); }

private:
    void onEraseBackGround(wxEraseEvent& event) {}
    void onSizeEvent(wxSizeEvent& event) { updateWindowSizes(); event.Skip(); }

    void onPaintEvent(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        drawSash(dc);
    }

    void updateWindowSizes();
    int getCenterWidth() const;
    int getCenterPosX() const; //return normalized posX
    int getCenterPosXOptimal() const;

    void drawSash(wxDC& dc);
    bool hitOnSashLine(int posX) const;

    void onMouseLeftDown(wxMouseEvent& event);
    void onMouseLeftUp(wxMouseEvent& event);
    void onMouseMovement(wxMouseEvent& event);
    void onLeaveWindow(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onMouseLeftDouble(wxMouseEvent& event);

    class SashMove;
    std::unique_ptr<SashMove> activeMove;

    int centerOffset = 0; //offset to add after "gravity" stretching

    wxWindow* windowL = nullptr;
    wxWindow* windowC = nullptr;
    wxWindow* windowR = nullptr;
};
}

#endif //TRIPLE_SPLITTER_H_8257804292846842573942534254
