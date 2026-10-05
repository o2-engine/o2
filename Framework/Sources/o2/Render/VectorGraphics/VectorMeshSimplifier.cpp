#include "o2/stdafx.h"
#include "VectorMeshSimplifier.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace o2
{
    namespace
    {
        const double kPositionGrid = 4096.0;        // Vertices of a layer at the same cell of a pixel are one point
        const double kMaxOutlineShift = 1.0/2048.0; // Pixels from a removed outline vertex to the new outline
        const double kMinArea = 1e-9;               // Doubled area of a triangle in pixels, smaller is degenerate
        const double kInsideEpsilon = 1e-7;
        const int    kMaxFanSize = 32;
        const int    kMaxPasses = 16;

        struct Point
        {
            double x = 0, y = 0;
        };

        double Cross(const Point& origin, const Point& a, const Point& b)
        {
            return (a.x - origin.x)*(b.y - origin.y) - (a.y - origin.y)*(b.x - origin.x);
        }

        // Point of the source mesh with its color: the simplified triangles are checked against all of them
        struct Sample
        {
            Point position;
            float channels[4];
        };

        struct Triangle
        {
            int  vertices[3];
            bool alive = true;

            std::vector<Sample> removed; // Removed vertices of the source mesh that are inside
        };

        // Triangles of a layer; vertices are indexes in the mesh
        class Layer
        {
        public:
            Layer(const VectorMesh& mesh, const VertexIndex* indexes, int indexesCount, double colorTolerance,
                  std::vector<int>& localIndexes):
                mMesh(mesh), mColorTolerance(colorTolerance), mLocalIndexes(localIndexes)
            {
                Point scale = { Math::Max((double)mesh.pixelScale.x, 1e-4), Math::Max((double)mesh.pixelScale.y, 1e-4) };

                mTriangles.reserve(indexesCount/3);
                mNextCorners.reserve(indexesCount);

                for (int i = 0; i + 2 < indexesCount; i += 3)
                {
                    int vertices[3];
                    for (int k = 0; k < 3; k++)
                        vertices[k] = GetLocalIndex(indexes[i + k], scale);

                    AddTriangle(vertices);
                }

                // Vertices of different colors at one point are a seam, removing one side of it opens a crack
                std::vector<std::pair<std::pair<long long, long long>, int>> points(mVertices.size());
                for (int i = 0; i < (int)mVertices.size(); i++)
                {
                    points[i] = { { llround(mPositions[i].x*kPositionGrid), llround(mPositions[i].y*kPositionGrid) }, i };
                }

                std::sort(points.begin(), points.end());

                mLocked.assign(mVertices.size(), 0);
                for (size_t i = 1; i < points.size(); i++)
                {
                    if (points[i].first == points[i - 1].first)
                    {
                        mLocked[points[i].second] = 1;
                        mLocked[points[i - 1].second] = 1;
                    }
                }
            }

            ~Layer()
            {
                for (VertexIndex vertex : mVertices)
                    mLocalIndexes[vertex] = -1;
            }

            void Simplify()
            {
                // A vertex that has stayed is tried again only after a vertex next to it was removed
                std::vector<char> changed(mVertices.size(), 1);

                for (int pass = 0; pass < kMaxPasses; pass++)
                {
                    bool removed = false;
                    for (int i = 0; i < (int)mVertices.size(); i++)
                    {
                        if (!changed[i])
                            continue;

                        changed[i] = 0;

                        if (mLocked[i] || mFirstCorners[i] < 0 || !RemoveVertex(i))
                            continue;

                        removed = true;
                        for (int ringVertex : mRing)
                            changed[ringVertex] = 1;
                    }

                    if (!removed)
                        break;
                }
            }

            void GetIndexes(Vector<VertexIndex>& indexes) const
            {
                for (const Triangle& triangle : mTriangles)
                {
                    if (!triangle.alive)
                        continue;

                    for (int vertex : triangle.vertices)
                        indexes.Add(mVertices[vertex]);
                }
            }

        private:
            const VectorMesh& mMesh;
            double            mColorTolerance;

            std::vector<int>& mLocalIndexes; // Index in the layer by index in the mesh, -1 out of the layer

            std::vector<VertexIndex>      mVertices;        // Indexes in the mesh
            std::vector<Point>            mPositions;       // In pixels
            std::vector<char>             mLocked;
            std::vector<Triangle>         mTriangles;

            // Alive triangles of a vertex are a list of their corners: corner is triangle*3 + place of the vertex
            std::vector<int> mFirstCorners; // By vertex, -1 at the end
            std::vector<int> mNextCorners;  // By corner, -1 at the end

            int mFanTriangles[kMaxFanSize]; // Triangles of the vertex being removed
            int mFanSize = 0;

            std::vector<int>    mRing, mPolygon, mFan, mCreated;
            std::vector<Sample> mSamples;

        private:
            int GetLocalIndex(VertexIndex vertex, const Point& scale)
            {
                int& local = mLocalIndexes[vertex];
                if (local < 0)
                {
                    local = (int)mVertices.size();
                    mVertices.push_back(vertex);
                    mPositions.push_back({ mMesh.positions[vertex].x*scale.x, mMesh.positions[vertex].y*scale.y });
                    mFirstCorners.push_back(-1);
                }

                return local;
            }

            int AddTriangle(const int* vertices)
            {
                int index = (int)mTriangles.size();

                Triangle triangle;
                for (int k = 0; k < 3; k++)
                {
                    triangle.vertices[k] = vertices[k];
                    mNextCorners.push_back(mFirstCorners[vertices[k]]);
                    mFirstCorners[vertices[k]] = index*3 + k;
                }

                mTriangles.push_back(triangle);
                return index;
            }

            void RemoveTriangle(int index)
            {
                Triangle& triangle = mTriangles[index];
                triangle.alive = false;
                triangle.removed.clear();

                for (int k = 0; k < 3; k++)
                {
                    int corner = index*3 + k;
                    int* link = &mFirstCorners[triangle.vertices[k]];
                    while (*link != corner)
                        link = &mNextCorners[*link];

                    *link = mNextCorners[corner];
                }
            }

            // Fills mFanTriangles with the triangles of the vertex; false when there are too many
            bool GetFan(int vertex)
            {
                mFanSize = 0;
                for (int corner = mFirstCorners[vertex]; corner >= 0; corner = mNextCorners[corner])
                {
                    if (mFanSize == kMaxFanSize)
                        return false;

                    mFanTriangles[mFanSize++] = corner/3;
                }

                return mFanSize > 0;
            }

            Sample GetSample(int vertex) const
            {
                Color32Bit color = mMesh.colors[mVertices[vertex]];
                return { mPositions[vertex], { (float)(color & 0xFF), (float)((color >> 8) & 0xFF),
                                               (float)((color >> 16) & 0xFF), (float)((color >> 24) & 0xFF) } };
            }

            // Fills mRing with the vertices around the vertex, counter clockwise; false when its triangles are not a chain
            bool GetRing(int vertex, bool& closed)
            {
                if (!GetFan(vertex))
                    return false;

                const int* triangles = mFanTriangles;
                int count = mFanSize;
                int from[kMaxFanSize], to[kMaxFanSize];

                for (int i = 0; i < count; i++)
                {
                    const int* vertices = mTriangles[triangles[i]].vertices;
                    int place = vertices[0] == vertex ? 0 : (vertices[1] == vertex ? 1 : 2);
                    from[i] = vertices[(place + 1)%3];
                    to[i] = vertices[(place + 2)%3];

                    double area = Cross(mPositions[vertex], mPositions[from[i]], mPositions[to[i]]);
                    if (fabs(area) < kMinArea)
                        return false;

                    if (area < 0)
                        std::swap(from[i], to[i]);
                }

                int start = -1;
                for (int i = 0; i < count; i++)
                {
                    bool hasPrevious = false;
                    for (int j = 0; j < count; j++)
                    {
                        if (i != j && from[i] == from[j])
                            return false;

                        hasPrevious = hasPrevious || to[j] == from[i];
                    }

                    if (!hasPrevious)
                    {
                        if (start >= 0)
                            return false;

                        start = i;
                    }
                }

                closed = start < 0;

                mRing.clear();

                int current = closed ? 0 : start;
                mRing.push_back(from[current]);
                for (int step = 0; step < count; step++)
                {
                    int end = to[current];
                    if (step == count - 1)
                    {
                        if (closed != (end == mRing[0]))
                            return false;

                        if (!closed)
                            mRing.push_back(end);

                        break;
                    }

                    mRing.push_back(end);

                    current = -1;
                    for (int j = 0; j < count; j++)
                    {
                        if (from[j] == end)
                            current = j;
                    }

                    if (current < 0)
                        return false;
                }

                if (closed)
                    return true;

                // The outline goes through the vertex straight, the layer keeps its shape without it
                const Point& first = mPositions[mRing.front()];
                const Point& last = mPositions[mRing.back()];
                const Point& middle = mPositions[vertex];
                double length = sqrt((last.x - first.x)*(last.x - first.x) + (last.y - first.y)*(last.y - first.y));
                double along = (middle.x - first.x)*(last.x - first.x) + (middle.y - first.y)*(last.y - first.y);
                return length > kMaxOutlineShift && along > 0.0 && along < length*length &&
                    fabs(Cross(first, last, middle)) <= kMaxOutlineShift*length;
            }

            // True when each color channel of the samples is within the tolerance of one plane
            bool AreOnColorPlane() const
            {
                int count = (int)mSamples.size();

                double centerX = 0, centerY = 0;
                for (const Sample& sample : mSamples)
                {
                    centerX += sample.position.x/count;
                    centerY += sample.position.y/count;
                }

                double xx = 0, xy = 0, yy = 0;
                for (const Sample& sample : mSamples)
                {
                    double x = sample.position.x - centerX, y = sample.position.y - centerY;
                    xx += x*x;
                    xy += x*y;
                    yy += y*y;
                }

                double determinant = xx*yy - xy*xy;

                for (int channel = 0; channel < 4; channel++)
                {
                    double mean = 0, minimum = 255, maximum = 0;
                    for (const Sample& sample : mSamples)
                    {
                        mean += sample.channels[channel]/count;
                        minimum = Math::Min(minimum, (double)sample.channels[channel]);
                        maximum = Math::Max(maximum, (double)sample.channels[channel]);
                    }

                    if (maximum - minimum <= mColorTolerance)
                        continue;

                    if (fabs(determinant) < 1e-12)
                        return false;

                    double xc = 0, yc = 0;
                    for (const Sample& sample : mSamples)
                    {
                        double value = sample.channels[channel] - mean;
                        xc += (sample.position.x - centerX)*value;
                        yc += (sample.position.y - centerY)*value;
                    }

                    double slopeX = (xc*yy - yc*xy)/determinant, slopeY = (yc*xx - xc*xy)/determinant;

                    // The plane is moved to the middle of the residuals
                    double lowest = 1e9, highest = -1e9;
                    for (const Sample& sample : mSamples)
                    {
                        double residual = sample.channels[channel] - mean - slopeX*(sample.position.x - centerX) -
                            slopeY*(sample.position.y - centerY);
                        lowest = Math::Min(lowest, residual);
                        highest = Math::Max(highest, residual);
                    }

                    if ((highest - lowest)*0.5 > mColorTolerance)
                        return false;
                }

                return true;
            }

            // Cuts ears of the counter clockwise mPolygon into mFan, 3 vertices per triangle
            bool Triangulate()
            {
                mFan.clear();
                if (mPolygon.size() < 3)
                    return false;

                while (mPolygon.size() > 3)
                {
                    int count = (int)mPolygon.size();
                    int best = -1;
                    double bestQuality = 0;

                    for (int i = 0; i < count; i++)
                    {
                        const Point& previous = mPositions[mPolygon[(i + count - 1)%count]];
                        const Point& tip = mPositions[mPolygon[i]];
                        const Point& next = mPositions[mPolygon[(i + 1)%count]];

                        double area = Cross(previous, tip, next);
                        if (area < kMinArea)
                            continue;

                        bool empty = true;
                        for (int j = 0; j < count && empty; j++)
                        {
                            if (j == i || j == (i + count - 1)%count || j == (i + 1)%count)
                                continue;

                            const Point& other = mPositions[mPolygon[j]];
                            empty = Cross(previous, tip, other) < -kInsideEpsilon || Cross(tip, next, other) < -kInsideEpsilon ||
                                Cross(next, previous, other) < -kInsideEpsilon;
                        }

                        if (!empty)
                            continue;

                        // Thin ears are left for later: the longest side against the area
                        double longest = Math::Max(Math::Max(SquareDistance(previous, tip), SquareDistance(tip, next)),
                                                   SquareDistance(next, previous));
                        double quality = area/longest;
                        if (quality > bestQuality)
                        {
                            bestQuality = quality;
                            best = i;
                        }
                    }

                    if (best < 0)
                        return false;

                    mFan.push_back(mPolygon[(best + count - 1)%count]);
                    mFan.push_back(mPolygon[best]);
                    mFan.push_back(mPolygon[(best + 1)%count]);
                    mPolygon.erase(mPolygon.begin() + best);
                }

                if (Cross(mPositions[mPolygon[0]], mPositions[mPolygon[1]], mPositions[mPolygon[2]]) < kMinArea)
                    return false;

                mFan.insert(mFan.end(), mPolygon.begin(), mPolygon.end());
                return true;
            }

            static double SquareDistance(const Point& a, const Point& b)
            {
                return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
            }

            bool RemoveVertex(int vertex)
            {
                bool closed = false;
                if (!GetRing(vertex, closed))
                    return false;

                mSamples.clear();
                for (int ringVertex : mRing)
                    mSamples.push_back(GetSample(ringVertex));

                mSamples.push_back(GetSample(vertex));

                for (int i = 0; i < mFanSize; i++)
                {
                    const std::vector<Sample>& removed = mTriangles[mFanTriangles[i]].removed;
                    mSamples.insert(mSamples.end(), removed.begin(), removed.end());
                }

                if (!AreOnColorPlane())
                    return false;

                mPolygon = mRing;
                if (!Triangulate())
                    return false;

                double previousArea = 0, area = 0;
                for (int i = 0; i < mFanSize; i++)
                {
                    const int* vertices = mTriangles[mFanTriangles[i]].vertices;
                    previousArea += fabs(Cross(mPositions[vertices[0]], mPositions[vertices[1]], mPositions[vertices[2]]));
                }

                for (size_t i = 0; i < mFan.size(); i += 3)
                    area += Cross(mPositions[mFan[i]], mPositions[mFan[i + 1]], mPositions[mFan[i + 2]]);

                double outlineShift = closed ? 0.0 :
                    fabs(Cross(mPositions[mRing.front()], mPositions[mRing.back()], mPositions[vertex]));
                if (fabs(area - previousArea) > 1e-6*previousArea + kMinArea + outlineShift)
                    return false;

                for (int i = 0; i < mFanSize; i++)
                    RemoveTriangle(mFanTriangles[i]);

                mCreated.clear();
                for (size_t i = 0; i < mFan.size(); i += 3)
                    mCreated.push_back(AddTriangle(&mFan[i]));

                // Samples of the removed vertices go to the triangles they are in now
                for (size_t i = mRing.size(); i < mSamples.size(); i++)
                {
                    int nearest = mCreated[0];
                    double nearestInside = -1e18;

                    for (int created : mCreated)
                    {
                        const int* vertices = mTriangles[created].vertices;
                        double inside = Math::Min(Math::Min(
                            Cross(mPositions[vertices[0]], mPositions[vertices[1]], mSamples[i].position),
                            Cross(mPositions[vertices[1]], mPositions[vertices[2]], mSamples[i].position)),
                            Cross(mPositions[vertices[2]], mPositions[vertices[0]], mSamples[i].position));

                        if (inside > nearestInside)
                        {
                            nearestInside = inside;
                            nearest = created;
                        }
                    }

                    mTriangles[nearest].removed.push_back(mSamples[i]);
                }

                return true;
            }
        };
    }

    void VectorMeshSimplifier::Simplify(VectorMesh& mesh, const Vector<int>& layers, float colorTolerance)
    {
        if (mesh.indexes.IsEmpty() || layers.IsEmpty())
            return;

        std::vector<int> localIndexes(mesh.positions.size(), -1);

        Vector<VertexIndex> indexes;
        indexes.reserve(mesh.indexes.size());

        int indexesCount = (int)mesh.indexes.size();
        int firstLayer = Math::Clamp(layers[0], 0, indexesCount);
        for (int i = 0; i < firstLayer; i++)
            indexes.Add(mesh.indexes[i]);

        for (int i = 0; i < layers.Count(); i++)
        {
            int begin = Math::Clamp(layers[i], 0, indexesCount);
            int end = i + 1 < layers.Count() ? Math::Clamp(layers[i + 1], begin, indexesCount) : indexesCount;
            if (end <= begin)
                continue;

            Layer layer(mesh, mesh.indexes.Data() + begin, end - begin, colorTolerance, localIndexes);
            layer.Simplify();
            layer.GetIndexes(indexes);
        }

        mesh.indexes = indexes;
        RemoveUnusedVertices(mesh);
    }

    void VectorMeshSimplifier::RemoveUnusedVertices(VectorMesh& mesh)
    {
        const VertexIndex unused = (VertexIndex)-1;
        std::vector<VertexIndex> placed(mesh.positions.size(), unused);

        Vector<Vec2F> positions;
        Vector<Color32Bit> colors;
        positions.reserve(mesh.positions.size());
        colors.reserve(mesh.colors.size());

        for (VertexIndex& index : mesh.indexes)
        {
            if (placed[index] == unused)
            {
                placed[index] = (VertexIndex)positions.size();
                positions.Add(mesh.positions[index]);
                colors.Add(mesh.colors[index]);
            }

            index = placed[index];
        }

        mesh.positions = positions;
        mesh.colors = colors;
    }
}
