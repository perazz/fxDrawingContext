/***************************************************************
 * Name:      wxChartsApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Federico Perini (federico.perini@gmail.com)
 * Created:   2022-11-04
 * Copyright: Federico Perini ()
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "wxPlotLibApp.h"
#include "../src/samples/sampleBook.hpp"
#include <wxPlotLib.h>
#include <wx/splitter.h>

//(*AppHeaders
#include <wx/image.h>
//*)

IMPLEMENT_APP(wxChartsApp);

bool wxChartsApp::OnInit()
{
	// AppInitialize
	wxInitAllImageHandlers();

    // Add wxCharts icons to the app art provider
    wxArtProvider::Push(new wxChartsArtProvider);

    sampleBook* frame = new sampleBook(NULL, wxID_ANY, wxT("wxPlotLib Samples Program"), wxDefaultPosition, wxSize(640,480));

    frame->Show(true);

    // Start the event loop
    return true;
}
