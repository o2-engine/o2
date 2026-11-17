#include "o2Editor/stdafx.h"
#include "PrototypePreview.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/SceneDrawableCategory.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include <map>
#include <string>

namespace Editor
{
    namespace PrototypePreview
    {
        namespace
        {
            // Screen a stretched root widget is laid out in: the portrait phone screen the games are made for
            const Vec2F referenceScreen(1080.0f, 1920.0f);

            struct CacheEntry
            {
                std::string key;       // Asset file state the thumbnail was made for
                TextureRef  thumbnail; // Empty when the prototype has no 2D content
            };

            std::map<std::string, CacheEntry>& Cache()
            {
                static std::map<std::string, CacheEntry> cache;
                return cache;
            }

            int& RenderedCount()
            {
                static int count = 0;
                return count;
            }

            std::string AssetId(const AssetRef<ActorAsset>& asset)
            {
                return std::string(((String)asset->GetUID().ToString()).Data());
            }

            // The file's size and modification time: a saved change makes another key
            std::string FileKey(const AssetRef<ActorAsset>& asset)
            {
                String path = asset->GetPath();
                if (path.IsEmpty())
                    return String::Format("%p", (const void*)asset.Get()).Data();

                auto info = o2FileSystem.GetFileInfo(o2Assets.GetAssetsPath() + path);
                const TimeStamp& date = info.editDate;
                return std::string(path.Data()) + String::Format(":%lld:%d-%d-%d-%d-%d-%d", (long long)info.size,
                                                                 date.mYear, date.mMonth, date.mDay, date.mHour,
                                                                 date.mMinute, date.mSecond).Data();
            }

            void IncludeRect(RectF& bounds, bool& has, const RectF& rect)
            {
                if (rect.Width() < 0.01f && rect.Height() < 0.01f)
                    return;

                bounds = has ? bounds.Expand(rect) : rect;
                has = true;
            }

            bool HasDrawableLayers(const Vector<Ref<WidgetLayer>>& layers)
            {
                for (auto& layer : layers)
                {
                    if (!layer->IsEnabled())
                        continue;

                    if (layer->GetDrawable() || HasDrawableLayers(layer->GetChildren()))
                        return true;
                }

                return false;
            }

            void CollectBounds(const Ref<Actor>& actor, RectF& bounds, bool& has)
            {
                if (!actor->IsEnabled())
                    return;

                if (auto widget = DynamicCast<Widget>(actor))
                {
                    if (HasDrawableLayers(widget->GetLayers()))
                        IncludeRect(bounds, has, widget->layout->GetWorldRect());
                }

                for (auto& component : actor->GetComponents())
                {
                    if (component->IsEnabled() && component->GetSceneDrawableCategory() == SceneDrawableCategory::Scene2D)
                        IncludeRect(bounds, has, actor->transform->GetWorldAxisAlignedRect());
                }

                for (auto& child : actor->GetChildren())
                    CollectBounds(child, bounds, has);
            }

            void DrawLayers(const Vector<Ref<WidgetLayer>>& layers)
            {
                for (auto& layer : layers)
                {
                    if (!layer->IsEnabled())
                        continue;

                    if (layer->GetDrawable())
                        layer->Draw();

                    DrawLayers(layer->GetChildren());
                }
            }

            // Draws only what is seen: Widget::Draw would also register the widgets for the input
            void DrawContent(const Ref<Actor>& actor)
            {
                if (!actor->IsEnabled())
                    return;

                if (auto widget = DynamicCast<Widget>(actor))
                    DrawLayers(widget->GetLayers());

                for (auto& component : actor->GetComponents())
                {
                    if (component->IsEnabled() && component->GetSceneDrawableCategory() == SceneDrawableCategory::Scene2D)
                        component->Draw();
                }

                for (auto& child : actor->GetChildren())
                    DrawContent(child);
            }

            TextureRef Render(const AssetRef<ActorAsset>& asset)
            {
                PushEditorScopeOnStack scope;

                auto prototype = asset->GetActor();
                if (!prototype)
                    return TextureRef();

                // A copy, laid out apart from the prototype: the prototype itself stays untouched
                auto instance = prototype->CloneAsRef<Actor>();
                instance->SetEnabled(true);

                Ref<Widget> screen;
                if (auto widget = DynamicCast<Widget>(instance))
                {
                    screen = mmake<Widget>();
                    *screen->layout = WidgetLayout::Based(BaseCorner::Center, referenceScreen);
                    screen->AddChild(widget);
                    screen->UpdateSelfTransform();
                    screen->UpdateChildrenTransforms();
                }
                else
                {
                    instance->UpdateSelfTransform();
                    instance->UpdateChildrenTransforms();
                }

                RectF bounds;
                if (!GetContentBounds(instance, bounds))
                    return TextureRef();

                Vec2F boundsSize = bounds.Size();
                float scale = (float)thumbnailSize/Math::Max(boundsSize.x, boundsSize.y);
                Vec2I size(Math::Max(8, (int)Math::Round(boundsSize.x*scale)), Math::Max(8, (int)Math::Round(boundsSize.y*scale)));

                TextureRef target(size, TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
                if (!target || !target->IsReady())
                    return TextureRef();

                target->SetName("Prototype thumbnail " + asset->GetPath());

                Camera previousCamera = o2Render.GetCamera();

                o2Render.PushRenderTargets({ target });
                o2Render.Clear(Color4(0, 0, 0, 0));

                o2Render.SetCamera(Camera(bounds.Center(), boundsSize));

                DrawContent(instance);

                o2Render.PopRenderTargets();
                o2Render.SetCamera(previousCamera);

                if (screen)
                    screen->RemoveChild(DynamicCast<Widget>(instance));

                RenderedCount()++;
                return target;
            }
        }

        TextureRef FindThumbnail(const AssetRef<ActorAsset>& asset)
        {
            if (!asset)
                return TextureRef();

            auto& cache = Cache();
            auto found = cache.find(AssetId(asset));
            if (found == cache.end() || found->second.key != FileKey(asset))
                return TextureRef();

            return found->second.thumbnail;
        }

        bool IsThumbnailPending(const AssetRef<ActorAsset>& asset)
        {
            if (!asset)
                return false;

            auto& cache = Cache();
            auto found = cache.find(AssetId(asset));
            return found == cache.end() || found->second.key != FileKey(asset);
        }

        TextureRef GetThumbnail(const AssetRef<ActorAsset>& asset)
        {
            if (!asset)
                return TextureRef();

            auto& cache = Cache();
            std::string id = AssetId(asset);
            std::string key = FileKey(asset);

            auto found = cache.find(id);
            if (found != cache.end() && found->second.key == key)
                return found->second.thumbnail;

            CacheEntry entry;
            entry.key = key;
            entry.thumbnail = Render(asset);
            cache[id] = entry;

            return entry.thumbnail;
        }

        bool GetContentBounds(const Ref<Actor>& actor, RectF& bounds)
        {
            bool has = false;
            if (actor)
                CollectBounds(actor, bounds, has);

            return has && bounds.Width() > 0.5f && bounds.Height() > 0.5f;
        }

        void ClearCache()
        {
            Cache().clear();
        }

        int GetRenderedCount()
        {
            return RenderedCount();
        }
    }
}
