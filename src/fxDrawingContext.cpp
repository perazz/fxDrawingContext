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

    // We'll keep track of the last point, to connect lines or arcs
    wxPoint2DDouble lastPt(0,0);
    bool haveLastPt = false;

    for (auto& seg : segments)
    {
        switch (seg.type)
        {
        case fxPathSegmentType::MoveTo:
            {
                // If we have an open subpath, fill it:
                if (!currentSubpath.empty()) {
                    dc->DrawLines(currentSubpath.size(), currentSubpath.data());
                    currentSubpath.clear();
                }
                if (!seg.points.empty()) {
                    lastPt = seg.points[0];
                    haveLastPt = true;
                    currentSubpath.push_back(wxPoint((int)lastPt.m_x, (int)lastPt.m_y));
                }
            }
            break;

        case fxPathSegmentType::LineTo:
            {
                if (!seg.points.empty()) {
                    lastPt = seg.points[0];
                    haveLastPt = true;
                    currentSubpath.push_back(wxPoint((int)lastPt.m_x, (int)lastPt.m_y));
                }
            }
            break;

        //-----------------------------------
        // Quadratic Bezier
        //-----------------------------------
        case fxPathSegmentType::QuadCurveTo:
            {
                // We have control point (cx,cy) and end point (x,y)
                if (seg.points.size() >= 2 && haveLastPt) {
                    auto poly = ApproxQuadBezier(lastPt, seg.points[0], seg.points[1], 12);
                    // The first point is lastPt again, so skip it to avoid duplication
                    for (size_t i=1; i<poly.size(); i++){
                        currentSubpath.push_back(wxPoint((int)poly[i].m_x, (int)poly[i].m_y));
                    }
                    lastPt = seg.points[1];
                }
            }
            break;

        //-----------------------------------
        // Cubic Bezier
        //-----------------------------------
        case fxPathSegmentType::CurveTo:
            {
                if (seg.points.size() >= 3 && haveLastPt) {
                    auto poly = ApproxCubicBezier(lastPt, seg.points[0], seg.points[1], seg.points[2], 12);
                    for (size_t i=1; i<poly.size(); i++){
                        currentSubpath.push_back(wxPoint((int)poly[i].m_x, (int)poly[i].m_y));
                    }
                    lastPt = seg.points[2];
                }
            }
            break;

        //-----------------------------------
        // Arc
        //-----------------------------------
        case fxPathSegmentType::Arc:
            {
                if (!seg.points.empty()) {
                    double r = seg.radius;
                    double startA = seg.startAngle;
                    double endA   = seg.endAngle;
                    bool cw       = seg.clockwise;
                    // If we have a 'lastPt' we could do a line from lastPt to arc start, 
                    // but in wxGraphicsPath AddArc typically 'moves' to start of arc first.
                    auto arcPts = ApproxArc(seg.points[0], r, startA, endA, cw, 12);
                    // optional: if you want a line from lastPt to arcPts[0], do so
                    for (size_t i = 0; i < arcPts.size(); i++){
                        currentSubpath.push_back(wxPoint((int)arcPts[i].m_x, (int)arcPts[i].m_y));
                    }
                    lastPt = arcPts.back();
                    haveLastPt = true;
                }
            }
            break;

        //-----------------------------------
        // ArcTo
        //-----------------------------------
        case fxPathSegmentType::ArcTo:
            {
                // ArcTo is a bit more complicated to approximate accurately because
                // it’s tangent from lastPt -> arc -> seg.points[1]. Let's do a naive approach:
                if (seg.points.size() >= 2 && haveLastPt) {
                    // We'll interpret (x1,y1) and (x2,y2) with radius r
                    double x1 = seg.points[0].m_x;
                    double y1 = seg.points[0].m_y;
                    double x2 = seg.points[1].m_x;
                    double y2 = seg.points[1].m_y;
                    double r  = seg.radius;
                    // We'll do a rough approach: 
                    // 1) line from lastPt to (x1,y1)
                    currentSubpath.push_back(wxPoint((int)x1, (int)y1));
                    // 2) approximate an arc with center = ??? 
                    // Actually, ArcTo is more complex. We'll do a small hack:
                    // just do a line from (x1,y1) to (x2,y2). Real arcTo is tangent arcs. 
                    // For a real approach, you'd compute the tangent points. 
                    currentSubpath.push_back(wxPoint((int)x2, (int)y2));
                    lastPt = wxPoint2DDouble(x2,y2);
                }
            }
            break;

        case fxPathSegmentType::Rectangle:
            {
                wxDouble x1 = seg.points[0].m_x;
                wxDouble y1 = seg.points[0].m_y;
                wxDouble x2 = seg.points[1].m_x;
                wxDouble y2 = seg.points[1].m_y;
                dc->DrawRectangle(wxRect((int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1)));
            }
            break;
            
        case fxPathSegmentType::RoundedRectangle:
        {
            if (seg.points.size() >= 2) {
                const double r = seg.radius;
                const int steps = 6;

                double x1 = seg.points[0].m_x;
                double y1 = seg.points[0].m_y;
                double x2 = seg.points[1].m_x;
                double y2 = seg.points[1].m_y;

                wxPoint2DDouble TL(x1 + r, y1 + r);
                wxPoint2DDouble TR(x2 - r, y1 + r);
                wxPoint2DDouble BR(x2 - r, y2 - r);
                wxPoint2DDouble BL(x1 + r, y2 - r);

                auto arcTL = ApproxArc(TL, r, M_PI,     3 * M_PI / 2, false, steps);
                auto arcTR = ApproxArc(TR, r, 3 * M_PI / 2, 2 * M_PI, false, steps);
                auto arcBR = ApproxArc(BR, r, 0,        M_PI / 2,     false, steps);
                auto arcBL = ApproxArc(BL, r, M_PI / 2, M_PI,         false, steps);

                std::vector<wxPoint> outline;
                for (const auto& pt : arcTL) outline.emplace_back(wxPoint(int(pt.m_x), int(pt.m_y)));
                for (const auto& pt : arcTR) outline.emplace_back(wxPoint(int(pt.m_x), int(pt.m_y)));
                for (const auto& pt : arcBR) outline.emplace_back(wxPoint(int(pt.m_x), int(pt.m_y)));
                for (const auto& pt : arcBL) outline.emplace_back(wxPoint(int(pt.m_x), int(pt.m_y)));

                dc->DrawPolygon(outline.size(), outline.data(), 0, 0, fillMode);

                if (!outline.empty()) {
                    lastPt = wxPoint2DDouble(outline.back().x, outline.back().y);
                    haveLastPt = true;
                }
            }
        }
        break;


        case fxPathSegmentType::Ellipse:
            // Already handled or partial. If it’s bounding box or circle center, see code snippet:
            {
                if (seg.points.size() == 1) {
                    // (center, radius)
                    double cx = seg.points[0].m_x;
                    double cy = seg.points[0].m_y;
                    double r  = seg.radius;
                    dc->DrawEllipse((int)(cx - r), (int)(cy - r), (int)(2*r), (int)(2*r));
                }
                else if (seg.points.size() == 2) {
                    double x1 = seg.points[0].m_x;
                    double y1 = seg.points[0].m_y;
                    double x2 = seg.points[1].m_x;
                    double y2 = seg.points[1].m_y;
                    dc->DrawEllipse((int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1));
                }
            }
            break;

        case fxPathSegmentType::Close:
            {
                if (!currentSubpath.empty()) {
                    dc->DrawPolygon(currentSubpath.size(), currentSubpath.data(), 0, 0, fillMode);
                    currentSubpath.clear();
                }
            }
            break;
            
            

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

void FillPathOnDC(wxDC* dc, const fxGraphicsPath& path, wxPolygonFillMode fillMode)
{
    if (!dc) return;

    wxPen oldPen = dc->GetPen();
    dc->SetPen(*wxTRANSPARENT_PEN);  // Fill only: no stroke

    DrawPathOnDC(dc, path, fillMode);  // Use correct fill mode

    dc->SetPen(oldPen);  // Restore original
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

    wxBrush oldBrush = dc->GetBrush();
    dc->SetBrush(*wxTRANSPARENT_BRUSH);  // Stroke only: no fill

    DrawPathOnDC(dc, path, wxODDEVEN_RULE);  // Polygon mode doesn't matter for stroke

    dc->SetBrush(oldBrush);  // Restore original
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
            // wxDC doesn't support scaling — no-op
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
        // wxDC doesn't support antialiasing mode — leave as false
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
        // wxDC has no AA mode — leave as default
    }, m_context);

    return mode;
}

