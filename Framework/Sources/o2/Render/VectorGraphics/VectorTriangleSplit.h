#pragma once

namespace o2
{
    namespace VectorDetails
    {
        // Splits triangles list (3 vertices per triangle) where the value equals threshold, keeping their order.
        // Vertices within epsilon of the threshold are on the cut: no slivers at them
        template<typename _container, typename _get_value, typename _lerp>
        void SplitTrianglesByValue(_container& triangles, double threshold, _get_value&& getValue, _lerp&& lerp,
                                   double epsilon = 1e-7)
        {
            typedef typename _container::value_type VertexType;

            auto side = [&](const VertexType& vertex)
            {
                double delta = (double)getValue(vertex) - threshold;
                return delta > epsilon ? 1 : (delta < -epsilon ? -1 : 0);
            };

            // Ordered ends make the cut point identical for both triangles sharing the edge
            auto cut = [&](const VertexType& a, const VertexType& b)
            {
                bool ordered = a.x < b.x || (a.x == b.x && a.y <= b.y);
                const VertexType& first = ordered ? a : b;
                const VertexType& second = ordered ? b : a;
                double valueFirst = getValue(first), valueSecond = getValue(second);
                return lerp(first, second, (threshold - valueFirst)/(valueSecond - valueFirst));
            };

            _container result;
            result.reserve(triangles.size());

            for (size_t i = 0; i + 2 < triangles.size(); i += 3)
            {
                VertexType v[3] = { triangles[i], triangles[i + 1], triangles[i + 2] };
                int s[3] = { side(v[0]), side(v[1]), side(v[2]) };

                bool hasPositive = s[0] > 0 || s[1] > 0 || s[2] > 0;
                bool hasNegative = s[0] < 0 || s[1] < 0 || s[2] < 0;
                if (!hasPositive || !hasNegative)
                {
                    result.insert(result.end(), v, v + 3);
                    continue;
                }

                int lone = 0;
                for (int k = 0; k < 3; k++)
                {
                    int next = s[(k + 1)%3], prev = s[(k + 2)%3];
                    if (s[k] == 0 || (next == prev && next != s[k]))
                        lone = k;
                }

                const VertexType a = v[lone], b = v[(lone + 1)%3], c = v[(lone + 2)%3];
                if (s[lone] == 0)
                {
                    VertexType middle = cut(b, c);
                    for (const VertexType& vertex : { a, b, middle, a, middle, c })
                        result.push_back(vertex);
                }
                else
                {
                    VertexType ab = cut(a, b), ac = cut(a, c);
                    for (const VertexType& vertex : { a, ab, ac, ab, b, c, ab, c, ac })
                        result.push_back(vertex);
                }
            }

            triangles.swap(result);
        }
    }
}
