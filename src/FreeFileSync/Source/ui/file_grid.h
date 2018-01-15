// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef CUSTOM_GRID_H_8405817408327894
#define CUSTOM_GRID_H_8405817408327894

#include <wx+/grid.h>
#include "file_view.h"
#include "file_grid_attr.h"
#include "../lib/icon_buffer.h"


namespace zen
{
//setup grid to show grid view within three components:
namespace filegrid
{
void init(Grid& gridLeft, Grid& gridCenter, Grid& gridRight);
FileView& getDataView(Grid& grid);


void highlightSyncAction(Grid& gridCenter, bool value);

void setupIcons(Grid& gridLeft, Grid& gridCenter, Grid& gridRight, bool show, IconBuffer::IconSize sz);

void setItemPathForm(Grid& grid, ItemPathFormat fmt); //only for left/right grid

void refresh(Grid& gridLeft, Grid& gridCenter, Grid& gridRight);

void setScrollMaster(Grid& grid);

//mark rows selected in overview panel and navigate to leading object
void setNavigationMarker(Grid& gridLeft,
                         std::unordered_set<const FileSystemObject*>&& markedFilesAndLinks,//mark files/symlinks directly within a container
                         std::unordered_set<const ContainerObject*>&& markedContainer);    //mark full container including child-objects
}

wxBitmap getSyncOpImage(SyncOperation syncOp);
wxBitmap getCmpResultImage(CompareFilesResult cmpResult);


//---------- custom events for middle grid ----------

//(UN-)CHECKING ROWS FROM SYNCHRONIZATION
extern const wxEventType EVENT_GRID_CHECK_ROWS;
//SELECTING SYNC DIRECTION
extern const wxEventType EVENT_GRID_SYNC_DIRECTION;

struct CheckRowsEvent : public wxCommandEvent
{
    CheckRowsEvent(size_t rowFirst, size_t rowLast, bool setIncluded) : wxCommandEvent(EVENT_GRID_CHECK_ROWS), rowFirst_(rowFirst), rowLast_(rowLast), setIncluded_(setIncluded) { assert(rowFirst <= rowLast); }
    wxEvent* Clone() const override { return new CheckRowsEvent(*this); }

    const size_t rowFirst_; //selected range: [rowFirst_, rowLast_)
    const size_t rowLast_;  //range is empty when clearing selection
    const bool setIncluded_;
};


struct SyncDirectionEvent : public wxCommandEvent
{
    SyncDirectionEvent(size_t rowFirst, size_t rowLast, SyncDirection direction) : wxCommandEvent(EVENT_GRID_SYNC_DIRECTION), rowFirst_(rowFirst), rowLast_(rowLast), direction_(direction) { assert(rowFirst <= rowLast); }
    wxEvent* Clone() const override { return new SyncDirectionEvent(*this); }

    const size_t rowFirst_; //see CheckRowsEvent
    const size_t rowLast_;  //
    const SyncDirection direction_;
};

using CheckRowsEventFunction     = void (wxEvtHandler::*)(CheckRowsEvent&);
using SyncDirectionEventFunction = void (wxEvtHandler::*)(SyncDirectionEvent&);

#define CheckRowsEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(CheckRowsEventFunction, &func)

#define SyncDirectionEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(SyncDirectionEventFunction, &func)
}

#endif //CUSTOM_GRID_H_8405817408327894
