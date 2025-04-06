// fxDrawingContext.hpp

#ifndef FXDRAWINGCONTEXT_HPP
#define FXDRAWINGCONTEXT_HPP

#include <variant>
#include <memory>       // for std::shared_ptr
#include <wx/dc.h>
#include <wx/graphics.h>
#include <wx/dcgraph.h>
#include <wx/dcsvg.h>
#include <vector>
#include "fxGraphicsPath.hpp"  // the fxGraphicsPath definition

class fxDrawingContext
{
public:
    using ContextVariant = std::variant<std::monostate, wxGraphicsContext*, wxDC*>;

    fxDrawingContext() = default;
    fxDrawingContext(wxGraphicsContext* gc) : m_context(gc) {}
    fxDrawingContext(wxDC* dc) : m_context(dc) {}
    ~fxDrawingContext() = default;  // no manual cleanup needed

    bool IsValid() const {
        return !std::holds_alternative<std::monostate>(m_context);
    }
    bool IsGC() const {
        return std::holds_alternative<wxGraphicsContext*>(m_context);
    }
    bool IsDC() const {
        return std::holds_alternative<wxDC*>(m_context);
    }

    // Font settings
    void SetFont(const wxFont& font, const wxColour& colour = *wxBLACK); 
    void GetPartialTextExtents(const wxString &text, wxArrayDouble &widths) const;
    void GetTextExtent(const wxString &text,
                       wxDouble *width,
                       wxDouble *height,
                       wxDouble *descent = nullptr,
                       wxDouble *externalLeading = nullptr) const;    
    
    // Basic draws
    void SetBrush(const wxBrush& brush);
    void SetPen(const wxPen& pen);
    void DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h);
    void DrawText(const wxString& text, wxDouble x, wxDouble y);

    // Path handling
    fxGraphicsPath NewPath();
    void DrawPath(const fxGraphicsPath& fxpath);

    // Access to underlying variant (optional, if you need it)
    const ContextVariant& GetVariant() const { return m_context; }

private:
    ContextVariant m_context;
    // Shared pointers will automatically free the objects when nobody references them.
    std::vector<std::shared_ptr<wxGraphicsContext>> m_tempGCs;
};

// Draw path fallback for wxDC
void DrawPathOnDC(wxDC* dc, const fxGraphicsPath& path, wxPolygonFillMode fillMode = wxODDEVEN_RULE);

#endif // FXDRAWINGCONTEXT_HPP
