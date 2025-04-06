#include "fxGraphicsPath.hpp"
#include <cmath>
#include <vector>
#include <wx/gdicmn.h> 


// Approximate a circular arc from angleStart to angleEnd (radians) around center (cx,cy), radius r.
// If clockwise is true, we go clockwise; else counterclockwise.
std::vector<wxPoint2DDouble> ApproxArc(
    double cx, double cy,
    double r,
    double angleStart,
    double angleEnd,
    bool clockwise,
    int steps)
{
    std::vector<wxPoint2DDouble> pts;
    pts.reserve(steps + 1);

    // If clockwise, swap or invert the param direction
    double totalAngle = angleEnd - angleStart;
    if (clockwise) {
        totalAngle = angleStart - angleEnd;
    }

    for (int i = 0; i <= steps; ++i)
    {
        double t = static_cast<double>(i) / steps; // 0..1
        double angle = angleStart + t * totalAngle;

        if (clockwise) {
            // for clockwise, angle goes from start down to end
            angle = angleStart - t * (angleStart - angleEnd);
        }

        double x = cx + r * std::cos(angle);
        double y = cy + r * std::sin(angle);
        pts.push_back(wxPoint2DDouble(x, y));
    }
    return pts;
}

// Approximate a quadratic Bezier with steps linear segments
// control points: (x0,y0), (cx,cy), (x1,y1)
std::vector<wxPoint2DDouble> ApproxQuadBezier(
    double x0, double y0,
    double cx, double cy,
    double x1, double y1,
    int steps)
{
    std::vector<wxPoint2DDouble> pts;
    pts.reserve(steps + 1);

    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps; // 0..1
        double mt = 1.0 - t;

        // Quadratic formula: B(t) = (1-t)^2 * P0 + 2t(1-t) * P1 + t^2 * P2
        double px = mt*mt*x0 + 2*t*mt*cx + t*t*x1;
        double py = mt*mt*y0 + 2*t*mt*cy + t*t*y1;
        pts.push_back(wxPoint2DDouble(px, py));
    }
    return pts;
}

// Approximate a cubic Bezier with steps linear segments
// control points: (x0,y0), (cx1,cy1), (cx2,cy2), (x1,y1)
std::vector<wxPoint2DDouble> ApproxCubicBezier(
    double x0, double y0,
    double cx1, double cy1,
    double cx2, double cy2,
    double x1, double y1,
    int steps)
{
    std::vector<wxPoint2DDouble> pts;
    pts.reserve(steps + 1);

    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double mt = 1.0 - t;

        // Cubic formula: B(t) = (1-t)^3 * P0 + 3t(1-t)^2 * P1 + 3t^2(1-t) * P2 + t^3 * P3
        double px = mt*mt*mt*x0 
                  + 3*t*mt*mt*cx1
                  + 3*t*t*mt*cx2
                  + t*t*t*x1;
        double py = mt*mt*mt*y0 
                  + 3*t*mt*mt*cy1
                  + 3*t*t*mt*cy2
                  + t*t*t*y1;
        pts.push_back(wxPoint2DDouble(px, py));
    }
    return pts;
}
