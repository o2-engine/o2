#include "o2/stdafx.h"
#include "SkinnedModelFormat.h"

#include "o2/Utils/Serialization/DataValue.h"

namespace o2
{
    int SkinnedModelData::FindAnimation(const String& name) const
    {
        return animations.IndexOf([&](auto& x) { return x.name == name; });
    }

    bool SkinnedModelData::GetBounds(AABB& bounds) const
    {
        if (positions.IsEmpty())
            return false;

        bounds = AABB::Bound(const_cast<Vector<Vec3F>&>(positions).Data(), positions.Count());
        return true;
    }

    namespace
    {
        // MSVC won't route a C-style cast to a class type through DataValue's template
        // conversion operator, so pull the string out via the explicit Get overload.
        String AsString(const DataValue& value)
        {
            String result;
            value.Get(result);
            return result;
        }

        template<typename T>
        int FindKeyFrame(const Vector<float>& times, float time, T& coef)
        {
            if (time <= times[0])
            {
                coef = T(0);
                return 0;
            }

            for (int i = 1; i < times.Count(); i++)
            {
                if (time <= times[i])
                {
                    float span = times[i] - times[i - 1];
                    coef = span > FLT_EPSILON ? (time - times[i - 1])/span : T(0);
                    return i - 1;
                }
            }

            coef = T(0);
            return times.Count() - 1;
        }
    }

    void SkinnedModelData::EvaluateNodeTransforms(int animation, float time, Vector<Mat4>& outLocals) const
    {
        Vector<Vec3F> localPositions;
        Vector<Quat>  localRotations;
        Vector<Vec3F> localScales;

        localPositions.Reserve(nodes.Count());
        localRotations.Reserve(nodes.Count());
        localScales.Reserve(nodes.Count());

        for (auto& node : nodes)
        {
            localPositions.Add(node.position);
            localRotations.Add(node.rotation);
            localScales.Add(node.scale);
        }

        if (animation >= 0 && animation < animations.Count())
        {
            for (auto& channel : animations[animation].channels)
            {
                if (channel.node < 0 || channel.node >= nodes.Count() || channel.times.IsEmpty())
                    continue;

                float coef;
                int key = FindKeyFrame(channel.times, time, coef);
                int nextKey = Math::Min(key + 1, channel.times.Count() - 1);

                if (channel.step)
                {
                    // STEP holds the left key of the interval; the exact key time takes that key
                    if (coef > 0.9999f)
                        key = nextKey;

                    coef = 0.0f;
                }

                if (channel.path == AnimationChannel::Path::Rotation)
                {
                    localRotations[channel.node] = Quat::Slerp(channel.rotations[key],
                                                               channel.rotations[nextKey], coef).Normalized();
                }
                else
                {
                    Vec3F value = Math::Lerp(channel.vectors[key], channel.vectors[nextKey], coef);
                    if (channel.path == AnimationChannel::Path::Translation)
                        localPositions[channel.node] = value;
                    else
                        localScales[channel.node] = value;
                }
            }
        }

        outLocals.Clear();
        outLocals.Reserve(nodes.Count());
        for (int i = 0; i < nodes.Count(); i++)
            outLocals.Add(Mat4::TRS(localPositions[i], localRotations[i], localScales[i]));
    }

    void SkinnedModelData::EvaluateGlobalTransforms(const Vector<Mat4>& locals, Vector<Mat4>& outGlobals) const
    {
        outGlobals.Resize(locals.Count());

        // Nodes are stored in glTF order where parents may follow children: resolve recursively with memoization
        Vector<bool> resolved;
        resolved.Resize(locals.Count());
        for (int i = 0; i < resolved.Count(); i++)
            resolved[i] = false;

        Function<void(int)> resolve = [&](int index)
        {
            if (resolved[index])
                return;

            int parent = nodes[index].parent;
            if (parent >= 0 && parent < locals.Count())
            {
                resolve(parent);
                outGlobals[index] = outGlobals[parent]*locals[index];
            }
            else
                outGlobals[index] = locals[index];

            resolved[index] = true;
        };

        for (int i = 0; i < locals.Count(); i++)
            resolve(i);
    }

    void SkinnedModelData::EvaluateJointsPalette(int animation, float time, Vector<Mat4>& outPalette) const
    {
        Vector<Mat4> locals, globals;
        EvaluateNodeTransforms(animation, time, locals);
        EvaluateGlobalTransforms(locals, globals);

        outPalette.Clear();
        outPalette.Reserve(joints.Count());

        for (int i = 0; i < joints.Count(); i++)
        {
            Mat4 inverseBind = i < inverseBindMatrices.Count() ? inverseBindMatrices[i] : Mat4::Identity();
            int node = joints[i];
            Mat4 global = node >= 0 && node < globals.Count() ? globals[node] : Mat4::Identity();
            outPalette.Add(global*inverseBind);
        }
    }

    void SkinnedModelData::SkinVertices(const Vector<Mat4>& palette, Vector<Vec3F>& outPositions,
                                        Vector<Vec3F>& outNormals) const
    {
        outPositions.Resize(positions.Count());

        bool hasSourceNormals = normals.Count() == positions.Count();
        if (hasSourceNormals)
            outNormals.Resize(positions.Count());

        for (int i = 0; i < positions.Count(); i++)
        {
            if (i >= influences.Count())
            {
                outPositions[i] = positions[i];
                if (hasSourceNormals)
                    outNormals[i] = normals[i];

                continue;
            }

            const VertexInfluence& influence = influences[i];

            Vec3F skinnedPosition;
            Vec3F skinnedNormal;
            float weightsSum = 0.0f;

            for (int j = 0; j < influencesPerVertex; j++)
            {
                float weight = influence.weights[j];
                if (weight <= 0.0f)
                    continue;

                int joint = influence.joints[j];
                if (joint < 0 || joint >= palette.Count())
                    continue;

                const Mat4& matrix = palette[joint];
                skinnedPosition += matrix.TransformPoint(positions[i])*weight;
                if (hasSourceNormals)
                    skinnedNormal += matrix.TransformDirection(normals[i])*weight;

                weightsSum += weight;
            }

            if (weightsSum <= FLT_EPSILON)
            {
                skinnedPosition = positions[i];
                if (hasSourceNormals)
                    skinnedNormal = normals[i];
            }

            outPositions[i] = skinnedPosition;
            if (hasSourceNormals)
                outNormals[i] = skinnedNormal.Normalized();
        }

        if (!hasSourceNormals)
            ComputeSmoothNormals(outPositions, indices, outNormals);
    }

    void SkinnedModelData::ComputeSmoothNormals(const Vector<Vec3F>& positions, const Vector<UInt>& indices,
                                                Vector<Vec3F>& outNormals)
    {
        outNormals.Resize(positions.Count());
        for (int i = 0; i < outNormals.Count(); i++)
            outNormals[i] = Vec3F();

        for (int i = 0; i + 2 < indices.Count(); i += 3)
        {
            UInt a = indices[i], b = indices[i + 1], c = indices[i + 2];
            if (a >= (UInt)positions.Count() || b >= (UInt)positions.Count() || c >= (UInt)positions.Count())
                continue;

            Vec3F faceNormal = (positions[b] - positions[a]).Cross(positions[c] - positions[a]);
            outNormals[a] += faceNormal;
            outNormals[b] += faceNormal;
            outNormals[c] += faceNormal;
        }

        for (int i = 0; i < outNormals.Count(); i++)
        {
            if (outNormals[i].SqrLength() > FLT_EPSILON)
                outNormals[i] = outNormals[i].Normalized();
            else
                outNormals[i] = Vec3F(0.0f, 0.0f, 1.0f);
        }
    }

    namespace GlbModelFormat
    {
        namespace
        {
            constexpr UInt glbMagic = 0x46546C67;     // "glTF"
            constexpr UInt jsonChunkType = 0x4E4F534A; // "JSON"
            constexpr UInt binChunkType = 0x004E4942;  // "BIN"

            enum class ComponentType
            {
                Byte = 5120, UnsignedByte = 5121, Short = 5122, UnsignedShort = 5123,
                UnsignedInt = 5125, Float = 5126
            };

            struct BinaryReader
            {
                const UInt8* data = nullptr;
                UInt size = 0;
            };

            // Resolved accessor view over the binary chunk
            struct AccessorView
            {
                const UInt8* data = nullptr; // First element pointer
                int count = 0;               // Elements count
                int components = 0;          // Components per element
                int componentType = 0;       // glTF component type
                int stride = 0;              // Bytes between elements

                bool IsValid() const { return data != nullptr && count > 0; }

                float ReadComponent(int element, int component) const
                {
                    const UInt8* ptr = data + element*stride;
                    switch ((ComponentType)componentType)
                    {
                        case ComponentType::Float:         return ((const float*)ptr)[component];
                        case ComponentType::UnsignedShort: return (float)((const UInt16*)ptr)[component];
                        case ComponentType::UnsignedByte:  return (float)ptr[component];
                        case ComponentType::UnsignedInt:   return (float)((const UInt32*)ptr)[component];
                        case ComponentType::Short:         return (float)((const short*)ptr)[component];
                        case ComponentType::Byte:          return (float)((const char*)ptr)[component];
                        default:                           return 0.0f;
                    }
                }

                UInt ReadIndex(int element) const
                {
                    const UInt8* ptr = data + element*stride;
                    switch ((ComponentType)componentType)
                    {
                        case ComponentType::UnsignedShort: return ((const UInt16*)ptr)[0];
                        case ComponentType::UnsignedInt:   return ((const UInt32*)ptr)[0];
                        case ComponentType::UnsignedByte:  return ptr[0];
                        default:                           return 0;
                    }
                }
            };

            int ComponentsOfType(const String& type)
            {
                if (type == "SCALAR") return 1;
                if (type == "VEC2") return 2;
                if (type == "VEC3") return 3;
                if (type == "VEC4") return 4;
                if (type == "MAT4") return 16;
                return 0;
            }

            int ComponentSize(int componentType)
            {
                switch ((ComponentType)componentType)
                {
                    case ComponentType::Byte:
                    case ComponentType::UnsignedByte: return 1;
                    case ComponentType::Short:
                    case ComponentType::UnsignedShort: return 2;
                    case ComponentType::UnsignedInt:
                    case ComponentType::Float: return 4;
                    default: return 0;
                }
            }

            bool ResolveAccessor(const DataValue& json, const BinaryReader& binary, int accessorIndex,
                                 AccessorView& outView, String* errorMessage)
            {
                auto accessorsNode = json.FindMember("accessors");
                auto bufferViewsNode = json.FindMember("bufferViews");
                if (!accessorsNode || accessorIndex < 0 || accessorIndex >= accessorsNode->GetElementsCount() ||
                    !bufferViewsNode)
                {
                    if (errorMessage)
                        *errorMessage = "Invalid accessor index";

                    return false;
                }

                const DataValue& accessor = accessorsNode->GetElement(accessorIndex);
                if (accessor.FindMember("sparse"))
                {
                    if (errorMessage)
                        *errorMessage = "Sparse accessors are not supported";

                    return false;
                }

                auto bufferViewNode = accessor.FindMember("bufferView");
                if (!bufferViewNode)
                {
                    if (errorMessage)
                        *errorMessage = "Accessor without buffer view is not supported";

                    return false;
                }

                int bufferViewIndex = (int)*bufferViewNode;
                if (bufferViewIndex < 0 || bufferViewIndex >= bufferViewsNode->GetElementsCount())
                {
                    if (errorMessage)
                        *errorMessage = "Invalid buffer view index";

                    return false;
                }

                const DataValue& bufferView = bufferViewsNode->GetElement(bufferViewIndex);

                int viewOffset = 0;
                if (auto node = bufferView.FindMember("byteOffset"))
                    viewOffset = (int)*node;

                int viewLength = 0;
                if (auto node = bufferView.FindMember("byteLength"))
                    viewLength = (int)*node;

                int accessorOffset = 0;
                if (auto node = accessor.FindMember("byteOffset"))
                    accessorOffset = (int)*node;

                AccessorView view;
                view.count = (int)accessor.GetMember("count");
                view.componentType = (int)accessor.GetMember("componentType");
                view.components = ComponentsOfType(AsString(accessor.GetMember("type")));

                int elementSize = view.components*ComponentSize(view.componentType);
                view.stride = elementSize;
                if (auto strideNode = bufferView.FindMember("byteStride"))
                    view.stride = (int)*strideNode;

                if (view.components == 0 || elementSize == 0 || view.count <= 0)
                {
                    if (errorMessage)
                        *errorMessage = "Unsupported accessor format";

                    return false;
                }

                UInt end = (UInt)(viewOffset + accessorOffset) + (UInt)(view.count - 1)*view.stride + elementSize;
                if (end > binary.size)
                {
                    if (errorMessage)
                        *errorMessage = "Accessor data is out of binary chunk bounds";

                    return false;
                }

                view.data = binary.data + viewOffset + accessorOffset;
                outView = view;
                return true;
            }

            bool ReadVec3Array(const DataValue& json, const BinaryReader& binary, int accessor,
                               Vector<Vec3F>& out, String* errorMessage)
            {
                AccessorView view;
                if (!ResolveAccessor(json, binary, accessor, view, errorMessage) || view.components < 3)
                    return false;

                out.Clear();
                out.Reserve(view.count);
                for (int i = 0; i < view.count; i++)
                    out.Add(Vec3F(view.ReadComponent(i, 0), view.ReadComponent(i, 1), view.ReadComponent(i, 2)));

                return true;
            }

            void ReadNodeTransform(const DataValue& node, SkinnedModelData::Node& outNode)
            {
                if (auto matrixNode = node.FindMember("matrix"))
                {
                    Mat4 matrix;
                    for (int i = 0; i < 16 && i < matrixNode->GetElementsCount(); i++)
                        matrix.m[i] = (float)matrixNode->GetElement(i);

                    matrix.Decompose(outNode.position, outNode.rotation, outNode.scale);
                    return;
                }

                if (auto translationNode = node.FindMember("translation"))
                {
                    outNode.position = Vec3F((float)translationNode->GetElement(0),
                                             (float)translationNode->GetElement(1),
                                             (float)translationNode->GetElement(2));
                }

                if (auto rotationNode = node.FindMember("rotation"))
                {
                    outNode.rotation = Quat((float)rotationNode->GetElement(0), (float)rotationNode->GetElement(1),
                                            (float)rotationNode->GetElement(2), (float)rotationNode->GetElement(3));
                }

                if (auto scaleNode = node.FindMember("scale"))
                {
                    outNode.scale = Vec3F((float)scaleNode->GetElement(0), (float)scaleNode->GetElement(1),
                                          (float)scaleNode->GetElement(2));
                }
            }

            bool ParseGeometry(const DataValue& json, const BinaryReader& binary, SkinnedModelData& model,
                               String* errorMessage)
            {
                auto meshesNode = json.FindMember("meshes");
                if (!meshesNode || meshesNode->GetElementsCount() == 0)
                {
                    if (errorMessage)
                        *errorMessage = "No meshes in the model";

                    return false;
                }

                const DataValue& primitive = meshesNode->GetElement(0).GetMember("primitives").GetElement(0);

                if (auto modeNode = primitive.FindMember("mode"))
                {
                    if ((int)*modeNode != 4)
                    {
                        if (errorMessage)
                            *errorMessage = "Only triangles primitives are supported";

                        return false;
                    }
                }

                const DataValue& attributes = primitive.GetMember("attributes");

                auto positionNode = attributes.FindMember("POSITION");
                if (!positionNode || !ReadVec3Array(json, binary, (int)*positionNode, model.positions, errorMessage))
                    return false;

                if (auto normalNode = attributes.FindMember("NORMAL"))
                {
                    if (!ReadVec3Array(json, binary, (int)*normalNode, model.normals, errorMessage))
                        return false;
                }

                if (auto uvNode = attributes.FindMember("TEXCOORD_0"))
                {
                    AccessorView view;
                    if (!ResolveAccessor(json, binary, (int)*uvNode, view, errorMessage))
                        return false;

                    // glTF V origin is the texture top; the engine mesh fill expects bottom-origin UVs
                    model.uvs.Reserve(view.count);
                    for (int i = 0; i < view.count; i++)
                        model.uvs.Add(Vec2F(view.ReadComponent(i, 0), 1.0f - view.ReadComponent(i, 1)));
                }

                auto jointsNode = attributes.FindMember("JOINTS_0");
                auto weightsNode = attributes.FindMember("WEIGHTS_0");
                if (jointsNode && weightsNode)
                {
                    AccessorView jointsView, weightsView;
                    if (!ResolveAccessor(json, binary, (int)*jointsNode, jointsView, errorMessage) ||
                        !ResolveAccessor(json, binary, (int)*weightsNode, weightsView, errorMessage))
                    {
                        return false;
                    }

                    bool normalizedWeights = weightsView.componentType != (int)ComponentType::Float;
                    float weightsScale = 1.0f;
                    if (weightsView.componentType == (int)ComponentType::UnsignedByte)
                        weightsScale = 1.0f/255.0f;
                    else if (weightsView.componentType == (int)ComponentType::UnsignedShort)
                        weightsScale = 1.0f/65535.0f;

                    model.influences.Reserve(jointsView.count);
                    for (int i = 0; i < jointsView.count; i++)
                    {
                        SkinnedModelData::VertexInfluence influence;
                        for (int j = 0; j < SkinnedModelData::influencesPerVertex; j++)
                        {
                            influence.joints[j] = (int)jointsView.ReadComponent(i, j);
                            influence.weights[j] = weightsView.ReadComponent(i, j)*
                                (normalizedWeights ? weightsScale : 1.0f);
                        }

                        model.influences.Add(influence);
                    }
                }

                if (auto indicesNode = primitive.FindMember("indices"))
                {
                    AccessorView view;
                    if (!ResolveAccessor(json, binary, (int)*indicesNode, view, errorMessage))
                        return false;

                    model.indices.Reserve(view.count);
                    for (int i = 0; i < view.count; i++)
                        model.indices.Add(view.ReadIndex(i));
                }
                else
                {
                    // Non-indexed primitive: sequential triangles
                    model.indices.Reserve(model.positions.Count());
                    for (int i = 0; i < model.positions.Count(); i++)
                        model.indices.Add((UInt)i);
                }

                return true;
            }

            void ParseNodes(const DataValue& json, SkinnedModelData& model)
            {
                auto nodesNode = json.FindMember("nodes");
                if (!nodesNode)
                    return;

                model.nodes.Reserve(nodesNode->GetElementsCount());
                for (int i = 0; i < nodesNode->GetElementsCount(); i++)
                {
                    const DataValue& nodeValue = nodesNode->GetElement(i);

                    SkinnedModelData::Node node;
                    if (auto nameNode = nodeValue.FindMember("name"))
                        node.name = AsString(*nameNode);

                    ReadNodeTransform(nodeValue, node);
                    model.nodes.Add(node);
                }

                for (int i = 0; i < nodesNode->GetElementsCount(); i++)
                {
                    if (auto childrenNode = nodesNode->GetElement(i).FindMember("children"))
                    {
                        for (int c = 0; c < childrenNode->GetElementsCount(); c++)
                        {
                            int child = (int)childrenNode->GetElement(c);
                            if (child >= 0 && child < model.nodes.Count())
                                model.nodes[child].parent = i;
                        }
                    }
                }
            }

            bool ParseSkin(const DataValue& json, const BinaryReader& binary, SkinnedModelData& model,
                           String* errorMessage)
            {
                auto skinsNode = json.FindMember("skins");
                if (!skinsNode || skinsNode->GetElementsCount() == 0)
                    return true;

                const DataValue& skin = skinsNode->GetElement(0);

                const DataValue& jointsNode = skin.GetMember("joints");
                model.joints.Reserve(jointsNode.GetElementsCount());
                for (int i = 0; i < jointsNode.GetElementsCount(); i++)
                    model.joints.Add((int)jointsNode.GetElement(i));

                if (auto ibmNode = skin.FindMember("inverseBindMatrices"))
                {
                    AccessorView view;
                    if (!ResolveAccessor(json, binary, (int)*ibmNode, view, errorMessage) || view.components != 16)
                        return false;

                    model.inverseBindMatrices.Reserve(view.count);
                    for (int i = 0; i < view.count; i++)
                    {
                        Mat4 matrix;
                        for (int c = 0; c < 16; c++)
                            matrix.m[c] = view.ReadComponent(i, c);

                        model.inverseBindMatrices.Add(matrix);
                    }
                }

                return true;
            }

            bool ParseAnimations(const DataValue& json, const BinaryReader& binary, SkinnedModelData& model,
                                 String* errorMessage)
            {
                auto animationsNode = json.FindMember("animations");
                if (!animationsNode)
                    return true;

                for (int a = 0; a < animationsNode->GetElementsCount(); a++)
                {
                    const DataValue& animationNode = animationsNode->GetElement(a);

                    SkinnedModelData::AnimationClip clip;
                    if (auto nameNode = animationNode.FindMember("name"))
                        clip.name = AsString(*nameNode);

                    const DataValue& samplersNode = animationNode.GetMember("samplers");
                    const DataValue& channelsNode = animationNode.GetMember("channels");

                    for (int c = 0; c < channelsNode.GetElementsCount(); c++)
                    {
                        const DataValue& channelNode = channelsNode.GetElement(c);
                        const DataValue& target = channelNode.GetMember("target");

                        String path = AsString(target.GetMember("path"));
                        if (path == "weights")
                            continue; // Morph targets are not supported

                        auto targetNode = target.FindMember("node");
                        if (!targetNode)
                            continue;

                        int samplerIndex = (int)channelNode.GetMember("sampler");
                        const DataValue& sampler = samplersNode.GetElement(samplerIndex);

                        String interpolation = "LINEAR";
                        if (auto interpolationNode = sampler.FindMember("interpolation"))
                            interpolation = AsString(*interpolationNode);

                        if (interpolation == "CUBICSPLINE")
                        {
                            if (errorMessage)
                                *errorMessage = "CUBICSPLINE animation interpolation is not supported";

                            return false;
                        }

                        SkinnedModelData::AnimationChannel channel;
                        channel.node = (int)*targetNode;
                        channel.step = interpolation == "STEP";

                        AccessorView inputView, outputView;
                        if (!ResolveAccessor(json, binary, (int)sampler.GetMember("input"), inputView, errorMessage) ||
                            !ResolveAccessor(json, binary, (int)sampler.GetMember("output"), outputView, errorMessage))
                        {
                            return false;
                        }

                        channel.times.Reserve(inputView.count);
                        for (int i = 0; i < inputView.count; i++)
                        {
                            float time = inputView.ReadComponent(i, 0);
                            channel.times.Add(time);
                            clip.duration = Math::Max(clip.duration, time);
                        }

                        int keysCount = Math::Min(inputView.count, outputView.count);
                        if (path == "rotation")
                        {
                            channel.path = SkinnedModelData::AnimationChannel::Path::Rotation;
                            channel.rotations.Reserve(keysCount);
                            for (int i = 0; i < keysCount; i++)
                            {
                                channel.rotations.Add(Quat(outputView.ReadComponent(i, 0), outputView.ReadComponent(i, 1),
                                                           outputView.ReadComponent(i, 2), outputView.ReadComponent(i, 3)));
                            }
                        }
                        else
                        {
                            channel.path = path == "translation"
                                ? SkinnedModelData::AnimationChannel::Path::Translation
                                : SkinnedModelData::AnimationChannel::Path::Scale;

                            channel.vectors.Reserve(keysCount);
                            for (int i = 0; i < keysCount; i++)
                            {
                                channel.vectors.Add(Vec3F(outputView.ReadComponent(i, 0), outputView.ReadComponent(i, 1),
                                                          outputView.ReadComponent(i, 2)));
                            }
                        }

                        clip.channels.Add(channel);
                    }

                    model.animations.Add(clip);
                }

                return true;
            }
        }

        bool Parse(const UInt8* data, UInt size, SkinnedModelData& model, String* errorMessage /*= nullptr*/)
        {
            model = SkinnedModelData();

            if (!data || size < 12)
            {
                if (errorMessage)
                    *errorMessage = "Data is too small for a GLB header";

                return false;
            }

            const UInt32* header = (const UInt32*)data;
            if (header[0] != glbMagic || header[1] != 2)
            {
                if (errorMessage)
                    *errorMessage = "Not a GLB 2.0 file";

                return false;
            }

            String jsonText;
            BinaryReader binary;

            UInt offset = 12;
            while (offset + 8 <= size)
            {
                UInt32 chunkLength = *(const UInt32*)(data + offset);
                UInt32 chunkType = *(const UInt32*)(data + offset + 4);
                offset += 8;

                if (offset + chunkLength > size)
                {
                    if (errorMessage)
                        *errorMessage = "Corrupted GLB chunk";

                    return false;
                }

                if (chunkType == jsonChunkType)
                    jsonText = String(std::string((const char*)(data + offset), (size_t)chunkLength));
                else if (chunkType == binChunkType)
                {
                    binary.data = data + offset;
                    binary.size = chunkLength;
                }

                offset += chunkLength;
            }

            if (jsonText.IsEmpty())
            {
                if (errorMessage)
                    *errorMessage = "GLB JSON chunk is missing";

                return false;
            }

            DataDocument json;
            if (!json.LoadFromData(jsonText))
            {
                if (errorMessage)
                    *errorMessage = "Failed to parse GLB JSON";

                return false;
            }

            if (!ParseGeometry(json, binary, model, errorMessage))
                return false;

            ParseNodes(json, model);

            if (!ParseSkin(json, binary, model, errorMessage))
                return false;

            if (!ParseAnimations(json, binary, model, errorMessage))
                return false;

            return true;
        }
    }
}
// --- META ---

ENUM_META(o2::SkinnedModelData::AnimationChannel::Path, o2__SkinnedModelData__AnimationChannel__Path)
{
    ENUM_ENTRY(Rotation);
    ENUM_ENTRY(Scale);
    ENUM_ENTRY(Translation);
}
END_ENUM_META;
// --- END META ---
