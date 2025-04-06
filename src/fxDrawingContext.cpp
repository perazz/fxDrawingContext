// fxDrawingContext.cpp
#include "fxDrawingContext.hpp"
#include <wx/graphics.h>
#include <wx/dcgraph.h>
#include <wx/log.h>

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

fxGraphicsPath fxDrawingContext::NewPath()
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
