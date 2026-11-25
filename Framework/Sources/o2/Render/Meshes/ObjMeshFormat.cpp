#include "o2/stdafx.h"
#include "ObjMeshFormat.h"

#include <cstdlib>
#include <unordered_map>

namespace o2
{
    namespace
    {
        const char* SkipSpaces(const char* p)
        {
            while (*p == ' ' || *p == '\t')
                p++;

            return p;
        }

        const char* SkipLine(const char* p)
        {
            while (*p && *p != '\n')
                p++;

            return *p ? p + 1 : p;
        }

        bool ResolveIndex(int index, int count, int& result)
        {
            if (index > 0 && index <= count)
                result = index - 1;
            else if (index < 0 && -index <= count)
                result = count + index;
            else
                return false;

            return true;
        }

        struct FaceRef
        {
            int position = -1;
            int uv = -1;
            int normal = -1;
        };

        bool ParseFaceRef(const char*& p, int positionsCount, int uvsCount, int normalsCount, FaceRef& result)
        {
            char* end = nullptr;
            int v = (int)strtol(p, &end, 10);
            if (end == p || !ResolveIndex(v, positionsCount, result.position))
                return false;

            p = end;
            if (*p != '/')
                return true;

            p++;
            if (*p != '/')
            {
                int vt = (int)strtol(p, &end, 10);
                if (end == p || !ResolveIndex(vt, uvsCount, result.uv))
                    return false;

                p = end;
            }

            if (*p != '/')
                return true;

            p++;
            int vn = (int)strtol(p, &end, 10);
            if (end == p || !ResolveIndex(vn, normalsCount, result.normal))
                return false;

            p = end;
            return true;
        }

        void Fail(String* errorMessage, const String& message)
        {
            if (errorMessage)
                *errorMessage = message;
        }
    }

    bool ObjMeshFormat::Parse(const String& text, Mesh3DData& output, String* errorMessage /*= nullptr*/)
    {
        std::vector<Vec3F> rawPositions;
        std::vector<Vec2F> rawUVs;
        std::vector<Vec3F> rawNormals;

        std::vector<Vec3F> positions;
        std::vector<Vec2F> uvs;
        std::vector<Vec3F> normals;
        std::vector<UInt> indices;

        std::unordered_map<UInt64, UInt> uniqueRefs;
        bool hasNormals = false;
        bool hasUVs = false;
        int lineNumber = 0;

        auto getUniqueIndex = [&](const FaceRef& ref)
        {
            UInt64 key = ((UInt64)(ref.position + 1) << 42) | ((UInt64)(ref.uv + 1) << 21) | (UInt64)(ref.normal + 1);
            auto found = uniqueRefs.find(key);
            if (found != uniqueRefs.end())
                return found->second;

            UInt index = (UInt)positions.size();
            positions.push_back(rawPositions[ref.position]);
            uvs.push_back(ref.uv >= 0 ? rawUVs[ref.uv] : Vec2F());
            normals.push_back(ref.normal >= 0 ? rawNormals[ref.normal] : Vec3F());

            hasUVs |= ref.uv >= 0;
            hasNormals |= ref.normal >= 0;

            uniqueRefs[key] = index;
            return index;
        };

        for (const char* p = text.Data(); *p; p = SkipLine(p))
        {
            lineNumber++;
            p = SkipSpaces(p);

            if (p[0] == 'v' && p[1] == ' ')
            {
                char* end = nullptr;
                float x = strtof(p + 2, &end);
                float y = strtof(end, &end);
                float z = strtof(end, &end);
                rawPositions.push_back(Vec3F(x, y, z));
            }
            else if (p[0] == 'v' && p[1] == 't' && p[2] == ' ')
            {
                char* end = nullptr;
                float u = strtof(p + 3, &end);
                float v = strtof(end, &end);
                rawUVs.push_back(Vec2F(u, v));
            }
            else if (p[0] == 'v' && p[1] == 'n' && p[2] == ' ')
            {
                char* end = nullptr;
                float x = strtof(p + 3, &end);
                float y = strtof(end, &end);
                float z = strtof(end, &end);
                rawNormals.push_back(Vec3F(x, y, z));
            }
            else if (p[0] == 'f' && p[1] == ' ')
            {
                FaceRef faceRefs[3];
                int refsCount = 0;

                const char* cursor = p + 2;
                while (true)
                {
                    cursor = SkipSpaces(cursor);
                    if (*cursor == 0 || *cursor == '\n' || *cursor == '\r' || *cursor == '#')
                        break;

                    FaceRef ref;
                    if (!ParseFaceRef(cursor, (int)rawPositions.size(), (int)rawUVs.size(), (int)rawNormals.size(), ref))
                    {
                        Fail(errorMessage, String("Invalid face reference at line ") + (String)lineNumber);
                        return false;
                    }

                    if (refsCount < 2)
                        faceRefs[refsCount] = ref;
                    else
                    {
                        faceRefs[2] = ref;
                        indices.push_back(getUniqueIndex(faceRefs[0]));
                        indices.push_back(getUniqueIndex(faceRefs[1]));
                        indices.push_back(getUniqueIndex(faceRefs[2]));
                        faceRefs[1] = faceRefs[2];
                    }

                    refsCount++;
                }

                if (refsCount < 3)
                {
                    Fail(errorMessage, String("Face with less than 3 vertices at line ") + (String)lineNumber);
                    return false;
                }
            }
        }

        if (positions.empty() || indices.empty())
        {
            Fail(errorMessage, "No geometry found");
            return false;
        }

        if (!hasNormals)
        {
            for (size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                Vec3F a = positions[indices[i]], b = positions[indices[i + 1]], c = positions[indices[i + 2]];
                Vec3F faceNormal = (b - a).Cross(c - a);

                normals[indices[i]] += faceNormal;
                normals[indices[i + 1]] += faceNormal;
                normals[indices[i + 2]] += faceNormal;
            }

            for (auto& normal : normals)
                normal = normal.Length() > FLT_EPSILON ? normal.Normalized() : Vec3F(0.0f, 0.0f, 1.0f);
        }

        output.positions.Clear(); output.uvs.Clear(); output.normals.Clear(); output.indices.Clear();
        for (auto& v : positions) output.positions.Add(v);
        for (auto& v : uvs) output.uvs.Add(v);
        for (auto& v : normals) output.normals.Add(v);
        for (auto& i : indices) output.indices.Add(i);

        return true;
    }

    String ObjMeshFormat::Write(const Mesh3DData& data)
    {
        String result;
        result.Reserve((data.positions.Count()*3 + data.indices.Count())*24);

        for (auto& v : data.positions)
            result += String::Format("v %f %f %f\n", v.x, v.y, v.z);

        for (auto& v : data.uvs)
            result += String::Format("vt %f %f\n", v.x, v.y);

        for (auto& v : data.normals)
            result += String::Format("vn %f %f %f\n", v.x, v.y, v.z);

        for (int i = 0; i + 2 < data.indices.Count(); i += 3)
        {
            int a = data.indices[i] + 1, b = data.indices[i + 1] + 1, c = data.indices[i + 2] + 1;
            result += String::Format("f %i/%i/%i %i/%i/%i %i/%i/%i\n", a, a, a, b, b, b, c, c, c);
        }

        return result;
    }
}
