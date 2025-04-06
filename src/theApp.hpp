#ifndef THEAPP_HPP
#define THEAPP_HPP

#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/dcmemory.h>
#include <wx/dcprint.h>
#include <wx/dcgraph.h>
#include <wx/image.h>
#include <wx/graphics.h>
#include <wx/dcsvg.h>
#include "fxDrawingContext.hpp"

// Provide a pattern that lists your export file types

class MyFrame : public wxFrame
{
public:
    MyFrame() : wxFrame(nullptr, wxID_ANY, "fxDrawingContext Demo", wxDefaultPosition, wxSize(600, 400))
    {
        auto* panel = new wxPanel(this);
        auto* btn = new wxButton(panel, wxID_ANY, "Export Drawing", wxPoint(20, 20));

        btn->Bind(wxEVT_BUTTON, &MyFrame::OnExport, this);
    }
    
    void DrawSample(fxDrawingContext& ctx)
    {
        // 1) Basic shapes with the old-style Draw* calls:
        ctx.SetPen(*wxBLACK_PEN);
        ctx.SetBrush(*wxRED_BRUSH);
        ctx.DrawRectangle(50, 50, 100, 100);
        
        wxFont font(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
        ctx.SetFont(font, *wxBLUE);  
        ctx.DrawText("fxDrawingContext", 60, 60);

        // 2) Create a new path. If we actually have a wxGraphicsContext,
        //    the returned fxGraphicsPath will store a real wxGraphicsPath internally.
        //    Otherwise, it will be tracking-only (m_gc == nullptr).
        fxGraphicsPath fxPath = ctx.CreatePath();

        // 3) Demonstrate a simple triangular path
        fxPath.MoveToPoint(200, 100);
        fxPath.AddLineToPoint(250, 50);
        fxPath.AddLineToPoint(300, 100);
        fxPath.CloseSubpath();

        // Optionally add arcs/curves, e.g.:
        // fxPath.AddArc(320, 100, 40, 0, M_PI, false);
        // fxPath.AddCurveToPoint(..., ..., ..., ..., ..., ...);

        // 4) Change pen/brush
        ctx.SetBrush(*wxBLUE_BRUSH);
        ctx.SetPen(*wxGREEN_PEN);

        // 5) Draw the path with your wrapper.
        //    - If we have a native GC, it calls gc->DrawPath(fxPath.GetPath()).
        //    - Otherwise, it uses your fallback logic (DrawPathOnDC).
        ctx.DrawPath(fxPath);
    }
    


    void OnExport(wxCommandEvent&)
    {
        wxFileDialog dlg(this, "Export", "", "", EXPORT_FILE_PATTERN, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return;

        wxString path = dlg.GetPath();
        wxString ext = path.AfterLast('.').Lower();

        fxDrawingContext ctx;
        wxBitmap bitmap(600, 400);
        wxMemoryDC memDC(bitmap);
        wxGraphicsContext* gc = nullptr;

        if (ext == "svg") {
            auto* svgDC = new wxSVGFileDC(path, 600, 400);
            ctx = fxDrawingContext(svgDC);
            DrawSample(ctx);
            delete svgDC;
        } else if (ext == "png" || ext == "jpg" || ext == "jpeg") {
            memDC.SetBackground(*wxWHITE_BRUSH);
            memDC.Clear();
            gc = wxGraphicsContext::Create(memDC);
            ctx = fxDrawingContext(gc);
            DrawSample(ctx);
            delete gc;

            wxImage img = bitmap.ConvertToImage();
            img.SaveFile(path, ext == "png" ? wxBITMAP_TYPE_PNG : wxBITMAP_TYPE_JPEG);
        } else {
            wxLogError("Unsupported format.");
        }
    }
};

class theApp : public wxApp
{
public:
    bool OnInit() override
    {
        wxInitAllImageHandlers();
        MyFrame* frame = new MyFrame();
        frame->Show();

        return true;
    }
};

#endif // THEAPP_HPP

