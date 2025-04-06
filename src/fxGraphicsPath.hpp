#ifndef FXGRAPHICSPATH_HPP
#define FXGRAPHICSPATH_HPP

#include <wx/graphics.h>
#include <vector>
#include <cmath>

// Simple path segment types
enum class fxPathSegmentType {
    MoveTo,
    LineTo,
    QuadCurveTo,
    CurveTo,
    Arc,
    ArcTo,
    Rectangle,
    RoundedRectangle,
    Ellipse,
    Close
};

struct fxPathSegment {
    fxPathSegmentType type;
    std::vector<wxPoint2DDouble> points; 
    double radius = 0.0;
    double startAngle = 0.0;
    double endAngle = 0.0;
    bool clockwise = false; 
};

class fxGraphicsPath
{
public:
    
    // ------------------------------------------
    // 1) Default constructor (empty path)
    // ------------------------------------------
    fxGraphicsPath()
        : m_gc(nullptr)  // no GC => tracking only
    {
        // m_path remains empty
        // m_segments is empty
    }
    
    // If gc is null, we won't build an internal wxGraphicsPath. We'll just track geometry in m_segments
    fxGraphicsPath(wxGraphicsContext* gc)
        : m_gc(gc)
    {
        if (m_gc) {
            m_path = m_gc->CreatePath();
        }
    }

    //================================================
    // 1) MoveTo
    //================================================
    void MoveToPoint(wxDouble x, wxDouble y)
    {
        // If we have a GC, do the real path op
        if (m_gc) {
            m_path.MoveToPoint(x, y);
        }
        // Always store the geometry
        m_segments.push_back({fxPathSegmentType::MoveTo, {{x, y}}});
    }

    void MoveToPoint(const wxPoint2DDouble& p)
    {
        MoveToPoint(p.m_x, p.m_y);
    }

    //================================================
    // 2) LineTo
    //================================================
    void AddLineToPoint(wxDouble x, wxDouble y)
    {
        if (m_gc) {
            m_path.AddLineToPoint(x, y);
        }
        m_segments.push_back({fxPathSegmentType::LineTo, {{x, y}}});
    }

    void AddLineToPoint(const wxPoint2DDouble& p)
    {
        AddLineToPoint(p.m_x, p.m_y);
    }

    //================================================
    // 3) Cubic bezier
    //================================================
    void AddCurveToPoint(wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y)
    {
        if (m_gc) {
            m_path.AddCurveToPoint(cx1, cy1, cx2, cy2, x, y);
        }
        m_segments.push_back({fxPathSegmentType::CurveTo, {{cx1, cy1}, {cx2, cy2}, {x, y}}});
    }

    void AddCurveToPoint(const wxPoint2DDouble& c1, const wxPoint2DDouble& c2, const wxPoint2DDouble& e)
    {
        AddCurveToPoint(c1.m_x, c1.m_y, c2.m_x, c2.m_y, e.m_x, e.m_y);
    }

    //================================================
    // 4) Quadratic bezier
    //================================================
    void AddQuadCurveToPoint(wxDouble cx, wxDouble cy, wxDouble x, wxDouble y)
    {
        if (m_gc) {
            m_path.AddQuadCurveToPoint(cx, cy, x, y);
        }
        m_segments.push_back({fxPathSegmentType::QuadCurveTo, {{cx, cy}, {x, y}}});
    }

    //================================================
    // 5) Arc
    //================================================
    void AddArc(wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise)
    {
        if (m_gc) {
            m_path.AddArc(x, y, r, startAngle, endAngle, clockwise);
        }
        m_segments.push_back({fxPathSegmentType::Arc, {{x, y}}, r, startAngle, endAngle, clockwise});
    }

    void AddArc(const wxPoint2DDouble& c, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise)
    {
        AddArc(c.m_x, c.m_y, r, startAngle, endAngle, clockwise);
    }

    //================================================
    // 6) ArcTo
    //================================================
    void AddArcToPoint(wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2, wxDouble r)
    {
        if (m_gc) {
            m_path.AddArcToPoint(x1, y1, x2, y2, r);
        }
        m_segments.push_back({fxPathSegmentType::ArcTo, {{x1, y1}, {x2, y2}}, r});
    }

    //================================================
    // 7) Circle
    //================================================
    void AddCircle(wxDouble x, wxDouble y, wxDouble r)
    {
        if (m_gc) {
            m_path.AddCircle(x, y, r);
        }
        // We'll store it as an ellipse with center + radius
        m_segments.push_back({fxPathSegmentType::Ellipse, {{x, y}}, r});
    }

    //================================================
    // 8) Ellipse
    //================================================
    void AddEllipse(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
    {
        if (m_gc) {
            m_path.AddEllipse(x, y, w, h);
        }
        m_segments.push_back({fxPathSegmentType::Ellipse, {{x, y}, {x + w, y + h}}});
    }

    //================================================
    // 9) Rectangle
    //================================================
    void AddRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
    {
        if (m_gc) {
            m_path.AddRectangle(x, y, w, h);
        }
        m_segments.push_back({fxPathSegmentType::Rectangle, {{x, y}, {x + w, y + h}}});
    }

    //================================================
    // 10) Rounded rectangle
    //================================================
    void AddRoundedRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius)
    {
        if (m_gc) {
            m_path.AddRoundedRectangle(x, y, w, h, radius);
        }
        m_segments.push_back({fxPathSegmentType::RoundedRectangle, {{x, y}, {x + w, y + h}}, radius});
    }

    //================================================
    // 11) Add entire path
    //================================================
    void AddPath(const fxGraphicsPath& other)
    {
        // If both paths share the same GC (or we have one GC and the other is tracking-only),
        // we can safely merge the native wxGraphicsPath data:
        if (m_gc && (other.m_gc == m_gc || other.m_gc == nullptr))
        {
            m_path.AddPath(other.m_path);
        }
        // else if you'd like, handle the mismatch case differently (log a warning, etc.)

        // Always merge geometry data:
        m_segments.insert(m_segments.end(), 
                          other.m_segments.begin(),
                          other.m_segments.end());
    }

    //================================================
    // 12) Close
    //================================================
    void CloseSubpath()
    {
        if (m_gc) {
            m_path.CloseSubpath();
        }
        m_segments.push_back({fxPathSegmentType::Close, {}});
    }

    //================================================
    // 13) Transform
    //================================================
    void Transform(const wxGraphicsMatrix& matrix)
    {
        if (m_gc) {
            m_path.Transform(matrix);
        }
        // Also transform our stored points
        for (auto& seg : m_segments)
        {
            for (auto& pt : seg.points)
            {
                matrix.TransformPoint(&pt.m_x, &pt.m_y);
            }
        }
    }

    //================================================
    // 14) Box, current point, Contains
    //================================================
    wxRect2DDouble GetBox() const
    {
        if (m_gc) {
            return m_path.GetBox();
        }
        // Approx: we can compute bounding box from m_segments
        // or just return empty if you prefer
        return ComputeSegmentsBoundingBox();
    }

    void GetBox(wxDouble* x, wxDouble* y, wxDouble* w, wxDouble* h) const
    {
        if (m_gc) {
            m_path.GetBox(x, y, w, h);
        }
        else {
            auto box = ComputeSegmentsBoundingBox();
            if (x) *x = box.m_x;
            if (y) *y = box.m_y;
            if (w) *w = box.m_width;
            if (h) *h = box.m_height;
        }
    }

    wxPoint2DDouble GetCurrentPoint() const
    {
        if (m_gc) {
            return m_path.GetCurrentPoint();
        }
        // If no GC, approximate by last segmentÍs last point
        if (!m_segments.empty() && !m_segments.back().points.empty()) {
            return m_segments.back().points.back();
        }
        return {0, 0};
    }

    void GetCurrentPoint(wxDouble* x, wxDouble* y) const
    {
        auto pt = GetCurrentPoint();
        if (x) *x = pt.m_x;
        if (y) *y = pt.m_y;
    }

    bool Contains(const wxPoint2DDouble& pt, wxPolygonFillMode fillStyle = wxODDEVEN_RULE) const
    {
        if (m_gc) {
            return m_path.Contains(pt, fillStyle);
        }
        // If no GC, you could do your own point-in-poly check for lines/polygons
        return false;
    }

    bool Contains(wxDouble x, wxDouble y, wxPolygonFillMode fillStyle = wxODDEVEN_RULE) const
    {
        return Contains({x, y}, fillStyle);
    }

    //================================================
    // 15) Native path
    //================================================
    void* GetNativePath() const
    {
        if (m_gc) {
            return m_path.GetNativePath();
        }
        return nullptr;
    }

    void UnGetNativePath(void* p) const
    {
        if (m_gc) {
            m_path.UnGetNativePath(p);
        }
    }

    //================================================
    // Accessors
    //================================================
    const wxGraphicsPath& GetPath() const { return m_path; }
    wxGraphicsContext*    GetContext() const { return m_gc; }

    const std::vector<fxPathSegment>& GetSegments() const { return m_segments; }

private:
    wxGraphicsContext* m_gc = nullptr;
    wxGraphicsPath     m_path; 
    std::vector<fxPathSegment> m_segments;

    // Helper: compute bounding box from the segment data
    wxRect2DDouble ComputeSegmentsBoundingBox() const
    {
        if (m_segments.empty())
            return wxRect2DDouble(0,0,0,0);

        double minx =  1e9, miny =  1e9;
        double maxx = -1e9, maxy = -1e9;

        for (auto& seg : m_segments) {
            for (auto& pt : seg.points) {
                if (pt.m_x < minx) minx = pt.m_x;
                if (pt.m_x > maxx) maxx = pt.m_x;
                if (pt.m_y < miny) miny = pt.m_y;
                if (pt.m_y > maxy) maxy = pt.m_y;
            }
        }
        return wxRect2DDouble(minx, miny, maxx - minx, maxy - miny);
    }
};

// Drawing helper functions
std::vector<wxPoint2DDouble> ApproxQuadBezier(double x0, double y0, double cx, double cy, double x1, double y1, int steps = 12);
std::vector<wxPoint2DDouble> ApproxCubicBezier(double x0, double y0, double cx1, double cy1, double cx2, double cy2, double x1, double y1, int steps = 12);
std::vector<wxPoint2DDouble> ApproxArc(double cx, double cy, double r, double startAngle, double endAngle, bool clockwise, int steps = 12);


#endif // FXGRAPHICSPATH_HPP
