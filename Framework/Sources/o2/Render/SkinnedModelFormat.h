#pragma once

#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // ------------------------------------------------------------------------------
    // Skinned model data: bind pose geometry with per-vertex bone influences, nodes
    // hierarchy with a skin (joints + inverse bind matrices) and animation clips.
    // Pose evaluation and CPU skinning produce model space geometry for rendering
    // ------------------------------------------------------------------------------
    struct SkinnedModelData
    {
        static constexpr int influencesPerVertex = 4;

        // Per-vertex bone influences: up to four weighted joints of the skin
        struct VertexInfluence
        {
            int   joints[influencesPerVertex] = { 0, 0, 0, 0 };
            float weights[influencesPerVertex] = { 0.0f, 0.0f, 0.0f, 0.0f };
        };

        // Scene node: local TRS transform inside the parents hierarchy
        struct Node
        {
            String name;
            int    parent = -1;

            Vec3F position;
            Quat  rotation;
            Vec3F scale = Vec3F(1.0f, 1.0f, 1.0f);
        };

        // Animation channel: keyframed track of one node transform component
        struct AnimationChannel
        {
            enum class Path { Translation, Rotation, Scale };

            int  node = -1;
            Path path = Path::Translation;
            bool step = false; // STEP interpolation, LINEAR otherwise

            Vector<float> times;
            Vector<Vec3F> vectors;   // Translation and scale keys
            Vector<Quat>  rotations; // Rotation keys
        };

        // Animation clip: named set of channels
        struct AnimationClip
        {
            String name;
            float  duration = 0.0f;

            Vector<AnimationChannel> channels;
        };

    public:
        Vector<Vec3F> positions; // Bind pose positions, model space
        Vector<Vec3F> normals;   // Bind pose normals; empty when the source has none
        Vector<Vec2F> uvs;
        Vector<UInt>  indices;

        Vector<VertexInfluence> influences; // Per-vertex bone influences

        Vector<Node> nodes;

        Vector<int>  joints;              // Skin joints as node indices
        Vector<Mat4> inverseBindMatrices; // Per-joint inverse bind matrices

        Vector<AnimationClip> animations;

    public:
        // Returns animation clip index by name, -1 when not found
        int FindAnimation(const String& name) const;

        // Returns bind pose positions bounds; false when empty
        bool GetBounds(AABB& bounds) const;

        // Evaluates local node transforms: bind pose, overridden by the clip channels at time.
        // Pass animation -1 for the pure bind pose
        void EvaluateNodeTransforms(int animation, float time, Vector<Mat4>& outLocals) const;

        // Converts local node transforms to global ones through the parents hierarchy
        void EvaluateGlobalTransforms(const Vector<Mat4>& locals, Vector<Mat4>& outGlobals) const;

        // Evaluates the skinning palette: globalJoint * inverseBind per skin joint
        void EvaluateJointsPalette(int animation, float time, Vector<Mat4>& outPalette) const;

        // Skins bind pose geometry with the palette into model space positions and normals.
        // When the source has no normals, smooth normals are computed from the skinned geometry
        void SkinVertices(const Vector<Mat4>& palette, Vector<Vec3F>& outPositions, Vector<Vec3F>& outNormals) const;

        // Computes smooth per-vertex normals from positions and triangle indices
        static void ComputeSmoothNormals(const Vector<Vec3F>& positions, const Vector<UInt>& indices,
                                         Vector<Vec3F>& outNormals);
    };

    // ------------------------------------------------------------------
    // Minimal GLB (binary glTF 2.0) parser for skinned animated models:
    // first mesh primitive, first skin, TRS node animations
    // ------------------------------------------------------------------
    namespace GlbModelFormat
    {
        // Parses GLB binary data into the model, returns false with error message on failure
        bool Parse(const UInt8* data, UInt size, SkinnedModelData& model, String* errorMessage = nullptr);
    }
}
// --- META ---

PRE_ENUM_META(o2::SkinnedModelData::AnimationChannel::Path);
// --- END META ---
