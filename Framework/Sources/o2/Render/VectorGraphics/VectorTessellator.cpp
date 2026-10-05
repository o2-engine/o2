#include "o2/stdafx.h"
#include "VectorTessellator.h"

#include "o2/Render/VectorGraphics/VectorMeshSimplifier.h"
#include "o2/Render/VectorGraphics/VectorTriangleSplit.h"
#include "o2/Utils/Math/Math.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace o2
{
    namespace
    {
        const double kPi = 3.14159265358979323846;
        const double kGrid = 4096.0;           // All geometry is snapped to 1/kGrid of a pixel
        const double kSliverWidth = 0.002;     // Cells narrower than this are snapping noise
        const double kSmoothCornerCos = 0.966; // Corners up to 15 degrees turn share mitered fringe points
        const double kSmoothCurveCos = 0.766;  // The same for vertices of flattened curves, up to 40 degrees
        const double kSmoothJointCos = 0.985;  // Path segments that meet within 10 degrees make no corner
        const double kMiterRoom = 0.9;         // Part of an edge that the fringes of its ends may take
        const double kMinBandLength = 1e-3;
        const double kMinFanSize = 0.2;        // Smaller faces get no vertex in the center
        const double kMaxFanError = 1.0/128.0;
        const double kMaxPatchEdge = 1.5;      // Longer edges between the cells of their ends get strips
        const double kCurveEndStub = 1.0/32.0;
        const double kMinChordTurn = 1e-3;     // Sine of the angle between a chord and its curve that is not a turn
        const int    kMaxFlattenDepth = 8;
        const size_t kMaxTriangleVertices = 6000000;
        const double kMaxPlaneError = 0.04;    // Of a strip against the exact coverage, larger makes its cell exact
        const double kMaxExactCell = 2.0;
        const double kLatticeEpsilon = 1e-6;
        const double kFineTolerance = 0.1;     // Part of the curve tolerance that is used for the tight curves
        const double kMinSmoothRadius = 32.0;  // Curves of smaller radius get cells instead of the mitered strips
        const float  kStopsTolerance = 0.75f;  // Color levels that the simplified gradient stops may be off
        const double kSplitSnap = 1.0/1024.0;  // Pixels from a vertex to a cut of triangles that goes through it

        struct Point
        {
            double x = 0, y = 0;

            Point() = default;
            Point(double x, double y):x(x), y(y) {}

            Point operator+(const Point& other) const { return Point(x + other.x, y + other.y); }
            Point operator-(const Point& other) const { return Point(x - other.x, y - other.y); }
            Point operator*(double value) const { return Point(x*value, y*value); }
            bool operator==(const Point& other) const { return x == other.x && y == other.y; }
        };

        double Dot(const Point& a, const Point& b)
        {
            return a.x*b.x + a.y*b.y;
        }

        double Cross(const Point& a, const Point& b)
        {
            return a.x*b.y - a.y*b.x;
        }

        double Length(const Point& a)
        {
            return sqrt(a.x*a.x + a.y*a.y);
        }

        Point Normalized(const Point& vector)
        {
            double length = Length(vector);
            return length > 1e-9 ? vector*(1.0/length) : Point();
        }

        double Snap(double value)
        {
            return floor(value*kGrid + 0.5)/kGrid;
        }

        Point Snap(const Point& point)
        {
            return Point(Snap(point.x), Snap(point.y));
        }

        // Vertex of intermediate triangles: position in pixels, coverage 0..1 and gradient ramp offset
        struct CoverageVertex
        {
            double x, y;
            float  coverage;
            float  offset;
        };

        typedef std::vector<CoverageVertex> TriangleSoup;
        typedef std::vector<Point> Polyline;

        // Triangulates convex polygon that may have points inside its sides, keeping all of them as vertices. Ears are
        // cut along the most even coverage, so the mirrored polygon gets the mirrored triangles
        template<typename _coverage>
        void EmitPolygon(std::vector<Point>& polygon, _coverage&& coverage, TriangleSoup& soup)
        {
            const double minArea = 1e-12;

            size_t count = 0;
            for (size_t i = 0; i < polygon.size(); i++)
            {
                if (count == 0 || !(polygon[i] == polygon[count - 1]))
                    polygon[count++] = polygon[i];
            }

            while (count > 1 && polygon[count - 1] == polygon[0])
                count--;

            polygon.resize(count);
            if (count < 3)
                return;

            double area = 0;
            for (size_t i = 0; i < count; i++)
                area += Cross(polygon[i], polygon[(i + 1)%count]);

            if (area < 0)
                std::reverse(polygon.begin(), polygon.end());

            std::vector<CoverageVertex> vertices(count);
            for (size_t i = 0; i < count; i++)
                vertices[i] = { polygon[i].x, polygon[i].y, (float)coverage(polygon[i]), 0.0f };

            auto earArea = [&](size_t i)
            {
                const CoverageVertex& prev = vertices[(i + vertices.size() - 1)%vertices.size()];
                const CoverageVertex& next = vertices[(i + 1)%vertices.size()];
                const CoverageVertex& vertex = vertices[i];
                return (vertex.x - prev.x)*(next.y - vertex.y) - (vertex.y - prev.y)*(next.x - vertex.x);
            };

            while (vertices.size() > 3)
            {
                size_t size = vertices.size();
                size_t ear = size;
                double earStep = 0, earSize = 0;
                for (size_t i = 0; i < size; i++)
                {
                    double currentSize = earArea(i);
                    if (currentSize <= minArea)
                        continue;

                    double step = fabs(vertices[(i + size - 1)%size].coverage - vertices[(i + 1)%size].coverage);
                    if (ear == size || step < earStep || (step == earStep && currentSize < earSize))
                    {
                        ear = i;
                        earStep = step;
                        earSize = currentSize;
                    }
                }

                if (ear == size)
                    return;

                soup.push_back(vertices[(ear + size - 1)%size]);
                soup.push_back(vertices[ear]);
                soup.push_back(vertices[(ear + 1)%size]);
                vertices.erase(vertices.begin() + ear);
            }

            if (earArea(1) > minArea)
                soup.insert(soup.end(), vertices.begin(), vertices.end());
        }

        struct SweepSegment
        {
            Point top, bottom;
            int   tag = 0;
            int   winding = 0; // +1 when source segment is directed down
            int   startEvent = 0, endEvent = 0;
        };

        struct StripEntry
        {
            double xTop, xBottom;
            int    segment;
        };

        // Horizontal strips between Y of all segments ends and intersections; segments do not cross inside a strip
        class Sweep
        {
        public:
            std::vector<SweepSegment> segments;
            std::vector<double>       events;

        public:
            // Adds segment, returns its index or -1 for horizontal one, that only makes events
            int Add(const Point& a, const Point& b, int tag)
            {
                events.push_back(a.y);
                events.push_back(b.y);

                if (a.y == b.y)
                    return -1;

                SweepSegment segment;
                segment.tag = tag;
                segment.top = a.y < b.y ? a : b;
                segment.bottom = a.y < b.y ? b : a;
                segment.winding = a.y < b.y ? 1 : -1;
                segments.push_back(segment);

                return (int)segments.size() - 1;
            }

            void Prepare()
            {
                std::vector<int> order(segments.size());
                for (size_t i = 0; i < order.size(); i++)
                    order[i] = (int)i;

                std::sort(order.begin(), order.end(), [&](int a, int b)
                {
                    return segments[a].top.y != segments[b].top.y ? segments[a].top.y < segments[b].top.y : a < b;
                });

                std::vector<int> active;
                for (int idx : order)
                {
                    const SweepSegment& segment = segments[idx];

                    size_t kept = 0;
                    for (int other : active)
                    {
                        if (segments[other].bottom.y > segment.top.y)
                            active[kept++] = other;
                    }

                    active.resize(kept);

                    for (int other : active)
                        AddIntersection(segment, segments[other]);

                    active.push_back(idx);
                }

                std::sort(events.begin(), events.end());
                events.erase(std::unique(events.begin(), events.end()), events.end());

                for (SweepSegment& segment : segments)
                {
                    segment.startEvent = EventIndex(segment.top.y);
                    segment.endEvent = EventIndex(segment.bottom.y);
                }
            }

            int EventIndex(double y) const
            {
                return (int)(std::lower_bound(events.begin(), events.end(), y) - events.begin());
            }

            double XAt(int idx, int event) const
            {
                const SweepSegment& segment = segments[idx];
                if (event <= segment.startEvent)
                    return segment.top.x;

                if (event >= segment.endEvent)
                    return segment.bottom.x;

                double coef = (events[event] - segment.top.y)/(segment.bottom.y - segment.top.y);
                return Snap(segment.top.x + (segment.bottom.x - segment.top.x)*coef);
            }

            Point PointAt(int idx, int event) const
            {
                return Point(XAt(idx, event), events[event]);
            }

            // Calls onStrip(event, entries) with the segments crossing each strip, ordered from left to right
            template<typename _function>
            void Run(_function&& onStrip) const
            {
                std::vector<int> order(segments.size());
                for (size_t i = 0; i < order.size(); i++)
                    order[i] = (int)i;

                std::sort(order.begin(), order.end(), [&](int a, int b)
                {
                    return segments[a].startEvent != segments[b].startEvent ?
                        segments[a].startEvent < segments[b].startEvent : a < b;
                });

                std::vector<int> active;
                std::vector<StripEntry> entries;
                size_t next = 0;

                for (int event = 0; event + 1 < (int)events.size(); event++)
                {
                    size_t kept = 0;
                    for (int idx : active)
                    {
                        if (segments[idx].endEvent > event)
                            active[kept++] = idx;
                    }

                    active.resize(kept);

                    while (next < order.size() && segments[order[next]].startEvent <= event)
                        active.push_back(order[next++]);

                    entries.clear();
                    for (int idx : active)
                        entries.push_back({ XAt(idx, event), XAt(idx, event + 1), idx });

                    std::sort(entries.begin(), entries.end(), [](const StripEntry& a, const StripEntry& b)
                    {
                        double sumA = a.xTop + a.xBottom, sumB = b.xTop + b.xBottom;
                        if (sumA != sumB)
                            return sumA < sumB;

                        return a.xTop != b.xTop ? a.xTop < b.xTop : a.segment < b.segment;
                    });

                    if (!entries.empty())
                        onStrip(event, entries);
                }
            }

        private:
            void AddIntersection(const SweepSegment& a, const SweepSegment& b)
            {
                const double epsilon = 1e-9;

                if (Math::Max(a.top.x, a.bottom.x) < Math::Min(b.top.x, b.bottom.x) ||
                    Math::Max(b.top.x, b.bottom.x) < Math::Min(a.top.x, a.bottom.x))
                {
                    return;
                }

                Point directionA = a.bottom - a.top, directionB = b.bottom - b.top;
                double denominator = Cross(directionA, directionB);
                if (fabs(denominator) < 1e-12)
                    return;

                Point offset = b.top - a.top;
                double coefA = Cross(offset, directionB)/denominator;
                double coefB = Cross(offset, directionA)/denominator;
                if (coefA <= epsilon || coefA >= 1.0 - epsilon || coefB <= epsilon || coefB >= 1.0 - epsilon)
                    return;

                events.push_back(a.top.y + directionA.y*coefA);
            }
        };

        // Piece of a face between two segments, merged through the strips while its sides are the same
        struct Cell
        {
            int left, right;
            int eventTop, eventBottom;
            int state;
        };

        class CellBuilder
        {
        public:
            std::vector<Cell> cells;

        public:
            void Add(int event, const StripEntry& left, const StripEntry& right, int state)
            {
                if (left.xTop >= right.xTop && left.xBottom >= right.xBottom)
                    return;

                unsigned long long key = ((unsigned long long)(unsigned)left.segment << 32) | (unsigned)right.segment;
                auto found = mOpened.find(key);
                if (found != mOpened.end() && cells[found->second].eventBottom == event &&
                    cells[found->second].state == state)
                {
                    cells[found->second].eventBottom = event + 1;
                    return;
                }

                mOpened[key] = (int)cells.size();
                cells.push_back({ left.segment, right.segment, event, event + 1, state });
            }

        private:
            std::unordered_map<unsigned long long, int> mOpened;
        };

        // Builds cells polygons with corners of the neighbor cells on their sides, so triangles have no T-junctions
        class CellEmitter
        {
        public:
            CellEmitter(const Sweep& sweep):
                mSweep(sweep), mSegmentEvents(sweep.segments.size()), mGroups(sweep.segments.size()),
                mLineXs(sweep.events.size())
            {
                std::map<double, int> verticals;
                std::map<std::pair<std::pair<double, double>, std::pair<double, double>>, int> equals;
                for (size_t i = 0; i < mGroups.size(); i++)
                {
                    const SweepSegment& segment = sweep.segments[i];
                    if (segment.top.x == segment.bottom.x)
                        mGroups[i] = verticals.insert(std::make_pair(segment.top.x, (int)i)).first->second;
                    else
                    {
                        auto key = std::make_pair(std::make_pair(segment.top.x, segment.top.y),
                                                  std::make_pair(segment.bottom.x, segment.bottom.y));
                        mGroups[i] = equals.insert(std::make_pair(key, (int)i)).first->second;
                    }
                }
            }

            void Register(const Cell& cell)
            {
                for (int segment : { cell.left, cell.right })
                {
                    for (int event : { cell.eventTop, cell.eventBottom })
                    {
                        mSegmentEvents[mGroups[segment]].push_back(event);
                        mLineXs[event].push_back(mSweep.XAt(segment, event));
                    }
                }
            }

            void Finish()
            {
                for (auto& events : mSegmentEvents)
                {
                    std::sort(events.begin(), events.end());
                    events.erase(std::unique(events.begin(), events.end()), events.end());
                }

                for (auto& xs : mLineXs)
                {
                    std::sort(xs.begin(), xs.end());
                    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
                }
            }

            // Appends registered points of segment strictly between events, ordered from eventFrom to eventTo
            void SegmentPoints(int segment, int eventFrom, int eventTo, std::vector<Point>& points) const
            {
                const std::vector<int>& events = mSegmentEvents[mGroups[segment]];
                if (eventFrom < eventTo)
                {
                    for (size_t i = 0; i < events.size(); i++)
                    {
                        if (events[i] > eventFrom && events[i] < eventTo)
                            points.push_back(mSweep.PointAt(segment, events[i]));
                    }
                }
                else
                {
                    for (size_t i = events.size(); i > 0; i--)
                    {
                        if (events[i - 1] < eventFrom && events[i - 1] > eventTo)
                            points.push_back(mSweep.PointAt(segment, events[i - 1]));
                    }
                }
            }

            // Appends registered points of event line strictly between xFrom and xTo, ordered from xFrom to xTo
            void LinePoints(int event, double xFrom, double xTo, std::vector<Point>& points) const
            {
                if (event < 0 || event >= (int)mLineXs.size())
                    return;

                const std::vector<double>& xs = mLineXs[event];
                double y = mSweep.events[event];
                if (xFrom < xTo)
                {
                    for (size_t i = 0; i < xs.size(); i++)
                    {
                        if (xs[i] > xFrom && xs[i] < xTo)
                            points.push_back(Point(xs[i], y));
                    }
                }
                else
                {
                    for (size_t i = xs.size(); i > 0; i--)
                    {
                        if (xs[i - 1] < xFrom && xs[i - 1] > xTo)
                            points.push_back(Point(xs[i - 1], y));
                    }
                }
            }

            void CellPolygon(const Cell& cell, std::vector<Point>& polygon) const
            {
                Point leftTop = mSweep.PointAt(cell.left, cell.eventTop);
                Point rightTop = mSweep.PointAt(cell.right, cell.eventTop);
                Point rightBottom = mSweep.PointAt(cell.right, cell.eventBottom);
                Point leftBottom = mSweep.PointAt(cell.left, cell.eventBottom);

                polygon.clear();
                polygon.push_back(leftTop);
                LinePoints(cell.eventTop, leftTop.x, rightTop.x, polygon);
                polygon.push_back(rightTop);
                SegmentPoints(cell.right, cell.eventTop, cell.eventBottom, polygon);
                polygon.push_back(rightBottom);
                LinePoints(cell.eventBottom, rightBottom.x, leftBottom.x, polygon);
                polygon.push_back(leftBottom);
                SegmentPoints(cell.left, cell.eventBottom, cell.eventTop, polygon);
            }

        private:
            const Sweep& mSweep;

            std::vector<std::vector<int>>    mSegmentEvents;
            std::vector<int>                 mGroups; // Equal segments and vertical ones of one line share the events
            std::vector<std::vector<double>> mLineXs;
        };

        struct BoundaryEdge
        {
            Point from, to;
            int   source;
        };

        // Fills loops with the boundary of the area filled by rule, filled side on the left; or hardTriangles
        void Resolve(const std::vector<Polyline>& polygons, VectorFillRule rule, std::vector<Polyline>* loops,
                     TriangleSoup* hardTriangles)
        {
            const double minEdgeLength = 1.0/32.0;

            Sweep sweep;
            for (const Polyline& polygon : polygons)
            {
                for (size_t i = 0; i < polygon.size(); i++)
                    sweep.Add(polygon[i], polygon[(i + 1)%polygon.size()], 0);
            }

            if (sweep.segments.empty())
                return;

            sweep.Prepare();

            struct Piece
            {
                int  segment, eventTop, eventBottom;
                bool down;
            };

            std::vector<Piece> pieces;
            std::vector<int> lastPieces(sweep.segments.size(), -1);
            std::vector<std::vector<double>> topToggles(sweep.events.size()), bottomToggles(sweep.events.size());
            CellBuilder builder;

            auto inside = [&](int winding)
            {
                return rule == VectorFillRule::NonZero ? winding != 0 : (winding & 1) != 0;
            };

            sweep.Run([&](int event, const std::vector<StripEntry>& entries)
            {
                int winding = 0;
                StripEntry cellLeft = entries[0];

                for (size_t i = 0; i < entries.size();)
                {
                    size_t groupEnd = i + 1;
                    int delta = sweep.segments[entries[i].segment].winding;
                    while (groupEnd < entries.size() && entries[groupEnd].xTop == entries[i].xTop &&
                           entries[groupEnd].xBottom == entries[i].xBottom)
                    {
                        delta += sweep.segments[entries[groupEnd].segment].winding;
                        groupEnd++;
                    }

                    bool before = inside(winding), after = inside(winding + delta);
                    if (before != after)
                    {
                        int segment = entries[i].segment;
                        int last = lastPieces[segment];
                        if (last >= 0 && pieces[last].eventBottom == event && pieces[last].down == before)
                            pieces[last].eventBottom = event + 1;
                        else
                        {
                            lastPieces[segment] = (int)pieces.size();
                            pieces.push_back({ segment, event, event + 1, before });
                        }

                        topToggles[event].push_back(entries[i].xTop);
                        bottomToggles[event].push_back(entries[i].xBottom);

                        if (after)
                            cellLeft = entries[i];
                        else if (hardTriangles)
                            builder.Add(event, cellLeft, entries[i], 1);
                    }

                    winding += delta;
                    i = groupEnd;
                }
            });

            if (hardTriangles)
            {
                CellEmitter emitter(sweep);
                for (const Cell& cell : builder.cells)
                    emitter.Register(cell);

                emitter.Finish();

                std::vector<Point> polygon;
                for (const Cell& cell : builder.cells)
                {
                    emitter.CellPolygon(cell, polygon);
                    EmitPolygon(polygon, [](const Point&) { return 1.0; }, *hardTriangles);
                }
            }

            if (!loops)
                return;

            std::vector<BoundaryEdge> edges;
            for (const Piece& piece : pieces)
            {
                Point top = sweep.PointAt(piece.segment, piece.eventTop);
                Point bottom = sweep.PointAt(piece.segment, piece.eventBottom);
                edges.push_back(piece.down ? BoundaryEdge{ top, bottom, piece.segment } :
                                BoundaryEdge{ bottom, top, piece.segment });
            }

            // Horizontal boundary: parts of event line that are filled only above or only below
            std::vector<std::pair<double, int>> toggles;
            for (int event = 0; event < (int)sweep.events.size(); event++)
            {
                toggles.clear();
                if (event > 0)
                {
                    for (double x : bottomToggles[event - 1])
                        toggles.push_back({ x, 0 });
                }

                for (double x : topToggles[event])
                    toggles.push_back({ x, 1 });

                std::sort(toggles.begin(), toggles.end());

                bool sides[2] = { false, false };
                double prevX = 0;
                double y = sweep.events[event];
                for (const auto& toggle : toggles)
                {
                    if (sides[0] != sides[1] && toggle.first > prevX)
                    {
                        Point left(prevX, y), right(toggle.first, y);
                        edges.push_back(sides[1] ? BoundaryEdge{ left, right, -1 - event } :
                                        BoundaryEdge{ right, left, -1 - event });
                    }

                    sides[toggle.second] = !sides[toggle.second];
                    prevX = toggle.first;
                }
            }

            typedef std::pair<long long, long long> PointKey;
            auto keyOf = [](const Point& point)
            {
                return PointKey(llround(point.x*kGrid), llround(point.y*kGrid));
            };

            std::map<PointKey, std::vector<int>> starts;
            for (int i = (int)edges.size() - 1; i >= 0; i--)
                starts[keyOf(edges[i].from)].push_back(i);

            auto isNear = [&](const Point& a, const Point& b)
            {
                return fabs(a.x - b.x) + fabs(a.y - b.y) < minEdgeLength;
            };

            std::vector<bool> used(edges.size(), false);
            std::vector<int> chain;
            for (size_t first = 0; first < edges.size(); first++)
            {
                if (used[first])
                    continue;

                chain.clear();
                PointKey startKey = keyOf(edges[first].from);
                int current = (int)first;
                bool closed = false;
                while (true)
                {
                    used[current] = true;
                    chain.push_back(current);

                    PointKey endKey = keyOf(edges[current].to);
                    if (endKey.second == startKey.second && llabs(endKey.first - startKey.first) <= 1)
                    {
                        closed = true;
                        break;
                    }

                    // Segments crossing on an event line are snapped apart, their points can be a grid step away
                    int next = -1;
                    for (long long shift : { 0, -1, 1 })
                    {
                        auto found = starts.find(PointKey(endKey.first + shift, endKey.second));
                        if (found == starts.end())
                            continue;

                        std::vector<int>& candidates = found->second;
                        while (!candidates.empty() && used[candidates.back()])
                            candidates.pop_back();

                        if (!candidates.empty())
                        {
                            next = candidates.back();
                            candidates.pop_back();
                            break;
                        }
                    }

                    if (next < 0)
                        break;

                    current = next;
                }

                if (!closed)
                    continue;

                Polyline loop;
                for (size_t i = 0; i < chain.size(); i++)
                {
                    const BoundaryEdge& edge = edges[chain[i]];
                    const BoundaryEdge& prev = edges[chain[(i + chain.size() - 1)%chain.size()]];
                    if (edge.source == prev.source && chain.size() > 2)
                        continue;

                    if (!loop.empty() && isNear(loop.back(), edge.from))
                        continue;

                    loop.push_back(edge.from);
                }

                while (loop.size() > 1 && isNear(loop.back(), loop[0]))
                    loop.pop_back();

                if (loop.size() >= 3)
                    loops->push_back(loop);
            }
        }

        enum class BandKind { Edge, Patch };

        // Convex zone of the fringe: strip along a boundary edge or a row of cells around vertices
        struct Band
        {
            Point  polygon[4];
            double values[4] = { 0, 0, 0, 0 }; // Coverage at the polygon points of an edge strip
            int    segments[4] = { -1, -1, -1, -1 };
            int    count = 0;

            BandKind kind = BandKind::Edge;

            double Coverage(const Point& point) const
            {
                if (count == 4)
                {
                    Point diagonal = polygon[2] - polygon[0];
                    double side = Cross(diagonal, point - polygon[0]);
                    double first = Cross(diagonal, polygon[1] - polygon[0]);
                    double second = Cross(diagonal, polygon[3] - polygon[0]);
                    if (fabs(second) > 1e-12 && (fabs(first) <= 1e-12 || side*second > 0))
                        return PlaneValue(0, 2, 3, point);
                }

                return PlaneValue(0, 1, 2, point);
            }

        private:
            double PlaneValue(int a, int b, int c, const Point& point) const
            {
                Point sideB = polygon[b] - polygon[a], sideC = polygon[c] - polygon[a];
                double area = Cross(sideB, sideC);
                if (fabs(area) < 1e-12)
                    return values[a];

                Point offset = point - polygon[a];
                double coefB = Cross(offset, sideC)/area, coefC = Cross(sideB, offset)/area;
                return values[a] + (values[b] - values[a])*coefB + (values[c] - values[a])*coefC;
            }
        };

        typedef std::pair<Point, Point> Splitter;
        typedef std::pair<long long, long long> PointKey;
        typedef std::set<PointKey> SmoothPoints; // Vertices of flattened curves

        PointKey KeyOf(const Point& point)
        {
            return PointKey(llround(point.x*kGrid), llround(point.y*kGrid));
        }

        double Clamp01(double value)
        {
            return value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
        }

        // Area of the pixel square centered at a point that is inside of the loops
        class CoverageField
        {
        public:
            CoverageField(const std::vector<Polyline>& loops)
            {
                double minY = 1e30, maxY = -1e30;
                for (const Polyline& loop : loops)
                {
                    for (size_t i = 0; i < loop.size(); i++)
                    {
                        const Point& from = loop[i];
                        const Point& to = loop[(i + 1)%loop.size()];
                        if (from.y == to.y)
                            continue;

                        mEdges.push_back({ from, to, 0 });
                        minY = Math::Min(minY, Math::Min(from.y, to.y));
                        maxY = Math::Max(maxY, Math::Max(from.y, to.y));
                    }
                }

                if (mEdges.empty())
                    return;

                mOrigin = minY;
                mEnd = maxY;
                mRowSize = Math::Max(1.0, (maxY - minY)/2048.0);
                mRows.resize((size_t)floor((maxY - minY)/mRowSize) + 1);

                for (size_t i = 0; i < mEdges.size(); i++)
                {
                    Edge& edge = mEdges[i];
                    edge.firstRow = Row(Math::Min(edge.from.y, edge.to.y));
                    int lastRow = Row(Math::Max(edge.from.y, edge.to.y));
                    for (int row = edge.firstRow; row <= lastRow; row++)
                        mRows[row].push_back((int)i);
                }
            }

            double At(const Point& point)
            {
                PointKey key = KeyOf(point);
                auto found = mValues.find(key);
                if (found != mValues.end())
                    return found->second;

                double left = point.x - 0.5, right = point.x + 0.5, top = point.y - 0.5, bottom = point.y + 0.5;

                double sum = 0;
                if (!mEdges.empty() && bottom > mOrigin && top < mEnd)
                {
                    int firstRow = Row(top), lastRow = Row(bottom);
                    for (int row = firstRow; row <= lastRow; row++)
                    {
                        for (int idx : mRows[row])
                        {
                            const Edge& edge = mEdges[idx];
                            if (Math::Max(edge.firstRow, firstRow) == row)
                                sum += Integral(edge, left, right, top, bottom);
                        }
                    }
                }

                double value = Clamp01(fabs(sum));
                mValues[key] = value;
                return value;
            }

            bool IsInside(const Point& point) const
            {
                if (mEdges.empty() || point.y < mOrigin || point.y > mEnd)
                    return false;

                bool inside = false;
                for (int idx : mRows[Row(point.y)])
                {
                    const Edge& edge = mEdges[idx];
                    if ((edge.from.y <= point.y) == (edge.to.y <= point.y))
                        continue;

                    double coef = (point.y - edge.from.y)/(edge.to.y - edge.from.y);
                    if (edge.from.x + (edge.to.x - edge.from.x)*coef > point.x)
                        inside = !inside;
                }

                return inside;
            }

        private:
            struct Edge
            {
                Point from, to;
                int   firstRow;
            };

            struct KeyHash
            {
                size_t operator()(const PointKey& key) const
                {
                    return (size_t)((unsigned long long)key.first*0x9E3779B97F4A7C15ull ^
                                    (unsigned long long)key.second*0xC2B2AE3D27D4EB4Full);
                }
            };

            std::vector<Edge>             mEdges;
            std::vector<std::vector<int>> mRows;

            double mOrigin = 0, mEnd = 0, mRowSize = 1;

            std::unordered_map<PointKey, double, KeyHash> mValues;

            int Row(double y) const
            {
                return Math::Clamp((int)floor((y - mOrigin)/mRowSize), 0, Math::Max((int)mRows.size() - 1, 0));
            }

            // Integral of the clamped x by dy along the edge inside of the rows range: Green's formula of the area
            static double Integral(const Edge& edge, double left, double right, double top, double bottom)
            {
                double height = edge.to.y - edge.from.y;
                double coefTop = Clamp01((top - edge.from.y)/height);
                double coefBottom = Clamp01((bottom - edge.from.y)/height);
                double begin = Math::Min(coefTop, coefBottom), end = Math::Max(coefTop, coefBottom);
                if (end <= begin)
                    return 0;

                double width = edge.to.x - edge.from.x;
                double cuts[4] = { begin, end, end, end };
                int count = 2;
                if (width != 0)
                {
                    for (double x : { left, right })
                    {
                        double coef = (x - edge.from.x)/width;
                        if (coef > begin && coef < end)
                            cuts[count++] = coef;
                    }

                    std::sort(cuts, cuts + count);
                }

                auto value = [&](double coef)
                {
                    return Math::Clamp(edge.from.x + width*coef, left, right) - left;
                };

                double sum = 0;
                for (int i = 0; i + 1 < count; i++)
                    sum += (value(cuts[i]) + value(cuts[i + 1]))*0.5*(cuts[i + 1] - cuts[i]);

                return sum*height;
            }
        };

        typedef std::function<double(const Point&)> CoverageFunction;

        // Fields of a shape with an opaque stroke: of the stroke and of its area united with the fill
        struct StrokeCover
        {
            CoverageField* united;
            CoverageField* stroke;
        };

        double LatticeLineAfter(double value)
        {
            return ceil(value - 0.5 - kLatticeEpsilon) + 0.5;
        }

        double LatticeLineBefore(double value)
        {
            return floor(value - 0.5 + kLatticeEpsilon) + 0.5;
        }

        // Covers pixel squares around the vertices with cells between the pixel centers, where the coverage is exact
        void BuildPatches(const std::vector<Point>& vertices, const std::vector<std::pair<int, int>>& edges,
                          const std::vector<std::pair<Point, Point>>& areas, const CoverageFunction& coverageAt,
                          const CoverageFunction& shapeCoverageAt, std::vector<Band>& bands,
                          std::vector<Splitter>& splitters)
        {
            const double minCoverage = 1.0/1024.0;

            // Row and column of a cell are the coordinates of its top left pixel center, rounded down
            std::set<PointKey> cells;

            auto addCells = [&](const Point& min, const Point& max)
            {
                long long firstRow = (long long)floor(LatticeLineBefore(min.y));
                long long lastRow = (long long)floor(LatticeLineAfter(max.y)) - 1;
                long long firstColumn = (long long)floor(LatticeLineBefore(min.x));
                long long lastColumn = (long long)floor(LatticeLineAfter(max.x)) - 1;

                for (long long row = firstRow; row <= lastRow; row++)
                {
                    for (long long column = firstColumn; column <= lastColumn; column++)
                        cells.insert(PointKey(row, column));
                }
            };

            for (const Point& vertex : vertices)
                addCells(vertex - Point(0.5, 0.5), vertex + Point(0.5, 0.5));

            for (const auto& edge : edges)
            {
                const Point& from = vertices[edge.first];
                const Point& to = vertices[edge.second];
                addCells(Point(Math::Min(from.x, to.x) - 0.5, Math::Min(from.y, to.y) - 0.5),
                         Point(Math::Max(from.x, to.x) + 0.5, Math::Max(from.y, to.y) + 0.5));
            }

            for (const auto& area : areas)
                addCells(area.first, area.second);

            // Cell with the same full or empty coverage in all corners has no boundary in the pixels around it.
            // Under a stroke the coverage is not the one of the shape, that makes the solid faces: both are checked
            for (auto cell = cells.begin(); cell != cells.end();)
            {
                double top = cell->first + 0.5, left = cell->second + 0.5;
                double min = 1.0, max = 0.0;
                for (const Point& corner : { Point(left, top), Point(left + 1.0, top), Point(left, top + 1.0),
                                             Point(left + 1.0, top + 1.0) })
                {
                    for (double coverage : { coverageAt(corner), shapeCoverageAt(corner) })
                    {
                        min = Math::Min(min, coverage);
                        max = Math::Max(max, coverage);
                    }
                }

                if (max < minCoverage || min > 1.0 - minCoverage)
                    cell = cells.erase(cell);
                else
                    cell++;
            }

            for (auto cell = cells.begin(); cell != cells.end();)
            {
                long long row = cell->first, begin = cell->second, end = begin + 1;
                for (cell++; cell != cells.end() && cell->first == row && cell->second == end; cell++)
                    end++;

                double top = row + 0.5, bottom = row + 1.5;

                Band band;
                band.kind = BandKind::Patch;
                band.count = 4;
                band.polygon[0] = Point(begin + 0.5, top);
                band.polygon[1] = Point(end + 0.5, top);
                band.polygon[2] = Point(end + 0.5, bottom);
                band.polygon[3] = Point(begin + 0.5, bottom);
                bands.push_back(band);

                for (long long column = begin + 1; column < end; column++)
                    splitters.push_back(Splitter(Point(column + 0.5, top), Point(column + 0.5, bottom)));
            }
        }

        // Pixel coverage across a slanted edge is a curve, one linear ramp is up to 11 levels off it: 3 ramps are built
        struct FringeProfile
        {
            double distances[4] = { -0.5, -0.5, 0.5, 0.5 }; // Into the shape from the edge
            double coverages[4] = { 0.0, 0.0, 1.0, 1.0 };

            FringeProfile() = default;

            FringeProfile(const Point& normal)
            {
                const double minSlant = 0.2;

                double x = fabs(normal.x), y = fabs(normal.y);
                double slant = 2.0*x*y;
                if (slant < minSlant)
                    return;

                double major = Math::Max(x, y), minor = Math::Min(x, y);
                double reach = (major + minor)*0.5, core = (major - minor)*0.5;
                double inner = 0.5 - 0.23*slant, outer = 0.5 + 0.1*slant;
                double innerCoverage = 0.01*slant*slant +
                    (inner <= core ? 0.5 + inner/major : 1.0 - (reach - inner)*(reach - inner)/slant);

                distances[0] = -outer;
                distances[1] = -inner;
                distances[2] = inner;
                distances[3] = outer;
                coverages[1] = 1.0 - innerCoverage;
                coverages[2] = innerCoverage;
            }

            double Outer() const
            {
                return distances[3];
            }
        };

        bool IsWhole(double value)
        {
            return fabs(value - floor(value + 0.5)) < 0.5/kGrid;
        }

        bool IsCrossingRect(const Point& from, const Point& to, const Point& min, const Point& max)
        {
            double begin = 0.0, end = 1.0;
            Point delta = to - from;
            for (int axis = 0; axis < 2; axis++)
            {
                double origin = axis == 0 ? from.x : from.y, step = axis == 0 ? delta.x : delta.y;
                double low = axis == 0 ? min.x : min.y, high = axis == 0 ? max.x : max.y;
                if (step == 0)
                {
                    if (origin <= low || origin >= high)
                        return false;

                    continue;
                }

                double coefA = (low - origin)/step, coefB = (high - origin)/step;
                begin = Math::Max(begin, Math::Min(coefA, coefB));
                end = Math::Min(end, Math::Max(coefA, coefB));
            }

            return begin < end;
        }

        // Edges on the pixels boundaries are hard: pixel centers are covered exactly without a strip. The other edges
        // must be out of the pixels along a hard one, except its neighbors that meet it in the cells of the vertex
        void FindHardEdges(const std::vector<Polyline>& loops, std::vector<std::vector<char>>& hard)
        {
            const double reach = 1.0 - 1e-6;

            auto isAligned = [](const Point& from, const Point& to)
            {
                return (from.x == to.x && IsWhole(from.x)) || (from.y == to.y && IsWhole(from.y));
            };

            hard.resize(loops.size());
            for (size_t i = 0; i < loops.size(); i++)
            {
                const Polyline& loop = loops[i];
                hard[i].resize(loop.size());
                for (size_t k = 0; k < loop.size(); k++)
                    hard[i][k] = isAligned(loop[k], loop[(k + 1)%loop.size()]);
            }

            std::vector<std::vector<char>> aligned = hard;
            for (size_t i = 0; i < loops.size(); i++)
            {
                const Polyline& loop = loops[i];
                size_t count = loop.size();
                for (size_t k = 0; k < count; k++)
                {
                    if (!aligned[i][k])
                        continue;

                    size_t prev = (k + count - 1)%count, next = (k + 1)%count;
                    Point from = loop[k], to = loop[next];
                    Point direction = Normalized(to - from);

                    // Pixels around a vertex with a slanted edge are covered by its cells
                    from = from + direction*(aligned[i][prev] ? -reach : 0.5);
                    to = to + direction*(aligned[i][next] ? reach : -0.5);
                    if (Dot(to - from, direction) <= 0)
                        continue;

                    Point min(Math::Min(from.x, to.x), Math::Min(from.y, to.y));
                    Point max(Math::Max(from.x, to.x), Math::Max(from.y, to.y));
                    (direction.x == 0 ? min.x : min.y) -= reach;
                    (direction.x == 0 ? max.x : max.y) += reach;

                    for (size_t j = 0; j < loops.size() && hard[i][k]; j++)
                    {
                        const Polyline& other = loops[j];
                        for (size_t n = 0; n < other.size() && hard[i][k]; n++)
                        {
                            if (aligned[j][n] || (i == j && (n == prev || n == next)))
                                continue;

                            hard[i][k] = !IsCrossingRect(other[n], other[(n + 1)%other.size()], min, max);
                        }
                    }
                }
            }
        }

        // Builds strips along the edges, mitered at smooth corners, and patches around the others; false with patches.
        // Only cells are built along all the edges when cellsOnly
        bool BuildBands(const std::vector<Polyline>& loops, const SmoothPoints& smoothPoints, bool hardAligned,
                        bool cellsOnly, const CoverageFunction& coverageAt, const CoverageFunction& shapeCoverageAt,
                        std::vector<Band>& bands, std::vector<Splitter>& splitters)
        {
            struct Corner
            {
                Point  in[4], out[4]; // Fringe points of the edges coming into the vertex and going out of it
                double inCoverages[4], outCoverages[4];
            };

            std::vector<Point> directions, normals, patchVertices;
            std::vector<std::pair<int, int>> patchEdges;
            std::vector<int> patchIndexes;
            std::vector<double> lengths, trims, reaches;
            std::vector<char> sharp, plain;
            std::vector<Corner> corners;
            std::vector<FringeProfile> edgeProfiles;
            std::vector<std::pair<Point, Point>> patchAreas;

            std::vector<std::vector<char>> hardEdges;
            if (hardAligned)
                FindHardEdges(loops, hardEdges);

            for (size_t loopIdx = 0; loopIdx < loops.size(); loopIdx++)
            {
                const Polyline& loop = loops[loopIdx];
                size_t count = loop.size();

                std::vector<char> hard = hardAligned ? hardEdges[loopIdx] : std::vector<char>(count, 0);

                directions.resize(count);
                normals.resize(count);
                lengths.resize(count);
                trims.resize(count);
                reaches.resize(count);
                sharp.resize(count);
                plain.resize(count);
                corners.resize(count);
                edgeProfiles.resize(count);
                patchIndexes.assign(count, -1);

                // Length from the vertex where the strip of the edge gets out of the pixel square around it
                auto getTrim = [&](size_t i)
                {
                    double x = fabs(directions[i].x), y = fabs(directions[i].y), outer = edgeProfiles[i].Outer();
                    double trim = Math::Min(x > 1e-9 ? (0.5 - outer*y)/x : 0.5, y > 1e-9 ? (0.5 - outer*x)/y : 0.5);
                    return Math::Max(trim, 0.0);
                };

                for (size_t i = 0; i < count; i++)
                {
                    const Point& from = loop[i];
                    const Point& to = loop[(i + 1)%count];
                    Point delta = to - from;
                    lengths[i] = Length(delta);
                    directions[i] = delta*(1.0/Math::Max(lengths[i], 1e-12));
                    normals[i] = Point(directions[i].y, -directions[i].x);
                    edgeProfiles[i] = FringeProfile();
                    trims[i] = getTrim(i);
                }

                auto isSlanted = [&](size_t edge)
                {
                    return FringeProfile(normals[edge]).Outer() > 0.5;
                };

                for (size_t i = 0; i < count; i++)
                {
                    size_t prev = (i + count - 1)%count;
                    double cosine = Dot(directions[prev], directions[i]);
                    double sine = Cross(directions[prev], directions[i]);

                    // Coverage of tight curves and of slanted edges is not one ramp across the edge: cells and 3 ramps
                    double turn = fabs(atan2(sine, cosine));
                    bool tight = Math::Min(lengths[prev], lengths[i]) < kMinSmoothRadius*turn;
                    bool slanted = isSlanted(prev) || isSlanted(i);

                    plain[i] = hard[prev] && hard[i];
                    sharp[i] = !plain[i] &&
                        (cosine < (smoothPoints.count(KeyOf(loop[i])) ? kSmoothCurveCos : kSmoothCornerCos) ||
                         hard[prev] || hard[i] || tight || slanted || cellsOnly);

                    reaches[i] = sharp[i] || plain[i] ? 0.0 : 0.5*fabs(sine)/(1.0 + cosine);
                }

                for (bool changed = true; changed;)
                {
                    changed = false;
                    for (size_t i = 0; i < count; i++)
                    {
                        size_t next = (i + 1)%count;
                        if (hard[i] || (sharp[i] && sharp[next]))
                            continue;

                        double taken = (sharp[i] ? trims[i] : reaches[i]) + (sharp[next] ? trims[i] : reaches[next]);
                        if (taken <= lengths[i]*kMiterRoom)
                            continue;

                        sharp[i] = sharp[next] = true;
                        changed = true;
                    }
                }

                // Slanted edge between sharp vertices has the strips of its own profile, the others have one ramp
                for (size_t i = 0; i < count; i++)
                {
                    if (!sharp[i] || !sharp[(i + 1)%count] || hard[i])
                        continue;

                    edgeProfiles[i] = FringeProfile(normals[i]);
                    trims[i] = getTrim(i);
                }

                for (size_t i = 0; i < count; i++)
                {
                    size_t prev = (i + count - 1)%count;
                    const Point& vertex = loop[i];
                    Corner& corner = corners[i];

                    if (plain[i])
                        continue;

                    if (sharp[i])
                    {
                        patchIndexes[i] = (int)patchVertices.size();
                        patchVertices.push_back(vertex);

                        Point end = vertex - directions[prev]*trims[prev], begin = vertex + directions[i]*trims[i];
                        for (int k = 0; k < 4; k++)
                        {
                            corner.in[k] = Snap(end - normals[prev]*edgeProfiles[prev].distances[k]);
                            corner.out[k] = Snap(begin - normals[i]*edgeProfiles[i].distances[k]);
                            corner.inCoverages[k] = edgeProfiles[prev].coverages[k];
                            corner.outCoverages[k] = edgeProfiles[i].coverages[k];
                        }

                        continue;
                    }

                    Point miter = (normals[prev] + normals[i])*(1.0/(1.0 + Dot(directions[prev], directions[i])));
                    for (int k = 0; k < 4; k++)
                    {
                        corner.in[k] = corner.out[k] = Snap(vertex - miter*edgeProfiles[i].distances[k]);
                        corner.inCoverages[k] = corner.outCoverages[k] = edgeProfiles[i].coverages[k];
                    }
                }

                // The strip of an edge at a sharp angle to a hard one goes along it, that part is covered by cells.
                // Returns the length of the hard edge that is taken by them
                auto coverNeighbor = [&](const Point& vertex, const Point& hardDirection, size_t edge, double sign)
                {
                    Point direction = directions[edge]*sign;
                    double along = Dot(direction, hardDirection), across = fabs(Cross(direction, hardDirection));
                    size_t far = sign > 0 ? (edge + 1)%count : edge;
                    if (hard[edge] || along <= 0 || across < 1e-9 || (sharp[far] && lengths[edge] <= kMaxPatchEdge))
                        return 0.5;

                    double outer = edgeProfiles[edge].Outer();
                    double length = Math::Min(lengths[edge], (0.5 + outer*along)/across);
                    Point end = vertex + direction*length;
                    patchAreas.push_back(std::make_pair(
                        Point(Math::Min(vertex.x, end.x) - outer, Math::Min(vertex.y, end.y) - outer),
                        Point(Math::Max(vertex.x, end.x) + outer, Math::Max(vertex.y, end.y) + outer)));

                    return Math::Max(length*along + outer, 0.5);
                };

                for (size_t i = 0; i < count; i++)
                {
                    size_t next = (i + 1)%count;
                    if (hard[i])
                    {
                        // The splitter is the side of the solid faces, it starts out of the cells around the vertices
                        Point from = loop[i], to = loop[next];
                        bool vertical = from.x == to.x;
                        double& begin = vertical ? from.y : from.x;
                        double& end = vertical ? to.y : to.x;
                        bool forward = begin < end;

                        if (sharp[i])
                        {
                            double taken = coverNeighbor(loop[i], directions[i], (i + count - 1)%count, -1.0);
                            begin = forward ? LatticeLineAfter(begin + taken) : LatticeLineBefore(begin - taken);
                        }

                        if (sharp[next])
                        {
                            double taken = coverNeighbor(loop[next], directions[i]*-1.0, next, 1.0);
                            end = forward ? LatticeLineBefore(end - taken) : LatticeLineAfter(end + taken);
                        }

                        if (forward ? begin < end : begin > end)
                            splitters.push_back(Splitter(from, to));

                        continue;
                    }

                    // Strip of a short edge between patches is covered by their cells
                    if (sharp[i] && sharp[next] && lengths[i] <= kMaxPatchEdge)
                    {
                        patchEdges.push_back(std::make_pair(patchIndexes[i], patchIndexes[next]));
                        continue;
                    }

                    if (cellsOnly)
                    {
                        int pieces = (int)ceil(lengths[i]);
                        for (int k = 0; k < pieces; k++)
                        {
                            Point from = loop[i] + directions[i]*(lengths[i]*k/pieces);
                            Point to = loop[i] + directions[i]*(lengths[i]*(k + 1)/pieces);
                            patchAreas.push_back(std::make_pair(
                                Point(Math::Min(from.x, to.x) - 0.5, Math::Min(from.y, to.y) - 0.5),
                                Point(Math::Max(from.x, to.x) + 0.5, Math::Max(from.y, to.y) + 0.5)));
                        }

                        continue;
                    }

                    const Corner& from = corners[i];
                    const Corner& to = corners[next];
                    if (Dot(to.in[0] - from.out[0], directions[i]) < kMinBandLength ||
                        Dot(to.in[3] - from.out[3], directions[i]) < kMinBandLength)
                    {
                        continue;
                    }

                    for (int k = 0; k < 3; k++)
                    {
                        Band band;
                        band.kind = BandKind::Edge;

                        auto add = [&](const Point& point, double coverage)
                        {
                            if (band.count > 0 && band.polygon[band.count - 1] == point)
                                return;

                            band.polygon[band.count] = point;
                            band.values[band.count] = coverage;
                            band.count++;
                        };

                        add(from.out[k], from.outCoverages[k]);
                        add(to.in[k], to.inCoverages[k]);
                        add(to.in[k + 1], to.inCoverages[k + 1]);
                        add(from.out[k + 1], from.outCoverages[k + 1]);

                        if (band.count > 1 && band.polygon[band.count - 1] == band.polygon[0])
                            band.count--;

                        if (band.count >= 3)
                            bands.push_back(band);
                    }
                }
            }

            BuildPatches(patchVertices, patchEdges, patchAreas, coverageAt, shapeCoverageAt, bands, splitters);
            return patchVertices.empty();
        }

        void EmitFan(std::vector<Point>& polygon, const CoverageVertex& center, const CoverageFunction& coverageAt,
                     TriangleSoup& soup)
        {
            size_t count = 0;
            for (size_t i = 0; i < polygon.size(); i++)
            {
                if (count == 0 || !(polygon[i] == polygon[count - 1]))
                    polygon[count++] = polygon[i];
            }

            while (count > 1 && polygon[count - 1] == polygon[0])
                count--;

            polygon.resize(count);
            if (count < 3)
                return;

            double area = 0;
            for (size_t i = 0; i < count; i++)
                area += Cross(polygon[i], polygon[(i + 1)%count]);

            if (area < 0)
                std::reverse(polygon.begin(), polygon.end());

            for (size_t i = 0; i < count; i++)
            {
                const Point& from = polygon[i];
                const Point& to = polygon[(i + 1)%count];
                if (fabs(Cross(from - Point(center.x, center.y), to - Point(center.x, center.y))) < 1e-12)
                    continue;

                soup.push_back(center);
                soup.push_back({ from.x, from.y, (float)coverageAt(from), 0.0f });
                soup.push_back({ to.x, to.y, (float)coverageAt(to), 0.0f });
            }
        }

        // Emits bands as is when they do not overlap, otherwise their arrangement with exact coverage under overlaps
        void BuildFringedCoverage(const std::vector<Polyline>& loops, const SmoothPoints& smoothPoints,
                                  bool hardAligned, const StrokeCover* strokeCover, TriangleSoup& soup)
        {
            const double minCoverage = 1.0/512.0;
            const int splitterTag = -1;
            const int solidState = -1;

            CoverageField field(loops);

            // Fill blended under the stroke of the shape gives the coverage of their united area with this one
            CoverageFunction coverageAt = [&](const Point& point)
            {
                if (!strokeCover)
                    return field.At(point);

                double stroke = strokeCover->stroke->At(point);
                if (stroke > 1.0 - 1e-6)
                    return field.At(point);

                return Clamp01((strokeCover->united->At(point) - stroke)/(1.0 - stroke));
            };

            std::vector<Band> bands;
            std::vector<Splitter> splitters;
            CoverageFunction shapeCoverageAt = [&](const Point& point) { return field.At(point); };

            bool plainFringe = BuildBands(loops, smoothPoints, hardAligned && !strokeCover, strokeCover != nullptr,
                                          coverageAt, shapeCoverageAt, bands, splitters);

            Sweep sweep;
            for (size_t i = 0; i < bands.size(); i++)
            {
                Band& band = bands[i];
                for (int k = 0; k < band.count; k++)
                    band.segments[k] = sweep.Add(band.polygon[k], band.polygon[(k + 1)%band.count], (int)i);
            }

            for (const Splitter& splitter : splitters)
                sweep.Add(splitter.first, splitter.second, splitterTag);

            if (sweep.segments.empty())
                return;

            sweep.Prepare();

            // State of a face: sorted indexes of bands over it
            std::map<std::vector<int>, int> stateIds;
            std::vector<std::vector<int>> states;
            std::vector<int> activeBands, state;
            CellBuilder builder;

            sweep.Run([&](int event, const std::vector<StripEntry>& entries)
            {
                activeBands.clear();

                // Faces under patches are their cells, not cut by the sides of the other bands
                int patches = 0;
                size_t leftIdx = entries.size();

                for (size_t i = 0; i < entries.size(); i++)
                {
                    int tag = sweep.segments[entries[i].segment].tag;
                    bool isPatch = tag != splitterTag && bands[tag].kind == BandKind::Patch;
                    bool isSide = tag == splitterTag || isPatch || patches == 0;

                    const StripEntry& right = entries[i];
                    if (isSide && leftIdx < entries.size() &&
                        (entries[leftIdx].xTop < right.xTop || entries[leftIdx].xBottom < right.xBottom))
                    {
                        const StripEntry& left = entries[leftIdx];

                        state.clear();
                        for (int idx : activeBands)
                        {
                            if (patches == 0 || bands[idx].kind == BandKind::Patch)
                                state.push_back(idx);
                        }

                        std::sort(state.begin(), state.end());

                        // Filled faces without bands are not merged with the empty ones over the hard edges
                        if (state.empty() &&
                            field.IsInside(Point((left.xTop + left.xBottom + right.xTop + right.xBottom)*0.25,
                                                 (sweep.events[event] + sweep.events[event + 1])*0.5)))
                        {
                            state.push_back(solidState);
                        }

                        auto found = stateIds.find(state);
                        if (found == stateIds.end())
                        {
                            found = stateIds.insert(std::make_pair(state, (int)states.size())).first;
                            states.push_back(state);
                        }

                        builder.Add(event, left, right, found->second);

                        double width = Math::Max(right.xTop - left.xTop, right.xBottom - left.xBottom);
                        if (state.size() > 1 && width > kSliverWidth)
                            plainFringe = false;
                    }

                    if (tag != splitterTag)
                    {
                        auto found = std::find(activeBands.begin(), activeBands.end(), tag);
                        if (found != activeBands.end())
                        {
                            activeBands.erase(found);
                            patches -= isPatch ? 1 : 0;
                        }
                        else
                        {
                            activeBands.push_back(tag);
                            patches += isPatch ? 1 : 0;
                        }
                    }

                    if (isSide)
                        leftIdx = i;
                }
            });

            auto cellCenter = [&](const Cell& cell)
            {
                Point sum = sweep.PointAt(cell.left, cell.eventTop) + sweep.PointAt(cell.right, cell.eventTop) +
                    sweep.PointAt(cell.left, cell.eventBottom) + sweep.PointAt(cell.right, cell.eventBottom);
                return sum*0.25;
            };

            auto cellWidth = [&](const Cell& cell)
            {
                return Math::Max(sweep.XAt(cell.right, cell.eventTop) - sweep.XAt(cell.left, cell.eventTop),
                                 sweep.XAt(cell.right, cell.eventBottom) - sweep.XAt(cell.left, cell.eventBottom));
            };

            CellEmitter emitter(sweep);
            std::vector<Point> polygon;

            auto isSolid = [&](const Cell& cell)
            {
                return states[cell.state].size() == 1 && states[cell.state][0] == solidState;
            };

            if (plainFringe)
            {
                for (const Cell& cell : builder.cells)
                {
                    if (isSolid(cell))
                        emitter.Register(cell);
                }

                emitter.Finish();

                for (const Cell& cell : builder.cells)
                {
                    if (!isSolid(cell))
                        continue;

                    emitter.CellPolygon(cell, polygon);
                    EmitPolygon(polygon, [](const Point&) { return 1.0; }, soup);
                }

                for (const Band& band : bands)
                {
                    polygon.clear();
                    for (int k = 0; k < band.count; k++)
                    {
                        const Point& from = band.polygon[k];
                        const Point& to = band.polygon[(k + 1)%band.count];
                        polygon.push_back(from);

                        int idx = band.segments[k];
                        if (idx >= 0)
                        {
                            const SweepSegment& segment = sweep.segments[idx];
                            bool down = from.y < to.y;
                            emitter.SegmentPoints(idx, down ? segment.startEvent : segment.endEvent,
                                                  down ? segment.endEvent : segment.startEvent, polygon);
                        }
                        else if (!(from == to))
                            emitter.LinePoints(sweep.EventIndex(from.y), from.x, to.x, polygon);
                    }

                    EmitPolygon(polygon, [&](const Point& point) { return Clamp01(band.Coverage(point)); }, soup);
                }

                return;
            }

            enum class CellKind { Skipped, Solid, Plane, Exact };

            // Strip of an edge is not its coverage where the pixel square reaches the other edges; a long strip
            // stays a plane, exact corners would spread the difference at one of them along it
            auto isPlane = [&](const Cell& cell, const Point* corners)
            {
                double height = sweep.events[cell.eventBottom] - sweep.events[cell.eventTop];
                if (Math::Max(height, Math::Max(corners[1].x, corners[3].x) - Math::Min(corners[0].x, corners[2].x)) >
                    kMaxExactCell)
                {
                    return true;
                }

                const Band& band = bands[states[cell.state][0]];
                for (int k = 0; k < 4; k++)
                {
                    if (fabs(Clamp01(band.Coverage(corners[k])) - field.At(corners[k])) > kMaxPlaneError)
                        return false;
                }

                return true;
            };

            std::vector<CellKind> kinds(builder.cells.size(), CellKind::Skipped);
            for (size_t i = 0; i < builder.cells.size(); i++)
            {
                const Cell& cell = builder.cells[i];
                const std::vector<int>& cellState = states[cell.state];
                Point corners[4] = { sweep.PointAt(cell.left, cell.eventTop), sweep.PointAt(cell.right, cell.eventTop),
                                     sweep.PointAt(cell.left, cell.eventBottom),
                                     sweep.PointAt(cell.right, cell.eventBottom) };

                if (cellState.empty())
                    continue;

                if (isSolid(cell))
                    kinds[i] = CellKind::Solid;
                else if (cellState.size() == 1 && bands[cellState[0]].kind == BandKind::Edge && isPlane(cell, corners))
                {
                    for (const Point& corner : corners)
                    {
                        if (bands[cellState[0]].Coverage(corner) > minCoverage)
                            kinds[i] = CellKind::Plane;
                    }
                }
                else
                {
                    bool visible = coverageAt(cellCenter(cell)) > minCoverage;
                    for (const Point& corner : corners)
                        visible = visible || coverageAt(corner) > minCoverage;

                    if (visible)
                        kinds[i] = CellKind::Exact;
                }

                if (kinds[i] != CellKind::Skipped)
                    emitter.Register(cell);
            }

            emitter.Finish();

            for (size_t i = 0; i < builder.cells.size(); i++)
            {
                if (kinds[i] == CellKind::Skipped)
                    continue;

                const Cell& cell = builder.cells[i];
                emitter.CellPolygon(cell, polygon);

                if (kinds[i] == CellKind::Solid)
                    EmitPolygon(polygon, [](const Point&) { return 1.0; }, soup);
                else if (kinds[i] == CellKind::Plane)
                {
                    // Corners of the cells are pixel centers with the exact coverage, the strip joins them with it
                    const Band& band = bands[states[cell.state][0]];
                    EmitPolygon(polygon, [&](const Point& point)
                    {
                        bool isPixelCenter = IsWhole(point.x - 0.5) && IsWhole(point.y - 0.5);
                        return isPixelCenter ? coverageAt(point) : Clamp01(band.Coverage(point));
                    }, soup);
                }
                else
                {
                    // Vertex in the center is needed where the coverage is not linear over the face
                    Point center = cellCenter(cell);
                    double leftTop = coverageAt(sweep.PointAt(cell.left, cell.eventTop));
                    double rightTop = coverageAt(sweep.PointAt(cell.right, cell.eventTop));
                    double leftBottom = coverageAt(sweep.PointAt(cell.left, cell.eventBottom));
                    double rightBottom = coverageAt(sweep.PointAt(cell.right, cell.eventBottom));
                    double average = (leftTop + rightTop + leftBottom + rightBottom)*0.25;
                    double twist = (leftTop + rightBottom - rightTop - leftBottom)*0.25;

                    double height = sweep.events[cell.eventBottom] - sweep.events[cell.eventTop];
                    const std::vector<int>& cellState = states[cell.state];
                    bool isPatch = !cellState.empty() && bands[cellState[0]].kind == BandKind::Patch;

                    if (isPatch || Math::Min(height, cellWidth(cell)) < kMinFanSize ||
                        (fabs(coverageAt(center) - average) < kMaxFanError && fabs(twist) < kMaxFanError))
                    {
                        EmitPolygon(polygon, coverageAt, soup);
                    }
                    else
                        EmitFan(polygon, { center.x, center.y, (float)coverageAt(center), 0.0f }, coverageAt, soup);
                }
            }
        }

        // Tight curves are in the cells of pixel centers, count of their chords costs no triangles: finer tolerance
        double GetChordTolerance(double tolerance, double bend, double length)
        {
            double chord = length/Math::Max(sqrt(0.75*bend/tolerance), 1.0);
            return chord*chord < 8.0*tolerance*kMinSmoothRadius ? tolerance*kFineTolerance : tolerance;
        }

        // Appends points of arc strictly between the angles
        void AddArc(Polyline& points, const Point& center, double radius, double fromAngle, double toAngle,
                    double tolerance, SmoothPoints& smoothPoints)
        {
            if (radius < kMinSmoothRadius)
                tolerance *= kFineTolerance;

            double step = radius > tolerance ? 2.0*acos(1.0 - tolerance/radius) : kPi*0.5;
            int count = Math::Clamp((int)ceil(fabs(toAngle - fromAngle)/Math::Max(step, 0.02)), 1, 256);
            for (int i = 0; i <= count; i++)
            {
                double angle = fromAngle + (toAngle - fromAngle)*i/count;
                Point point = Snap(center + Point(cos(angle), sin(angle))*radius);
                smoothPoints.insert(KeyOf(point));

                if (i > 0 && i < count)
                    points.push_back(point);
            }
        }

        struct PathVertex
        {
            Point position;
            Point tangentIn, tangentOut; // Path directions at the joint, zero when unknown
            bool  joint = false;         // End of a path segment, not a point inside of a flattened curve
        };

        typedef std::vector<PathVertex> PathPolyline;

        bool IsZero(const Point& point)
        {
            return point.x == 0 && point.y == 0;
        }

        bool IsCorner(const PathVertex& vertex)
        {
            return vertex.joint && (IsZero(vertex.tangentIn) || IsZero(vertex.tangentOut) ||
                                    Dot(vertex.tangentIn, vertex.tangentOut) < kSmoothJointCos);
        }

        struct Flattener
        {
            double tolerance = 0.05;
            double minChordCos = -1.0; // Cosine of the maximum angle between a chord and the curve at its ends

            PathPolyline& points;

            Flattener(PathPolyline& points):
                points(points)
            {}

            void Add(const Point& position)
            {
                Point snapped = Snap(position);
                if (!points.empty() && points.back().position == snapped)
                    return;

                PathVertex vertex;
                vertex.position = snapped;
                points.push_back(vertex);
            }

            static Point StartTangent(const Point& p0, const Point& p1, const Point& p2, const Point& p3)
            {
                Point res = Normalized(p1 - p0);
                if (IsZero(res))
                    res = Normalized(p2 - p0);

                return IsZero(res) ? Normalized(p3 - p0) : res;
            }

            struct Curve
            {
                Point p0, p1, p2, p3;

                Point At(double t) const
                {
                    double it = 1.0 - t;
                    return p0*(it*it*it) + p1*(3.0*it*it*t) + p2*(3.0*it*t*t) + p3*(t*t*t);
                }

                Point Tangent(double t) const
                {
                    double it = 1.0 - t;
                    return Normalized((p1 - p0)*(it*it) + (p2 - p1)*(2.0*it*t) + (p3 - p2)*(t*t));
                }
            };

            // Splits the piece of curve while its chord is turned from the curve at the ends more than allowed
            void Piece(const Curve& curve, double from, double to, int depth)
            {
                if (minChordCos > -1.0 && depth < kMaxFlattenDepth)
                {
                    Point chord = Normalized(curve.At(to) - curve.At(from));
                    Point tangentFrom = curve.Tangent(from), tangentTo = curve.Tangent(to);
                    if (!IsZero(chord) && ((!IsZero(tangentFrom) && Dot(tangentFrom, chord) < minChordCos) ||
                                           (!IsZero(tangentTo) && Dot(tangentTo, chord) < minChordCos)))
                    {
                        Piece(curve, from, (from + to)*0.5, depth + 1);
                        Piece(curve, (from + to)*0.5, to, depth + 1);
                        return;
                    }
                }

                Add(curve.At(to));
            }

            void Cubic(const Point& p0, const Point& p1, const Point& p2, const Point& p3)
            {
                const int maxSteps = 256;

                Curve curve = { p0, p1, p2, p3 };
                double bend = Math::Max(Length(p0 - p1*2.0 + p2), Length(p1 - p2*2.0 + p3));
                double length = (Length(p1 - p0) + Length(p2 - p1) + Length(p3 - p2) + Length(p3 - p0))*0.5;
                double chordTolerance = GetChordTolerance(tolerance, bend, length);
                int steps = Math::Clamp((int)ceil(sqrt(0.75*bend/chordTolerance)), 1, maxSteps);
                for (int i = 0; i < steps; i++)
                    Piece(curve, (double)i/steps, i + 1 == steps ? 1.0 : (double)(i + 1)/steps, 0);
            }

            // Returns false when path has not finite or too large coordinates
            bool Path(const VectorSubPath& path, const Point& scale)
            {
                const double maxCoordinate = 1e6;

                bool valid = true;
                auto toPixels = [&](const Vec2F& point)
                {
                    Point res((double)point.x*scale.x, (double)point.y*scale.y);
                    valid = valid && std::isfinite(res.x) && std::isfinite(res.y) && fabs(res.x) <= maxCoordinate &&
                        fabs(res.y) <= maxCoordinate;
                    return res;
                };

                auto setJoint = [&](const Point& tangentIn, const Point& tangentOut)
                {
                    PathVertex& vertex = points.back();
                    vertex.joint = true;
                    if (!IsZero(tangentIn))
                        vertex.tangentIn = tangentIn;

                    if (!IsZero(tangentOut))
                        vertex.tangentOut = tangentOut;
                };

                Point current = toPixels(path.start);
                if (!valid)
                    return false;

                Add(current);
                setJoint(Point(), Point());

                for (const VectorSegment& segment : path.segments)
                {
                    Point end = toPixels(segment.end);
                    Point control1 = segment.cubic ? toPixels(segment.control1) : current;
                    Point control2 = segment.cubic ? toPixels(segment.control2) : end;
                    if (!valid)
                        return false;

                    setJoint(Point(), StartTangent(current, control1, control2, end));

                    if (segment.cubic)
                        Cubic(current, control1, control2, end);
                    else
                        Add(end);

                    setJoint(StartTangent(end, control2, control1, current)*-1.0, Point());
                    current = end;
                }

                if (path.closed && points.size() > 1)
                {
                    PathVertex& first = points[0];
                    PathVertex& last = points.back();
                    if (last.position == first.position)
                    {
                        first.tangentIn = last.tangentIn;
                        points.pop_back();
                    }
                    else
                        first.tangentIn = last.tangentOut = Normalized(first.position - last.position);
                }

                return true;
            }
        };

        // Outline polygons of the stroke are sums of clockwise pieces, so their non zero fill is the stroke
        struct Stroker
        {
            double         halfWidth = 0.5;
            VectorLineCap  cap = VectorLineCap::Butt;
            VectorLineJoin join = VectorLineJoin::Miter;
            double         miterLimit = 4.0;
            double         tolerance = 0.15;

            std::vector<Polyline>& polygons;
            SmoothPoints&          smoothPoints;

            Stroker(std::vector<Polyline>& polygons, SmoothPoints& smoothPoints):
                polygons(polygons), smoothPoints(smoothPoints)
            {}

            Point Offset(const Point& vertex, const Point& direction, double sign) const
            {
                return Snap(vertex + Point(direction.y, -direction.x)*(sign*halfWidth));
            }

            void AddJoin(Polyline& side, const Point& vertex, const Point& directionA, const Point& directionB,
                         double sign, VectorLineJoin join, double miterLimit) const
            {
                Point from = Offset(vertex, directionA, sign), to = Offset(vertex, directionB, sign);
                double sine = Cross(directionA, directionB), cosine = Dot(directionA, directionB);
                bool straight = fabs(sine) < 1e-9;

                if (straight && cosine > 0)
                {
                    side.push_back(from);
                    return;
                }

                if (!straight && sine*sign < 0)
                {
                    side.push_back(from);
                    side.push_back(vertex);
                    side.push_back(to);
                    return;
                }

                if (join == VectorLineJoin::Miter && 1.0 + cosine > 1e-9)
                {
                    Point normalA(directionA.y, -directionA.x), normalB(directionB.y, -directionB.x);
                    Point miter = (normalA + normalB)*(1.0/(1.0 + cosine));
                    if (Length(miter) <= miterLimit)
                    {
                        side.push_back(Snap(vertex + miter*(sign*halfWidth)));
                        return;
                    }
                }

                side.push_back(from);

                if (join == VectorLineJoin::Round)
                {
                    Point normalA = Point(directionA.y, -directionA.x)*sign;
                    Point normalB = Point(directionB.y, -directionB.x)*sign;
                    double fromAngle = atan2(normalA.y, normalA.x);
                    double delta = atan2(normalB.y, normalB.x) - fromAngle;
                    if (straight)
                        delta = Cross(normalA, directionA) > 0 ? kPi : -kPi;
                    else if (delta > kPi)
                        delta -= 2.0*kPi;
                    else if (delta < -kPi)
                        delta += 2.0*kPi;

                    AddArc(side, vertex, halfWidth, fromAngle, fromAngle + delta, tolerance, smoothPoints);
                }

                side.push_back(to);
            }

            // Chord of a curve is turned from the curve end, on the inner side of that turn its piece gets own corner
            void AddChordCorner(Polyline& side, const Point& vertex, const Point& chord, const Point& tangent,
                                double sign, bool end) const
            {
                double sine = end ? Cross(chord, tangent) : Cross(tangent, chord);
                if (fabs(sine) < kMinChordTurn || sine*sign >= 0)
                    return;

                if (!end)
                    side.push_back(vertex);

                side.push_back(Offset(vertex, chord, sign));

                if (end)
                    side.push_back(vertex);
            }

            void AddCap(Polyline& polygon, const Point& point, const Point& direction) const
            {
                Point normal(direction.y, -direction.x);
                if (cap == VectorLineCap::Square)
                {
                    polygon.push_back(Snap(point + (normal + direction)*halfWidth));
                    polygon.push_back(Snap(point + (direction - normal)*halfWidth));
                }
                else if (cap == VectorLineCap::Round)
                {
                    double angle = atan2(normal.y, normal.x);
                    AddArc(polygon, point, halfWidth, angle, angle + kPi, tolerance, smoothPoints);
                }
            }

            void AddDot(const Point& point)
            {
                Polyline polygon;
                if (cap == VectorLineCap::Round)
                {
                    polygon.push_back(Snap(point + Point(halfWidth, 0)));
                    AddArc(polygon, point, halfWidth, 0, 2.0*kPi, tolerance, smoothPoints);
                }
                else if (cap == VectorLineCap::Square)
                {
                    polygon.push_back(Snap(point + Point(-halfWidth, -halfWidth)));
                    polygon.push_back(Snap(point + Point(halfWidth, -halfWidth)));
                    polygon.push_back(Snap(point + Point(halfWidth, halfWidth)));
                    polygon.push_back(Snap(point + Point(-halfWidth, halfWidth)));
                }

                if (polygon.size() >= 3)
                    polygons.push_back(polygon);
            }

            void AddPiece(Polyline& polygon)
            {
                double area = 0;
                for (size_t i = 0; i < polygon.size(); i++)
                    area += Cross(polygon[i], polygon[(i + 1)%polygon.size()]);

                if (area < 0)
                    std::reverse(polygon.begin(), polygon.end());

                if (polygon.size() >= 3)
                    polygons.push_back(polygon);
            }

            // Piece of the last chord of a curve, cut by the line across the curve end, with the cap on that line
            void AddCurveEnd(const Point& end, const Point& neighbor, const Point& tangent)
            {
                Point chord = Normalized(end - neighbor);
                Point normal = Point(chord.y, -chord.x)*halfWidth;
                double overrun = halfWidth*fabs(Cross(chord, tangent))/Math::Max(Dot(chord, tangent), 0.1);
                Point far = end + chord*(overrun + kCurveEndStub);

                Point corners[4] = { neighbor + normal, far + normal, far - normal, neighbor - normal };
                Polyline piece;
                for (int i = 0; i < 4; i++)
                {
                    const Point& current = corners[i];
                    const Point& next = corners[(i + 1)%4];
                    double currentSide = Dot(current - end, tangent), nextSide = Dot(next - end, tangent);

                    if (currentSide <= 0)
                        piece.push_back(Snap(current));

                    if ((currentSide < 0 && nextSide > 0) || (currentSide > 0 && nextSide < 0))
                        piece.push_back(Snap(current + (next - current)*(currentSide/(currentSide - nextSide))));
                }

                AddPiece(piece);

                if (cap == VectorLineCap::Butt)
                    return;

                Polyline capPiece;
                capPiece.push_back(Offset(end, tangent, 1.0));
                AddCap(capPiece, end, tangent);
                capPiece.push_back(Offset(end, tangent, -1.0));
                AddPiece(capPiece);
            }

            static bool IsTurned(const Point& tangent, const Point& chord)
            {
                return !IsZero(tangent) && fabs(Cross(tangent, Normalized(chord))) > kMinChordTurn;
            }

            void Add(const PathPolyline& points, bool closed)
            {
                size_t count = points.size();
                if (count == 0)
                    return;

                if (count == 1)
                {
                    AddDot(points[0].position);
                    return;
                }

                if (closed)
                {
                    AddChain(points, true, true, true);
                    return;
                }

                // Chords at the ends of curves are turned from the curve, their pieces are built apart
                PathPolyline chain = points;
                Point startChord = points[1].position - points[0].position;
                Point endChord = points[count - 1].position - points[count - 2].position;
                bool startCut = IsTurned(points[0].tangentOut, startChord);
                bool endCut = IsTurned(points[count - 1].tangentIn, endChord);

                if (startCut)
                {
                    AddCurveEnd(points[0].position, points[1].position, points[0].tangentOut*-1.0);
                    chain[0].position = Snap(points[1].position -
                                             startChord*Math::Min(kCurveEndStub/Length(startChord), 1.0));
                    chain[0].tangentOut = Point();
                }

                if (endCut)
                {
                    AddCurveEnd(points[count - 1].position, points[count - 2].position, points[count - 1].tangentIn);
                    chain[count - 1].position = Snap(points[count - 2].position +
                                                     endChord*Math::Min(kCurveEndStub/Length(endChord), 1.0));
                    chain[count - 1].tangentIn = Point();
                }

                if (count > 2 || !startCut || !endCut)
                    AddChain(chain, false, !startCut, !endCut);
            }

            void AddChain(const PathPolyline& points, bool closed, bool startCap, bool endCap)
            {
                size_t count = points.size();
                size_t segmentsCount = closed ? count : count - 1;
                std::vector<Point> directions(segmentsCount);
                std::vector<double> lengths(segmentsCount);
                for (size_t i = 0; i < segmentsCount; i++)
                {
                    Point delta = points[(i + 1)%count].position - points[i].position;
                    lengths[i] = Length(delta);
                    directions[i] = delta*(1.0/lengths[i]);
                }

                auto tangentIn = [&](size_t idx, size_t segment)
                {
                    const PathVertex& vertex = points[idx];
                    return vertex.joint && !IsZero(vertex.tangentIn) ? vertex.tangentIn : directions[segment];
                };

                auto tangentOut = [&](size_t idx, size_t segment)
                {
                    const PathVertex& vertex = points[idx];
                    return vertex.joint && !IsZero(vertex.tangentOut) ? vertex.tangentOut : directions[segment];
                };

                auto addVertex = [&](Polyline& side, size_t idx, size_t segmentIn, size_t segmentOut, double sign)
                {
                    const Point& vertex = points[idx].position;
                    if (IsCorner(points[idx]))
                    {
                        Point in = tangentIn(idx, segmentIn), out = tangentOut(idx, segmentOut);
                        AddChordCorner(side, vertex, directions[segmentIn], in, sign, true);
                        AddJoin(side, vertex, in, out, sign, join, miterLimit);
                        AddChordCorner(side, vertex, directions[segmentOut], out, sign, false);
                        return;
                    }

                    const Point& in = directions[segmentIn];
                    const Point& out = directions[segmentOut];
                    size_t begin = side.size();

                    double sine = Cross(in, out), cosine = Dot(in, out);
                    double reach = halfWidth*fabs(sine)/(1.0 + cosine);
                    double room = Math::Min(lengths[segmentIn], lengths[segmentOut])*0.45;
                    if (cosine > 0.5 && (sine*sign >= 0 || reach < room))
                    {
                        Point normals = Point(in.y, -in.x) + Point(out.y, -out.x);
                        side.push_back(Snap(vertex + normals*(sign*halfWidth/(1.0 + cosine))));
                    }
                    else
                        AddJoin(side, vertex, in, out, sign, VectorLineJoin::Round, miterLimit);

                    for (size_t i = begin; i < side.size(); i++)
                        smoothPoints.insert(KeyOf(side[i]));
                };

                Polyline sides[2];
                if (closed)
                {
                    for (int k = 0; k < 2; k++)
                    {
                        for (size_t i = 0; i < count; i++)
                            addVertex(sides[k], i, (i + count - 1)%count, i, k == 0 ? 1.0 : -1.0);
                    }

                    std::reverse(sides[1].begin(), sides[1].end());
                    polygons.push_back(sides[0]);
                    polygons.push_back(sides[1]);
                    return;
                }

                const Point& start = points[0].position;
                const Point& end = points.back().position;
                Point startDirection = tangentOut(0, 0), endDirection = tangentIn(count - 1, segmentsCount - 1);

                for (int k = 0; k < 2; k++)
                {
                    double sign = k == 0 ? 1.0 : -1.0;
                    Polyline& side = sides[k];

                    side.push_back(Offset(start, startDirection, sign));
                    for (size_t i = 1; i + 1 < count; i++)
                        addVertex(side, i, i - 1, i, sign);

                    side.push_back(Offset(end, endDirection, sign));
                }

                Polyline polygon = sides[0];
                if (endCap)
                    AddCap(polygon, end, endDirection);

                polygon.insert(polygon.end(), sides[1].rbegin(), sides[1].rend());
                if (startCap)
                    AddCap(polygon, start, startDirection*-1.0);

                polygons.push_back(polygon);
            }
        };

        class MeshBuilder
        {
        public:
            MeshBuilder(VectorMesh& mesh, const Point& pixelScale):
                mMesh(mesh), mPixelScale(pixelScale)
            {}

            // Triangles added after it overlap the previous ones
            void BeginLayer()
            {
                mLayers.Add((int)mMesh.indexes.Count());
            }

            // Removes the vertices that the triangles of their layer draw the same without
            void Simplify(float colorTolerance)
            {
                if (colorTolerance > 0.0f)
                    VectorMeshSimplifier::Simplify(mMesh, mLayers, colorTolerance);
            }

            void AddTriangle(const CoverageVertex* vertices, const Color32Bit* colors)
            {
                if (((colors[0] | colors[1] | colors[2]) >> 24) == 0)
                    return;

                double area = (vertices[1].x - vertices[0].x)*(vertices[2].y - vertices[0].y) -
                    (vertices[1].y - vertices[0].y)*(vertices[2].x - vertices[0].x);
                if (fabs(area) < 1e-9)
                    return;

                for (int i = 0; i < 3; i++)
                    mMesh.indexes.Add(GetVertexIndex(vertices[i], colors[i]));
            }

        private:
            struct Key
            {
                long long  x, y;
                Color32Bit color;

                bool operator==(const Key& other) const
                {
                    return x == other.x && y == other.y && color == other.color;
                }
            };

            struct KeyHash
            {
                size_t operator()(const Key& key) const
                {
                    unsigned long long hash = (unsigned long long)key.x*0x9E3779B97F4A7C15ull;
                    hash ^= (unsigned long long)key.y*0xC2B2AE3D27D4EB4Full + (hash << 6) + (hash >> 2);
                    hash ^= (unsigned long long)key.color*0x165667B19E3779F9ull + (hash << 6) + (hash >> 2);
                    return (size_t)hash;
                }
            };

        private:
            VectorMesh& mMesh;
            Point       mPixelScale;
            Vector<int> mLayers; // First indexes of the triangles of each paint

            std::unordered_map<Key, VertexIndex, KeyHash> mWelded;

        private:
            VertexIndex GetVertexIndex(const CoverageVertex& vertex, Color32Bit color)
            {
                Key key = { llround(vertex.x*kGrid), llround(vertex.y*kGrid), color };
                auto found = mWelded.find(key);
                if (found != mWelded.end())
                    return found->second;

                VertexIndex idx = (VertexIndex)mMesh.positions.Count();
                mMesh.positions.Add(Vec2F((float)(vertex.x/mPixelScale.x), (float)(vertex.y/mPixelScale.y)));
                mMesh.colors.Add(color);
                mWelded[key] = idx;
                return idx;
            }
        };

        Color32Bit PackColor(const Color4& color, double alphaScale)
        {
            int alpha = Math::Clamp((int)floor((double)color.a*alphaScale + 0.5), 0, 255);
            return ((Color32Bit)alpha << 24) | ((Color32Bit)Math::Clamp(color.b, 0, 255) << 16) |
                ((Color32Bit)Math::Clamp(color.g, 0, 255) << 8) | (Color32Bit)Math::Clamp(color.r, 0, 255);
        }

        // Splits edges at their middles while isFlat(a, b) is false; both triangles of an edge split it the same
        template<typename _is_flat>
        void Subdivide(TriangleSoup& soup, _is_flat&& isFlat)
        {
            const double minEdge = 1.0;
            const double minArea = 1e-6;

            for (size_t i = 0; i + 2 < soup.size(); i += 3)
            {
                while (soup.size() < kMaxTriangleVertices)
                {
                    double area = (soup[i + 1].x - soup[i].x)*(soup[i + 2].y - soup[i].y) -
                        (soup[i + 1].y - soup[i].y)*(soup[i + 2].x - soup[i].x);
                    if (fabs(area) < minArea)
                        break;

                    int longest = -1;
                    double longestLength = minEdge*minEdge;
                    for (int k = 0; k < 3; k++)
                    {
                        const CoverageVertex& a = soup[i + k];
                        const CoverageVertex& b = soup[i + (k + 1)%3];
                        double length = (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
                        if (length > longestLength && !isFlat(a, b))
                        {
                            longestLength = length;
                            longest = k;
                        }
                    }

                    if (longest < 0)
                        break;

                    CoverageVertex a = soup[i + longest], b = soup[i + (longest + 1)%3], c = soup[i + (longest + 2)%3];
                    CoverageVertex middle = { (a.x + b.x)*0.5, (a.y + b.y)*0.5, (a.coverage + b.coverage)*0.5f, 0.0f };

                    soup[i] = a;
                    soup[i + 1] = middle;
                    soup[i + 2] = c;
                    soup.push_back(middle);
                    soup.push_back(b);
                    soup.push_back(c);
                }
            }
        }

        void SplitByCoordinate(TriangleSoup& soup, bool vertical, double coordinate)
        {
            VectorDetails::SplitTrianglesByValue(soup, coordinate,
                                  [&](const CoverageVertex& vertex) { return vertical ? vertex.x : vertex.y; },
                                  [&](const CoverageVertex& a, const CoverageVertex& b, double coef)
            {
                CoverageVertex res = { a.x + (b.x - a.x)*coef, a.y + (b.y - a.y)*coef,
                                       (float)(a.coverage + (b.coverage - a.coverage)*coef), 0.0f };
                (vertical ? res.x : res.y) = coordinate;
                return res;
            });
        }

        // Maximum change of the paint color channels, in levels per unit of the ramp offset
        double GetRampSlope(const VectorPaint& paint, double opacity)
        {
            double slope = 1.0;
            for (int i = 1; i < paint.stops.Count(); i++)
            {
                const Color4& from = paint.stops[i - 1].color;
                const Color4& to = paint.stops[i].color;
                double range = Math::Max((double)paint.stops[i].offset - paint.stops[i - 1].offset, 1e-3);
                int difference = Math::Max(Math::Max(Math::Abs(to.r - from.r), Math::Abs(to.g - from.g)),
                                           Math::Max(Math::Abs(to.b - from.b), Math::Abs(to.a - from.a)));
                slope = Math::Max(slope, difference*opacity/range);
            }

            return slope;
        }

        // Splits triangles so the ramp offset is linear inside each; true when split by the lines of pixel centers
        bool SplitByRadialRamp(TriangleSoup& soup, const VectorPaint& paint, double opacity, const Point& pixelScale)
        {
            const double bound = 1.0625;
            const double maxColorError = 1.0;     // In levels, from the polygons standing for the circles
            const double minDeviation = 1.0/32.0; // Of a polygon from its circle, in pixels, that is worth more sectors
            const int minSectors = 8, maxSectors = 64;

            Basis inverse = paint.transform.Inverted();
            auto toLocal = [&](float x, float y)
            {
                Vec2F local = inverse.Transform(Vec2F(x, y));
                return Point(((double)local.x - paint.center.x)/paint.radius,
                             ((double)local.y - paint.center.y)/paint.radius);
            };

            // Gradient space, where the last stop is the unit circle, is an affine map of the pixels
            Point origin = toLocal(0, 0);
            Point axisX = (toLocal(1, 0) - origin)*(1.0/pixelScale.x);
            Point axisY = (toLocal(0, 1) - origin)*(1.0/pixelScale.y);
            auto local = [&](const CoverageVertex& vertex)
            {
                return origin + axisX*vertex.x + axisY*vertex.y;
            };

            auto lerp = [](const CoverageVertex& a, const CoverageVertex& b, double coef)
            {
                return CoverageVertex{ a.x + (b.x - a.x)*coef, a.y + (b.y - a.y)*coef,
                                       (float)(a.coverage + (b.coverage - a.coverage)*coef),
                                       (float)(a.offset + (b.offset - a.offset)*coef) };
            };

            double determinant = fabs(Cross(axisX, axisY));
            double pixelRadius = determinant > 1e-12 ? 1.0/sqrt(determinant) : 1.0;
            double epsilon = Math::Max(kSplitSnap/pixelRadius, 1e-7);

            auto splitByOffset = [&](TriangleSoup& triangles, float threshold)
            {
                VectorDetails::SplitTrianglesByValue(triangles, threshold,
                                                     [](const CoverageVertex& vertex) { return vertex.offset; },
                                                     [&](const CoverageVertex& a, const CoverageVertex& b, double coef)
                {
                    CoverageVertex res = lerp(a, b, coef);
                    res.offset = threshold;
                    return res;
                }, epsilon);
            };

            auto localX = [&](const CoverageVertex& vertex) { return local(vertex).x; };
            auto localY = [&](const CoverageVertex& vertex) { return local(vertex).y; };
            for (double side : { -bound, bound })
            {
                VectorDetails::SplitTrianglesByValue(soup, side, localX, lerp, epsilon);
                VectorDetails::SplitTrianglesByValue(soup, side, localY, lerp, epsilon);
            }

            auto centerOf = [&](const TriangleSoup& triangles, size_t idx)
            {
                return (local(triangles[idx]) + local(triangles[idx + 1]) + local(triangles[idx + 2]))*(1.0/3.0);
            };

            TriangleSoup inner, outer;
            for (size_t i = 0; i + 2 < soup.size(); i += 3)
            {
                Point center = centerOf(soup, i);
                TriangleSoup& target = fabs(center.x) < bound && fabs(center.y) < bound ? inner : outer;
                target.insert(target.end(), soup.begin() + i, soup.begin() + i + 3);
            }

            for (CoverageVertex& vertex : outer)
                vertex.offset = (float)bound;

            Point min(1e30, 1e30), max(-1e30, -1e30);
            double pixelsCount = 0;
            for (size_t i = 0; i + 2 < inner.size(); i += 3)
            {
                pixelsCount += fabs((inner[i + 1].x - inner[i].x)*(inner[i + 2].y - inner[i].y) -
                                    (inner[i + 1].y - inner[i].y)*(inner[i + 2].x - inner[i].x))*0.5;

                for (int k = 0; k < 3; k++)
                {
                    min = Point(Math::Min(min.x, inner[i + k].x), Math::Min(min.y, inner[i + k].y));
                    max = Point(Math::Max(max.x, inner[i + k].x), Math::Max(max.y, inner[i + k].y));
                }
            }

            TriangleSoup source = inner;

            auto splitByRays = [&](double firstAngle, double step, int count)
            {
                for (int i = 0; i < count; i++)
                {
                    Point normal(-sin(firstAngle + step*i), cos(firstAngle + step*i));
                    VectorDetails::SplitTrianglesByValue(inner, 0.0, [&](const CoverageVertex& vertex)
                    {
                        return Dot(local(vertex), normal);
                    }, lerp, epsilon);
                }
            };

            // Distance along the middle of a sector is linear and differs from the radius evenly to both sides
            auto setOffsets = [&](double sectorAngle)
            {
                double scale = 2.0/(1.0 + cos(sectorAngle*0.5));
                for (size_t i = 0; i + 2 < inner.size(); i += 3)
                {
                    Point center = centerOf(inner, i);
                    double middle = (floor(atan2(center.y, center.x)/sectorAngle) + 0.5)*sectorAngle;
                    Point direction(cos(middle), sin(middle));
                    for (int k = 0; k < 3; k++)
                        inner[i + k].offset = (float)Math::Max(Dot(local(inner[i + k]), direction)*scale, 0.0);
                }
            };

            double slope = GetRampSlope(paint, opacity);
            double lastOffset = paint.stops.back().offset;

            int sectors = minSectors;
            splitByRays(0.0, 2.0*kPi/sectors, sectors/2);
            setOffsets(2.0*kPi/sectors);

            // Polygon is off its circle by angle^2/16 of the radius: more sectors are needed farther from the center
            soup.clear();
            for (; sectors < maxSectors; sectors *= 2)
            {
                double angle = 2.0*kPi/sectors;
                double limit = Math::Max(maxColorError/slope, minDeviation/pixelRadius)*16.0/(angle*angle);
                if (limit >= lastOffset)
                    break;

                splitByOffset(inner, (float)limit);

                TriangleSoup rest;
                for (size_t i = 0; i + 2 < inner.size(); i += 3)
                {
                    bool isNear = inner[i].offset + inner[i + 1].offset + inner[i + 2].offset < limit*3.0;
                    TriangleSoup& target = isNear ? soup : rest;
                    target.insert(target.end(), inner.begin() + i, inner.begin() + i + 3);
                }

                inner.swap(rest);
                splitByRays(angle*0.5, angle, sectors/2);
                setOffsets(angle*0.5);
            }

            soup.insert(soup.end(), inner.begin(), inner.end());

            for (int i = 0; i < paint.stops.Count(); i++)
            {
                float threshold = paint.stops[i].offset;
                if ((i == 0 || threshold != paint.stops[i - 1].offset) && threshold > 0.0f)
                    splitByOffset(soup, threshold);
            }

            bool byPixels = soup.size()/3 > source.size()/3 + (size_t)(pixelsCount*2.0);
            if (byPixels)
            {
                // Each triangle is cut by the lattice lines that cross it; the cutting stops as soon as it gives
                // more triangles than the sectors do
                TriangleSoup cells, pieces;
                for (size_t i = 0; i + 2 < source.size() && byPixels; i += 3)
                {
                    pieces.assign(source.begin() + i, source.begin() + i + 3);

                    for (int axis = 0; axis < 2; axis++)
                    {
                        auto coordinate = [&](const CoverageVertex& vertex) { return axis == 0 ? vertex.x : vertex.y; };

                        double from = Math::Min(Math::Min(coordinate(source[i]), coordinate(source[i + 1])),
                                                coordinate(source[i + 2]));
                        double to = Math::Max(Math::Max(coordinate(source[i]), coordinate(source[i + 1])),
                                              coordinate(source[i + 2]));

                        double first = LatticeLineAfter(axis == 0 ? min.x : min.y);
                        double end = Math::Min(axis == 0 ? max.x : max.y, to + kSplitSnap);
                        for (double line = first + Math::Max(floor(from - kSplitSnap - first), 0.0); line < end; line += 1.0)
                            VectorDetails::SplitTrianglesByValue(pieces, line, coordinate, lerp, kSplitSnap);
                    }

                    cells.insert(cells.end(), pieces.begin(), pieces.end());
                    byPixels = cells.size() + (source.size() - i - 3) < soup.size();
                }

                if (byPixels)
                    source.swap(cells);
            }

            if (byPixels)
            {
                soup.swap(source);
                for (CoverageVertex& vertex : soup)
                    vertex.offset = (float)Length(local(vertex));
            }

            soup.insert(soup.end(), outer.begin(), outer.end());
            return byPixels;
        }

        void ApplyPaint(TriangleSoup& soup, const VectorPaint& sourcePaint, double opacity, const Point& pixelScale,
                        MeshBuilder& builder)
        {
            Color32Bit colors[3];

            builder.BeginLayer();

            if (sourcePaint.type == VectorPaintType::Solid)
            {
                for (size_t i = 0; i + 2 < soup.size(); i += 3)
                {
                    for (int k = 0; k < 3; k++)
                        colors[k] = PackColor(sourcePaint.color, opacity*soup[i + k].coverage);

                    builder.AddTriangle(&soup[i], colors);
                }

                return;
            }

            VectorPaint paint = sourcePaint;
            // Difference of the stops is seen multiplied by the opacity
            float stopsTolerance = kStopsTolerance/(float)Math::Clamp(opacity, 0.05, 1.0);
            paint.stops = VectorTessellator::SimplifyStops(sourcePaint.stops, stopsTolerance);
            if (paint.stops.IsEmpty())
                return;

            auto offsetAt = [&](double x, double y)
            {
                return paint.GetRampOffset(Vec2F((float)(x/pixelScale.x), (float)(y/pixelScale.y)));
            };

            bool vertexSpans = false;

            if (paint.type == VectorPaintType::LinearGradient)
            {
                double origin = offsetAt(0, 0);
                double slopeX = offsetAt(pixelScale.x, 0) - origin, slopeY = offsetAt(0, pixelScale.y) - origin;
                for (CoverageVertex& vertex : soup)
                    vertex.offset = (float)(origin + slopeX*vertex.x/pixelScale.x + slopeY*vertex.y/pixelScale.y);

                double epsilon = Math::Max(sqrt(slopeX*slopeX/(pixelScale.x*pixelScale.x) +
                                                slopeY*slopeY/(pixelScale.y*pixelScale.y))*kSplitSnap, 1e-7);

                for (size_t i = 0; i < paint.stops.size(); i++)
                {
                    if (i > 0 && paint.stops[i].offset == paint.stops[i - 1].offset)
                        continue;

                    float threshold = paint.stops[i].offset;
                    VectorDetails::SplitTrianglesByValue(soup, threshold,
                                          [](const CoverageVertex& vertex) { return vertex.offset; },
                                          [&](const CoverageVertex& a, const CoverageVertex& b, double coef)
                    {
                        return CoverageVertex{ a.x + (b.x - a.x)*coef, a.y + (b.y - a.y)*coef,
                                               (float)(a.coverage + (b.coverage - a.coverage)*coef), threshold };
                    }, epsilon);
                }
            }
            else if (paint.focal == paint.center && paint.radius > 0.0f)
                vertexSpans = SplitByRadialRamp(soup, paint, opacity, pixelScale);
            else
            {
                const float maxColorError = 2.5f;
                const int gridLines = 2;

                // Without the grid over the gradient circle all checked points of a large triangle can be out of it
                Vec2F center = paint.transform.Transform(paint.center);
                double centerX = center.x*pixelScale.x, centerY = center.y*pixelScale.y;
                double extentX = paint.radius*Math::Sqrt(paint.transform.xv.x*paint.transform.xv.x +
                                                         paint.transform.yv.x*paint.transform.yv.x)*pixelScale.x;
                double extentY = paint.radius*Math::Sqrt(paint.transform.xv.y*paint.transform.xv.y +
                                                         paint.transform.yv.y*paint.transform.yv.y)*pixelScale.y;

                for (int i = -gridLines; i <= gridLines; i++)
                {
                    SplitByCoordinate(soup, true, centerX + extentX*i/gridLines);
                    SplitByCoordinate(soup, false, centerY + extentY*i/gridLines);
                }

                auto colorAt = [&](double x, double y) { return paint.GetRampColor(offsetAt(x, y)); };

                auto isNear = [&](int from, int to, int value, float coef)
                {
                    return Math::Abs(Math::Lerp((float)from, (float)to, coef) - (float)value) <= maxColorError;
                };

                // Edge is flat when colors inside of it are the interpolation of its ends colors
                Subdivide(soup, [&](const CoverageVertex& a, const CoverageVertex& b)
                {
                    Color4 colorA = colorAt(a.x, a.y), colorB = colorAt(b.x, b.y);
                    for (float coef : { 0.25f, 0.5f, 0.75f })
                    {
                        Color4 color = colorAt(a.x + (b.x - a.x)*coef, a.y + (b.y - a.y)*coef);
                        if (!isNear(colorA.r, colorB.r, color.r, coef) || !isNear(colorA.g, colorB.g, color.g, coef) ||
                            !isNear(colorA.b, colorB.b, color.b, coef) || !isNear(colorA.a, colorB.a, color.a, coef))
                        {
                            return false;
                        }
                    }

                    return true;
                });

                for (CoverageVertex& vertex : soup)
                    vertex.offset = offsetAt(vertex.x, vertex.y);
            }

            auto spanOf = [&](float offset)
            {
                size_t span = 1;
                while (span + 1 < paint.stops.size() && paint.stops[span].offset < offset)
                    span++;

                return span;
            };

            // Colors are taken from the ramp span of the triangle, so equal offset stops make a hard step; or from
            // the span of each vertex, when the triangles are not split by the stops
            for (size_t i = 0; i + 2 < soup.size(); i += 3)
            {
                float center = (soup[i].offset + soup[i + 1].offset + soup[i + 2].offset)/3.0f;
                for (int k = 0; k < 3; k++)
                {
                    const CoverageVertex& vertex = soup[i + k];
                    float spanOffset = vertexSpans ? vertex.offset : center;
                    size_t span = spanOf(spanOffset);

                    const VectorGradientStop& from = paint.stops.size() > 1 ? paint.stops[span - 1] : paint.stops[0];
                    const VectorGradientStop& to = paint.stops.size() > 1 ? paint.stops[span] : paint.stops[0];
                    float range = to.offset - from.offset;
                    float coef = range > 0.0f ? Math::Clamp01((vertex.offset - from.offset)/range) :
                        (spanOffset < from.offset ? 0.0f : 1.0f);

                    Color4 color(Math::RoundToInt(Math::Lerp((float)from.color.r, (float)to.color.r, coef)),
                                 Math::RoundToInt(Math::Lerp((float)from.color.g, (float)to.color.g, coef)),
                                 Math::RoundToInt(Math::Lerp((float)from.color.b, (float)to.color.b, coef)), 255);
                    double alpha = Math::Lerp((float)from.color.a, (float)to.color.a, coef)/255.0;
                    colors[k] = PackColor(color, alpha*opacity*vertex.coverage);
                }

                builder.AddTriangle(&soup[i], colors);
            }
        }

        void BuildCoverage(const std::vector<Polyline>& polygons, VectorFillRule rule, bool antialiasing,
                           bool hardAligned, const SmoothPoints& smoothPoints, TriangleSoup& soup)
        {
            if (!antialiasing)
            {
                Resolve(polygons, rule, nullptr, &soup);
                return;
            }

            std::vector<Polyline> loops;
            Resolve(polygons, rule, &loops, nullptr);
            BuildFringedCoverage(loops, smoothPoints, hardAligned, nullptr, soup);
        }

        Point GetPixelScale(const VectorTessellationParams& params)
        {
            // A strongly anisotropic scale splits geometry into slivers and takes seconds to tessellate
            const float maxAxesRatio = 16.0f;
            float maxScale = Math::Max(Math::Max(params.pixelScale.x, params.pixelScale.y), 1e-4f);

            return Point(Math::Max(params.pixelScale.x, maxScale/maxAxesRatio),
                         Math::Max(params.pixelScale.y, maxScale/maxAxesRatio));
        }

        bool IsOpaque(const VectorPaint& paint)
        {
            if (paint.type == VectorPaintType::Solid)
                return paint.color.a >= 255;

            for (const VectorGradientStop& stop : paint.stops)
            {
                if (stop.color.a < 255)
                    return false;
            }

            return !paint.stops.IsEmpty();
        }

        bool IsSameSolid(const VectorPaint& a, const VectorPaint& b)
        {
            return a.type == VectorPaintType::Solid && b.type == VectorPaintType::Solid && a.color == b.color;
        }

        // False when the fill has an edge without the stroke: the one that closes an open sub path
        bool IsClosed(const VectorShape& shape)
        {
            for (const VectorSubPath& path : shape.subPaths)
            {
                bool hasArea = path.segments.Count() > 1 || (path.segments.Count() == 1 && path.segments[0].cubic);
                if (!path.closed && hasArea)
                    return false;
            }

            return true;
        }

        void TessellateShape(const VectorShape& shape, const VectorTessellationParams& params, MeshBuilder& builder)
        {
            Point pixelScale = GetPixelScale(params);
            double tolerance = Math::Max(params.curveTolerance, 0.01f);

            TriangleSoup soup;
            PathPolyline points;

            double fillOpacity = (double)shape.fillOpacity*shape.opacity;
            double strokeOpacity = (double)shape.strokeOpacity*shape.opacity;
            double strokeWidth = shape.strokeWidth*sqrt(pixelScale.x*pixelScale.y);
            bool hasFill = !shape.fill.IsNone() && fillOpacity > 0.0;
            bool hasStroke = !shape.stroke.IsNone() && strokeOpacity > 0.0 && strokeWidth > 0.0 && strokeWidth < 1e6;

            std::vector<Polyline> outlines;
            SmoothPoints strokeSmoothPoints;
            if (hasStroke)
            {
                Stroker stroker(outlines, strokeSmoothPoints);
                stroker.halfWidth = strokeWidth*0.5;
                stroker.cap = shape.strokeCap;
                stroker.join = shape.strokeJoin;
                stroker.miterLimit = Math::Max(shape.strokeMiterLimit, 1.0f);
                stroker.tolerance = tolerance;

                for (const VectorSubPath& path : shape.subPaths)
                {
                    points.clear();

                    // Chords of the outer side of a curve stroke deviate as of a curve that is larger by half width
                    Flattener flattener(points);
                    flattener.tolerance = tolerance;
                    flattener.minChordCos = stroker.halfWidth > tolerance ? 1.0 - tolerance/stroker.halfWidth : -1.0;
                    if (!flattener.Path(path, pixelScale))
                        return;

                    stroker.Add(points, path.closed);
                }
            }

            if (hasFill)
            {
                bool opaqueStroke = hasStroke && strokeOpacity >= 1.0 && IsOpaque(shape.stroke);
                bool merged = opaqueStroke && IsSameSolid(shape.fill, shape.stroke) && fillOpacity >= 1.0;

                std::vector<Polyline> polygons;
                SmoothPoints smoothPoints;
                for (const VectorSubPath& path : shape.subPaths)
                {
                    points.clear();

                    Flattener flattener(points);
                    flattener.tolerance = tolerance;
                    if (!flattener.Path(path, pixelScale))
                        return;

                    if (points.size() > 1 && points.back().position == points[0].position)
                        points.pop_back();

                    if (points.size() < 3)
                        continue;

                    Polyline polygon;
                    for (const PathVertex& vertex : points)
                    {
                        polygon.push_back(vertex.position);
                        if (!IsCorner(vertex))
                            smoothPoints.insert(KeyOf(vertex.position));
                    }

                    polygons.push_back(polygon);
                }

                if (merged)
                {
                    // Stroke of the same opaque color is one area with the fill, it has no fringes inside
                    std::vector<Polyline> loops;
                    Resolve(polygons, shape.fillRule, &loops, nullptr);
                    loops.insert(loops.end(), outlines.begin(), outlines.end());
                    smoothPoints.insert(strokeSmoothPoints.begin(), strokeSmoothPoints.end());

                    BuildCoverage(loops, VectorFillRule::NonZero, params.antialiasing, params.pixelSnapped,
                                  smoothPoints, soup);
                    ApplyPaint(soup, shape.fill, fillOpacity, pixelScale, builder);
                    return;
                }

                // Fringes of the fill and of an opaque stroke blended one over another leave the outline translucent.
                // Stroke of a pixel width hides the edge of the fill, a thinner one or of an open path does not
                if (opaqueStroke && params.antialiasing && strokeWidth >= 1.0 && IsClosed(shape))
                    BuildCoverage(polygons, shape.fillRule, false, false, smoothPoints, soup);
                else if (opaqueStroke && params.antialiasing)
                {
                    std::vector<Polyline> fillLoops, strokeLoops, unitedLoops;
                    Resolve(polygons, shape.fillRule, &fillLoops, nullptr);
                    Resolve(outlines, VectorFillRule::NonZero, &strokeLoops, nullptr);

                    std::vector<Polyline> both = fillLoops;
                    both.insert(both.end(), strokeLoops.begin(), strokeLoops.end());
                    Resolve(both, VectorFillRule::NonZero, &unitedLoops, nullptr);

                    CoverageField unitedField(unitedLoops), strokeField(strokeLoops);
                    StrokeCover strokeCover = { &unitedField, &strokeField };
                    BuildFringedCoverage(fillLoops, smoothPoints, false, &strokeCover, soup);
                }
                else
                {
                    BuildCoverage(polygons, shape.fillRule, params.antialiasing, params.pixelSnapped, smoothPoints,
                                  soup);
                }

                ApplyPaint(soup, shape.fill, fillOpacity, pixelScale, builder);
            }

            if (hasStroke)
            {
                soup.clear();
                BuildCoverage(outlines, VectorFillRule::NonZero, params.antialiasing, params.pixelSnapped,
                              strokeSmoothPoints, soup);
                ApplyPaint(soup, shape.stroke, strokeOpacity, pixelScale, builder);
            }
        }
    }

    void VectorTessellator::Tessellate(const VectorImage& image, VectorMesh& mesh,
                                       const VectorTessellationParams& params /*= VectorTessellationParams()*/)
    {
        mesh.Clear();
        mesh.size = image.size;
        Point pixelScale = GetPixelScale(params);
        mesh.pixelScale = Vec2F((float)pixelScale.x, (float)pixelScale.y);

        MeshBuilder builder(mesh, pixelScale);
        for (const VectorShape& shape : image.shapes)
            o2::TessellateShape(shape, params, builder);

        builder.Simplify(params.simplification);
    }

    void VectorTessellator::TessellateShape(const VectorShape& shape, VectorMesh& mesh,
                                            const VectorTessellationParams& params /*= VectorTessellationParams()*/)
    {
        Point pixelScale = GetPixelScale(params);
        mesh.pixelScale = Vec2F((float)pixelScale.x, (float)pixelScale.y);

        MeshBuilder builder(mesh, pixelScale);
        o2::TessellateShape(shape, params, builder);
        builder.Simplify(params.simplification);
    }

    Vector<VectorGradientStop> VectorTessellator::SimplifyStops(const Vector<VectorGradientStop>& stops,
                                                                float tolerance /*= 1.5f*/)
    {
        if (stops.Count() <= 2)
            return stops;

        auto fits = [&](int from, int to)
        {
            float range = stops[to].offset - stops[from].offset;
            if (range <= 0.0f)
                return false;

            const Color4& first = stops[from].color;
            const Color4& last = stops[to].color;
            for (int i = from + 1; i < to; i++)
            {
                float coef = (stops[i].offset - stops[from].offset)/range;
                const Color4& color = stops[i].color;
                if (Math::Abs(Math::Lerp((float)first.r, (float)last.r, coef) - (float)color.r) > tolerance ||
                    Math::Abs(Math::Lerp((float)first.g, (float)last.g, coef) - (float)color.g) > tolerance ||
                    Math::Abs(Math::Lerp((float)first.b, (float)last.b, coef) - (float)color.b) > tolerance ||
                    Math::Abs(Math::Lerp((float)first.a, (float)last.a, coef) - (float)color.a) > tolerance)
                {
                    return false;
                }
            }

            return true;
        };

        Vector<VectorGradientStop> res;
        res.Add(stops[0]);

        int anchor = 0;
        for (int i = 2; i < stops.Count(); i++)
        {
            if (fits(anchor, i))
                continue;

            anchor = i - 1;
            res.Add(stops[anchor]);
        }

        res.Add(stops.back());
        return res;
    }
}
