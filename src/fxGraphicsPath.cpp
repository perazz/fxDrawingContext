#include "fxGraphicsPath.hpp"
#include <cmath>
#include <vector>
#include <wx/gdicmn.h> 


// Approximate a circular arc from angleStart to angleEnd (radians) around center (cx,cy), radius r.
// If clockwise is true, we go clockwise; else counterclockwise.
std::vector<wxPoint2DDouble> ApproxArc(wxPoint2DDouble center, double radius,
                                       double angleStart, double angleEnd,
                                       bool clockwise, int steps)
{
    std::vector<wxPoint2DDouble> pts;
    pts.reserve(steps + 1);

    double totalAngle = angleEnd - angleStart;
    double direction = clockwise ? -1.0 : 1.0;

    for (int i = 0; i <= steps; ++i)
    {
        double t = static_cast<double>(i) / steps;
        double angle = angleStart + direction * t * std::abs(totalAngle);

        double x = center.m_x + radius * std::cos(angle);
        double y = center.m_y + radius * std::sin(angle);
        pts.emplace_back(x, y);
    }
    return pts;
}


// Approximate a quadratic Bezier with steps linear segments
// control points: (x0,y0), (cx,cy), (x1,y1)
std::vector<wxPoint2DDouble> ApproxQuadBezier(wxPoint2DDouble p0, wxPoint2DDouble c,
                                              wxPoint2DDouble p1, int steps)
{
    std::vector<wxPoint2DDouble> pts;
    pts.reserve(steps + 1);

    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double mt = 1.0 - t;

        wxPoint2DDouble pt = p0 * (mt * mt) + c  * (2 * t * mt) + p1 * (t * t);

        pts.push_back(pt);
    }
    return pts;
}


// Approximate a cubic Bezier with steps linear segments
// control points: (x0,y0), (cx1,cy1), (cx2,cy2), (x1,y1)
std::vector<wxPoint2DDouble> ApproxCubicBezier(wxPoint2DDouble p0, wxPoint2DDouble c1,
                                               wxPoint2DDouble c2, wxPoint2DDouble p1, int steps)
{
    std::vector<wxPoint2DDouble> pts;
    pts.reserve(steps + 1);

    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double mt = 1.0 - t;

        wxPoint2DDouble pt =
            p0  * (mt * mt * mt) +
            c1  * (3 * t * mt * mt) +
            c2  * (3 * t * t * mt) +
            p1  * (t * t * t);

        pts.push_back(pt);
    }
    return pts;
}

