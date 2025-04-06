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

enum class ExportFormat
{
    JPEG = 0,
    PNG,
    SVG,
    PDF
};

const wxString EXPORT_FILE_PATTERN = 
    "JPEG files (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
    "PNG files (*.png)|*.png|"
    "SVG files (*.svg)|*.svg|"
    "PDF files (*.pdf)|*.pdf|";

class fxDrawingContext
{
public:
    using ContextVariant = std::variant<std::monostate, wxGraphicsContext*, wxDC*>;

    fxDrawingContext() = default;
    fxDrawingContext(wxGraphicsContext* gc);
    fxDrawingContext(wxDC* dc);
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

    // Get context size
    void GetSize(wxDouble* width, wxDouble* height) const;
    wxSize GetSize() const;
    
    // Appearance
    void Scale(wxDouble xScale, wxDouble yScale);
    bool SetAntialiasMode(wxAntialiasMode mode);
    wxAntialiasMode GetAntialiasMode() const;    
    
    // Font settings
    void SetFont(const wxFont& font, const wxColour& colour = *wxBLACK); 
    void GetPartialTextExtents(const wxString &text, wxArrayDouble &widths) const;
    void GetTextExtent(const wxString &text,
                       wxDouble *width,
                       wxDouble *height,
                       wxDouble *descent = nullptr,
                       wxDouble *externalLeading = nullptr) const;    
    
    void GetTextSize(const wxFont& font,
                     const wxString& text,
                     wxDouble& width,
                     wxDouble& height,
                     wxDouble angleRad = 0.0,
                     wxDouble* descent = nullptr,
                     wxDouble* externalLeading = nullptr);
        
    // Basic draws
    void SetBrush(const wxBrush& brush);
    void SetPen(const wxPen& pen);
    void DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h);
    void DrawText(const wxString& text, wxDouble x, wxDouble y);
    void DrawText(const wxString& text, wxDouble x, wxDouble y, wxDouble angleRad);
    
    // Lines
    void StrokeLine(wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2);
    void StrokeLines(size_t n, const wxPoint2DDouble* beginPoints, const wxPoint2DDouble* endPoints);
    void StrokeLines(size_t n, const wxPoint2DDouble* points);    

    // Path handling
    fxGraphicsPath CreatePath();
    void DrawPath(const fxGraphicsPath& fxpath);
    
    // Fills the given path with the current brush.
    // On a real wxGraphicsContext, calls FillPath(path).
    // On a raw wxDC, approximates fill from the path segments.
    void FillPath(const fxGraphicsPath& path, 
                  wxPolygonFillMode fillStyle = wxODDEVEN_RULE);
    
    // Strokes along the given path with the current pen.
    // On a real wxGraphicsContext, calls StrokePath(path).
    // On a raw wxDC, approximates a stroke from the path segments.
    void StrokePath(const fxGraphicsPath& path);    
    
    // Access to underlying variant (optional, if you need it)
    const ContextVariant& GetVariant() const { return m_context; }

    // Flush the context if supported (e.g., for buffered drawing)
    void Flush();
    
private:
    ContextVariant m_context;
    // We only need one newly created GC for the entire lifetime:
    std::shared_ptr<wxGraphicsContext> m_ownedGC;
};

// Draw path fallback for wxDC
void DrawPathOnDC(wxDC* dc, const fxGraphicsPath& path, wxPolygonFillMode fillMode = wxODDEVEN_RULE);

// Fallback helper to fill on a DC
void FillPathOnDC(wxDC* dc, const fxGraphicsPath& path, wxPolygonFillMode fillStyle = wxODDEVEN_RULE);

// Fallback helper to stroke on a DC
void StrokePathOnDC(wxDC* dc, const fxGraphicsPath& path);

#endif // FXDRAWINGCONTEXT_HPP
