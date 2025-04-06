// fxDrawingContext.cpp
#include "fxDrawingContext.hpp"
#include <wx/graphics.h>
#include <wx/dcgraph.h>
#include <wx/log.h>
#include <wx/dcmemory.h>
#include <wx/dcprint.h>
#include <wx/dcclient.h>
#include <wx/dc.h>

//---------------------------------------------------------
// Constructor from wxGraphicsContext*
//---------------------------------------------------------
fxDrawingContext::fxDrawingContext(wxGraphicsContext* gc)
{
    if (gc) {
        // Just store the raw pointer in the variant
        m_context = gc;
    } else {
        // monostate if null
        m_context = std::monostate{};
    }
}

//---------------------------------------------------------
// Constructor from wxDC*
//---------------------------------------------------------
fxDrawingContext::fxDrawingContext(wxDC* dc)
{
    if (!dc) {
        // null DC => monostate
        m_context = std::monostate{};
        return;
    }

    // Attempt to detect a known DC subtype that can produce a wxGraphicsContext
    wxWindowDC*   windowDC   = dynamic_cast<wxWindowDC*>(dc);
    wxMemoryDC*   memoryDC   = dynamic_cast<wxMemoryDC*>(dc);
    wxPrinterDC*  printerDC  = dynamic_cast<wxPrinterDC*>(dc);

    wxGraphicsContext* rawGC = nullptr;

    if (windowDC) {
        // We have a wxWindowDC
        rawGC = wxGraphicsContext::Create(*windowDC);
    }
    else if (memoryDC) {
        // We have a wxMemoryDC
        rawGC = wxGraphicsContext::Create(*memoryDC);
    }
    else if (printerDC) {
        // We have a wxPrinterDC
        rawGC = wxGraphicsContext::Create(*printerDC);
    }

    if (rawGC) {
        // We successfully created a GC
        m_context = rawGC;

        // Store in a shared_ptr so we free it automatically
        m_ownedGC = std::shared_ptr<wxGraphicsContext>(rawGC, [](wxGraphicsContext* p){ delete p; }
        );
    } else {
        // GC creation not possible => fallback to raw DC
        m_context = dc;
    }
}

// -------------------------------------------------------------
// GetSize: Get context size
// -------------------------------------------------------------
wxSize fxDrawingContext::GetSize() const
{
    wxSize sizeResult(0, 0);

    std::visit([&](auto&& c){
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>)
        {
            if (c)
            {
                double w, h;
                c->GetSize(&w, &h);
                sizeResult.SetWidth(static_cast<int>(w));
                sizeResult.SetHeight(static_cast<int>(h));
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>)
        {
            if (c)
            {
                int w, h;
                c->GetSize(&w, &h);
                sizeResult.SetWidth(w);
                sizeResult.SetHeight(h);
            }
        }
        // If monostate (no context), remain (0, 0)
    }, m_context);

    return sizeResult;
}

void fxDrawingContext::GetSize(wxDouble* width, wxDouble* height) const
{
    // Initialize to zero if pointers are non-null
    if (width)  *width  = 0.0;
    if (height) *height = 0.0;

    std::visit([&](auto&& c){
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>)
        {
            if (c && width && height)
            {
                c->GetSize(width, height);
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>)
        {
            if (c)
            {
                int w = 0, h = 0;
                c->GetSize(&w, &h);
                if (width)  *width  = static_cast<wxDouble>(w);
                if (height) *height = static_cast<wxDouble>(h);
            }
        }
        // else monostate: do nothing (already set to 0 above)
    }, m_context);
}
    
// -------------------------------------------------------------
// SetFont: unify font setting for GC and DC
// -------------------------------------------------------------
// For wxGraphicsContext: SetFont(const wxFont& font, const wxColour& col)
// For wxDC: SetFont + SetTextForeground
void fxDrawingContext::SetFont(const wxFont& font, const wxColour& colour)
{
    std::visit([&](auto&& ctx){
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) {
                ctx->SetFont(font, colour);
            }
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) {
                ctx->SetFont(font);
                ctx->SetTextForeground(colour);
            }
        }
    }, m_context);
}

void fxDrawingContext::GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const
{
    widths.clear();
    if (text.empty())
        return;

    std::visit([&](auto&& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (c) {
                c->GetPartialTextExtents(text, widths);
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            if (c) {
                // Fallback: measure each substring
                widths.reserve(text.size());
                wxCoord w, h, d, e;
                for (size_t i = 0; i < text.size(); ++i)
                {
                    wxString sub = text.SubString(0, i);
                    c->GetTextExtent(sub, &w, &h, &d, &e);
                    widths.push_back(static_cast<double>(w));
                }
            }
        }
        else {
            // monostate => do nothing
        }
    }, m_context);
}

//---------------------------------------------------
// Get Text extent (similar to wxGraphicsContext API)
//---------------------------------------------------
void fxDrawingContext::GetTextExtent(const wxString& text,
                                     wxDouble* width,
                                     wxDouble* height,
                                     wxDouble* descent,
                                     wxDouble* externalLeading) const
{
    // Initialize outputs if provided
    auto setIfNotNull = [](wxDouble* p, double val){ if(p) *p = val; };

    // Default to zero
    setIfNotNull(width, 0.0);
    setIfNotNull(height, 0.0);
    setIfNotNull(descent, 0.0);
    setIfNotNull(externalLeading, 0.0);

    std::visit([&](auto&& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (c) {
                // wxGraphicsContext version
                c->GetTextExtent(text, width, height, descent, externalLeading);
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            if (c) {
                // DC fallback: integer-based measurement
                wxCoord w{}, h{}, d{}, e{};
                c->GetTextExtent(text, &w, &h, &d, &e);

                setIfNotNull(width,           static_cast<double>(w));
                setIfNotNull(height,          static_cast<double>(h));
                setIfNotNull(descent,         static_cast<double>(d));
                setIfNotNull(externalLeading, static_cast<double>(e));
            }
        }
    }, m_context);
}

//----------------------------------------
// Get Text size (with optional arguments)
//----------------------------------------
void fxDrawingContext::GetTextSize(const wxFont& font,
                                   const wxString& text,
                                   wxDouble& width,
                                   wxDouble& height,
                                   wxDouble angleRad,
                                   wxDouble* descent,
                                   wxDouble* externalLeading)
{
    wxDouble localDescent = 0.0;
    wxDouble localExternal = 0.0;

    SetFont(font, *wxBLACK);

    // Use internal variables if pointers not provided
    wxDouble* descPtr = descent ? descent : &localDescent;
    wxDouble* extPtr  = externalLeading ? externalLeading : &localExternal;

    wxDouble rawW = 0.0, rawH = 0.0;
    GetTextExtent(text, &rawW, &rawH, descPtr, extPtr);

    if (angleRad == 0.0) {
        width = rawW;
        height = rawH;
    } else {
        // Compute rotated bounding box
        width  = rawH * std::sin(angleRad) + rawW * std::cos(angleRad);
        height = rawH * std::cos(angleRad) + rawW * std::sin(angleRad);
    }
}


//--------------------------------------
// Basic drawing methods
//--------------------------------------
void fxDrawingContext::SetBrush(const wxBrush& brush)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) ctx->SetBrush(brush);
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) ctx->SetBrush(brush);
        }
    }, m_context);
}

void fxDrawingContext::SetPen(const wxPen& pen)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) ctx->SetPen(pen);
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) ctx->SetPen(pen);
        }
    }, m_context);
}

void fxDrawingContext::DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) ctx->DrawRectangle(x, y, w, h);
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) ctx->DrawRectangle(wxRect(x, y, w, h));
        }
    }, m_context);
}


//--------------------------------------
// Draw a text box
//--------------------------------------
void fxDrawingContext::DrawText(const wxString& text, wxDouble x, wxDouble y)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) ctx->DrawText(text, x, y);
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) ctx->DrawText(text, wxPoint(x, y));
        }
    }, m_context);
}

void fxDrawingContext::DrawText(const wxString& text, wxDouble x, wxDouble y, wxDouble angleRad)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) {
                // wxGraphicsContext uses radians
                ctx->DrawText(text, x, y, angleRad);
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) {
                // wxDC::DrawRotatedText takes degrees, clockwise
                double angleDeg = angleRad * 180.0 / M_PI;
                ctx->DrawRotatedText(text, wxPoint(x, y), angleDeg);
            }
        }
    }, m_context);
}


fxGraphicsPath fxDrawingContext::CreatePath()
{
    wxGraphicsContext* actualGC = nullptr;

    std::visit([&](auto&& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            // We have a real GC
            if (c) {
                actualGC = c;
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            // We only have a plain DC (e.g. wxSVGFileDC, wxMemoryDC, etc.)
            // => do not attempt to create a GC
            // => fallback will rely on fxPathSegment geometry
            wxLogDebug("fxDrawingContext::NewPath() => DC only => fallback (no GC).");
        }
    }, m_context);

    if (!actualGC) {
        // We'll have to do tracking-only
        wxLogDebug("fxDrawingContext::NewPath() => actualGC == nullptr, returning fxGraphicsPath(nullptr).");
        return fxGraphicsPath(nullptr);
    }

    // If we do have a GC, build a path with it
    return fxGraphicsPath(actualGC);
}

//--------------------------------------
// Draw the path
//--------------------------------------
void fxDrawingContext::DrawPath(const fxGraphicsPath& fxpath)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) {
                // Use native DrawPath
                ctx->DrawPath(fxpath.GetPath());
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) {
                // Fallback: interpret geometry from fxGraphicsPath
                DrawPathOnDC(ctx, fxpath);
            }
        }
    }, m_context);
}

//--------------------------------------
// Fallback for drawing path geometry on plain wxDC
//--------------------------------------
void DrawPathOnDC(wxDC* dc, const fxGraphicsPath& path, wxPolygonFillMode fillMode)
{
    if (!dc) return;
    const auto& segments = path.GetSegments();
    if (segments.empty()) return;

    std::vector<wxPoint> currentSubpath;
    currentSubpath.reserve(16);

    for (auto& seg : segments)
    {
        switch (seg.type)
        {
        case fxPathSegmentType::MoveTo:
            if (!currentSubpath.empty()) {
                dc->DrawLines(currentSubpath.size(), currentSubpath.data());
                currentSubpath.clear();
            }
            if (!seg.points.empty()) {
                auto& p = seg.points[0];
                currentSubpath.push_back(wxPoint((int)p.m_x, (int)p.m_y));
            }
            break;

        case fxPathSegmentType::LineTo:
            if (!seg.points.empty()) {
                auto& p = seg.points[0];
                currentSubpath.push_back(wxPoint((int)p.m_x, (int)p.m_y));
            }
            break;

        case fxPathSegmentType::Rectangle:
        {
            wxDouble x1 = seg.points[0].m_x;
            wxDouble y1 = seg.points[0].m_y;
            wxDouble x2 = seg.points[1].m_x;
            wxDouble y2 = seg.points[1].m_y;
            dc->DrawRectangle(wxRect((int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1)));
            break;
        }

        case fxPathSegmentType::Ellipse:
            // ...
            break;

        case fxPathSegmentType::Close:
            if (!currentSubpath.empty()) {
                dc->DrawPolygon(currentSubpath.size(), currentSubpath.data(), 0, 0, fillMode);
                currentSubpath.clear();
            }
            break;

        // ... handle arcs, curves, etc. as needed

        default:
            break;
        }
    }

    // If there's an open subpath, draw it as lines
    if (!currentSubpath.empty()) {
        dc->DrawLines(currentSubpath.size(), currentSubpath.data());
    }
}

void fxDrawingContext::FillPath(const fxGraphicsPath& fxpath, 
                                wxPolygonFillMode fillStyle)
{
    std::visit([&](auto&& c){
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            // If we have a real GC, use native FillPath
            if (c) {
                c->FillPath(fxpath.GetPath(), fillStyle);
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            // Fallback for DC
            if (c) {
                FillPathOnDC(c, fxpath, fillStyle);
            }
        }
    }, m_context);
}

void FillPathOnDC(wxDC* dc, const fxGraphicsPath& path,
                  wxPolygonFillMode fillStyle)
{
    if (!dc) return;

    // Save the old pen and brush so we can restore later if desired
    wxPen oldPen = dc->GetPen();
    dc->SetPen(*wxTRANSPARENT_PEN); // fill only, no outline

    const auto& segments = path.GetSegments();
    if (segments.empty()) {
        dc->SetPen(oldPen);
        return;
    }

    // We’ll collect polygons in a subpath
    std::vector<wxPoint> currentSubpath;
    currentSubpath.reserve(16);

    for (auto& seg : segments)
    {
        switch (seg.type)
        {
        case fxPathSegmentType::MoveTo:
            // If there's an ongoing subpath, fill it
            if (!currentSubpath.empty()) {
                dc->DrawPolygon(currentSubpath.size(), currentSubpath.data(), 
                                0, 0, fillStyle);
                currentSubpath.clear();
            }
            // Start new subpath
            if (!seg.points.empty()) {
                const auto& p = seg.points[0];
                currentSubpath.push_back(wxPoint((int)p.m_x, (int)p.m_y));
            }
            break;

        case fxPathSegmentType::LineTo:
            if (!seg.points.empty()) {
                const auto& p = seg.points[0];
                currentSubpath.push_back(wxPoint((int)p.m_x, (int)p.m_y));
            }
            break;

        case fxPathSegmentType::Rectangle:
            {
                // We'll fill it immediately
                double x1 = seg.points[0].m_x;
                double y1 = seg.points[0].m_y;
                double x2 = seg.points[1].m_x;
                double y2 = seg.points[1].m_y;
                double w  = x2 - x1;
                double h  = y2 - y1;
                dc->DrawRectangle(wxRect((int)x1, (int)y1, (int)w, (int)h));
            }
            break;

        case fxPathSegmentType::Ellipse:
            {
                // If we stored it as circle center+radius => seg.points.size() == 1
                // If bounding box => seg.points.size() == 2
                if (seg.points.size() == 1) {
                    // (center, radius)
                    double cx = seg.points[0].m_x;
                    double cy = seg.points[0].m_y;
                    double r  = seg.radius;
                    dc->DrawEllipse((int)(cx - r), (int)(cy - r), 
                                    (int)(2*r), (int)(2*r));
                }
                else if (seg.points.size() == 2) {
                    // bounding box
                    double x1 = seg.points[0].m_x;
                    double y1 = seg.points[0].m_y;
                    double x2 = seg.points[1].m_x;
                    double y2 = seg.points[1].m_y;
                    dc->DrawEllipse((int)x1, (int)y1, 
                                    (int)(x2 - x1), (int)(y2 - y1));
                }
            }
            break;

        case fxPathSegmentType::Close:
            // Fill subpath
            if (!currentSubpath.empty()) {
                dc->DrawPolygon(currentSubpath.size(), currentSubpath.data(), 
                                0, 0, fillStyle);
                currentSubpath.clear();
            }
            break;

        // For arcs/curves, you'd do an approximation or skip. 
        // We'll skip them for brevity here.
        default:
            break;
        }
    } // end for segments

    // If there's an open subpath that wasn't closed, fill it now:
    if (!currentSubpath.empty()) {
        dc->DrawPolygon(currentSubpath.size(), currentSubpath.data(), 
                        0, 0, fillStyle);
        currentSubpath.clear();
    }

    // Restore original pen
    dc->SetPen(oldPen);
}

void fxDrawingContext::StrokePath(const fxGraphicsPath& fxpath)
{
    std::visit([&](auto&& c){
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (c) {
                c->StrokePath(fxpath.GetPath());
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>) {
            if (c) {
                StrokePathOnDC(c, fxpath);
            }
        }
    }, m_context);
}

void StrokePathOnDC(wxDC* dc, const fxGraphicsPath& path)
{
    if (!dc) return;

    // Save old brush so we can restore it later
    wxBrush oldBrush = dc->GetBrush();
    // Use a transparent brush to emulate -stroke only-
    dc->SetBrush(*wxTRANSPARENT_BRUSH);

    const auto& segments = path.GetSegments();
    if (segments.empty()) {
        dc->SetBrush(oldBrush);
        return;
    }

    // We’ll accumulate subpath lines in currentSubpath
    std::vector<wxPoint> currentSubpath;
    currentSubpath.reserve(16);

    for (auto& seg : segments)
    {
        switch (seg.type)
        {
        case fxPathSegmentType::MoveTo:
            // If we have an existing subpath, draw it
            if (currentSubpath.size() > 1) {
                // Connect them as lines
                dc->DrawLines(currentSubpath.size(), currentSubpath.data());
            }
            currentSubpath.clear();
            // Start a new subpath
            if (!seg.points.empty()) {
                const auto& p = seg.points[0];
                currentSubpath.push_back(wxPoint((int)p.m_x, (int)p.m_y));
            }
            break;

        case fxPathSegmentType::LineTo:
            if (!seg.points.empty()) {
                const auto& p = seg.points[0];
                currentSubpath.push_back(wxPoint((int)p.m_x, (int)p.m_y));
            }
            break;

        case fxPathSegmentType::Rectangle:
            {
                // Outline a rectangle
                double x1 = seg.points[0].m_x;
                double y1 = seg.points[0].m_y;
                double x2 = seg.points[1].m_x;
                double y2 = seg.points[1].m_y;
                double w  = x2 - x1;
                double h  = y2 - y1;
                dc->DrawRectangle(wxRect((int)x1, (int)y1, (int)w, (int)h));
            }
            break;

        case fxPathSegmentType::Ellipse:
            {
                // Outline a circle or ellipse
                if (seg.points.size() == 1) {
                    // circle: (center, radius)
                    double cx = seg.points[0].m_x;
                    double cy = seg.points[0].m_y;
                    double r  = seg.radius;
                    dc->DrawEllipse((int)(cx - r), (int)(cy - r),
                                    (int)(2 * r),   (int)(2 * r));
                }
                else if (seg.points.size() == 2) {
                    // bounding box
                    double x1 = seg.points[0].m_x;
                    double y1 = seg.points[0].m_y;
                    double x2 = seg.points[1].m_x;
                    double y2 = seg.points[1].m_y;
                    dc->DrawEllipse((int)x1,       (int)y1,
                                    (int)(x2 - x1), (int)(y2 - y1));
                }
            }
            break;

        case fxPathSegmentType::Close:
            // If we have a subpath, close it by connecting last to first
            if (currentSubpath.size() > 1) {
                // Draw the lines
                dc->DrawLines(currentSubpath.size(), currentSubpath.data());
                // Also connect end to start
                dc->DrawLine(currentSubpath.back(), currentSubpath.front());
            }
            currentSubpath.clear();
            break;

        // For arcs/curves, you'd approximate them if you want real stroke
        default:
            break;
        }
    }

    // If there's an unclosed subpath, stroke it as lines
    if (currentSubpath.size() > 1) {
        dc->DrawLines(currentSubpath.size(), currentSubpath.data());
    }

    // Restore original brush
    dc->SetBrush(oldBrush);
}


//--------------------------------------
// Flush (if supported)
//--------------------------------------
void fxDrawingContext::Flush()
{
    std::visit([&](auto&& c) {
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>)
        {
            if (c) {
                c->Flush();  // Delegate to native GC
            }
        }
        else if constexpr (std::is_same_v<T, wxDC*>)
        {
            // wxDC doesn't have Flush(); safe no-op
        }
        // If monostate: no-op
    }, m_context);
}


//--------------------------------------
// StrokeLine
//--------------------------------------
void fxDrawingContext::StrokeLine(wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) {
                ctx->StrokeLine(x1, y1, x2, y2);
            }
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) {
                ctx->DrawLine(wxPoint(x1, y1), wxPoint(x2, y2));
            }
        }
    }, m_context);
}

void fxDrawingContext::StrokeLines(size_t n, const wxPoint2DDouble* beginPoints, const wxPoint2DDouble* endPoints)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) {
                ctx->StrokeLines(n, beginPoints, endPoints);
            }
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx) {
                for (size_t i = 0; i < n; ++i) {
                    ctx->DrawLine(
                        wxPoint(beginPoints[i].m_x, beginPoints[i].m_y),
                        wxPoint(endPoints[i].m_x, endPoints[i].m_y)
                    );
                }
            }
        }
    }, m_context);
}

void fxDrawingContext::StrokeLines(size_t n, const wxPoint2DDouble* points)
{
    std::visit([&](auto&& ctx) {
        using T = std::decay_t<decltype(ctx)>;

        if constexpr (std::is_same_v<T, wxGraphicsContext*>) {
            if (ctx) {
                ctx->StrokeLines(n, points);
            }
        } else if constexpr (std::is_same_v<T, wxDC*>) {
            if (ctx && n > 1) {
                for (size_t i = 0; i < n - 1; ++i) {
                    ctx->DrawLine(
                        wxPoint(points[i].m_x, points[i].m_y),
                        wxPoint(points[i + 1].m_x, points[i + 1].m_y)
                    );
                }
            }
        }
    }, m_context);
}

void fxDrawingContext::Scale(wxDouble xScale, wxDouble yScale)
{
    std::visit([&](auto&& ctx){
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>)
        {
            if (ctx) ctx->Scale(xScale, yScale);
        }
        else if constexpr (std::is_same_v<T, wxDC*>)
        {
            // wxDC doesn't support scaling ó no-op
        }
    }, m_context);
}

bool fxDrawingContext::SetAntialiasMode(wxAntialiasMode mode)
{
    bool supported = false;

    std::visit([&](auto&& ctx){
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>)
        {
            if (ctx) {
                supported = ctx->SetAntialiasMode(mode);
            }
        }
        // wxDC doesn't support antialiasing mode ó leave as false
    }, m_context);

    return supported;
}

wxAntialiasMode fxDrawingContext::GetAntialiasMode() const
{
    wxAntialiasMode mode = wxANTIALIAS_DEFAULT;

    std::visit([&](auto&& ctx){
        using T = std::decay_t<decltype(ctx)>;
        if constexpr (std::is_same_v<T, wxGraphicsContext*>)
        {
            if (ctx) {
                mode = ctx->GetAntialiasMode();
            }
        }
        // wxDC has no AA mode ó leave as default
    }, m_context);

    return mode;
}
