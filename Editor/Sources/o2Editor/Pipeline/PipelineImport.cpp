#include "o2Editor/stdafx.h"
#include "PipelineImport.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineZip.h"

namespace Editor
{
    namespace PipelineImport
    {
        static String StringOf(DataValue* value, const String& def = "")
        {
            if (!value || !value->IsString())
                return def;

            return String(value->GetString());
        }

        static float NumberOf(DataValue* value, float def = 0.0f)
        {
            return value ? PipelineUtils::ValueToNumber(*value, def) : def;
        }

        static Vec2F PointOf(DataValue* value)
        {
            if (!value || !value->IsObject())
                return Vec2F();

            return Vec2F(NumberOf(value->FindMember("x")), NumberOf(value->FindMember("y")));
        }

        static Vector<PipelinePort> PortsOf(DataValue* value)
        {
            Vector<PipelinePort> ports;
            if (!value || !value->IsArray())
                return ports;

            for (int i = 0; i < value->GetElementsCount(); i++)
            {
                auto& port = value->GetElement(i);
                if (!port.IsObject())
                    continue;

                bool custom = false;
                if (auto flag = port.FindMember("custom"))
                {
                    if (flag->IsBoolean())
                        flag->Get(custom);
                }

                ports.Add(PipelinePort(StringOf(port.FindMember("id")), StringOf(port.FindMember("name")),
                                       PipelinePortTypeFromString(StringOf(port.FindMember("type"), "text")), custom));
            }

            return ports;
        }

        static PipelinePortType MediaTypeOf(const String& mediaType)
        {
            if (mediaType == "image") return PipelinePortType::Image;
            if (mediaType == "video") return PipelinePortType::Video;
            if (mediaType == "audio") return PipelinePortType::Audio;
            return PipelinePortType::Text;
        }

        static PipelinePortType TypeOfMime(const String& mime)
        {
            if (mime.StartsWith("image/")) return PipelinePortType::Image;
            if (mime.StartsWith("video/")) return PipelinePortType::Video;
            if (mime.StartsWith("audio/")) return PipelinePortType::Audio;
            return PipelinePortType::Text;
        }

        static PipelineValue MakeValue(PipelinePortType type, const String& mime, const String& bytes)
        {
            if (bytes.IsEmpty())
                return PipelineValue();

            if (type == PipelinePortType::Text)
                return PipelineValue::Text(bytes);

            String fallback = PipelineUtils::MimeForExtension(type == PipelinePortType::Image ? "png" : type == PipelinePortType::Video ? "mp4" : "mp3");
            return PipelineValue::Bytes(type, bytes, mime.IsEmpty() ? fallback : mime);
        }

        // Decodes "data:<mime>;base64,<payload>" into bytes; mime receives the media type
        static String DecodeDataUrl(const String& dataUrl, String& mime)
        {
            int comma = dataUrl.Find(",");
            if (!dataUrl.StartsWith("data:") || comma < 0)
                return "";

            String header = dataUrl.SubStr(5, comma);
            int semicolon = header.Find(";");
            mime = semicolon >= 0 ? header.SubStr(0, semicolon) : header;
            return PipelineUtils::Base64Decode(dataUrl.SubStr(comma + 1));
        }

        static PipelineValue ResultOf(DataValue& result)
        {
            PipelinePortType type = MediaTypeOf(StringOf(result.FindMember("mediaType"), "text"));
            if (type == PipelinePortType::Text)
            {
                if (auto text = result.FindMember("text"))
                    return PipelineValue::Text(StringOf(text));
            }

            String mime;
            String bytes = DecodeDataUrl(StringOf(result.FindMember("dataUrl")), mime);
            return MakeValue(type, mime, bytes);
        }

        Bundle Parse(const String& json)
        {
            Bundle bundle;
            DataDocument doc;
            if (!doc.LoadFromData(json) || !doc.IsObject())
            {
                bundle.error = "not a JSON object";
                return bundle;
            }

            DataValue* pipeline = &doc;
            if (auto inner = doc.FindMember("pipeline"))
            {
                if (inner->IsObject())
                    pipeline = inner;
            }

            auto nodes = pipeline->FindMember("nodes");
            auto edges = pipeline->FindMember("edges");
            if (!nodes || !nodes->IsArray() || !edges || !edges->IsArray())
            {
                bundle.error = "missing nodes or edges";
                return bundle;
            }

            bundle.name = StringOf(pipeline->FindMember("name"));
            for (int i = 0; i < nodes->GetElementsCount(); i++)
            {
                auto& value = nodes->GetElement(i);
                if (!value.IsObject())
                    continue;

                auto node = mmake<PipelineNode>();
                node->id = StringOf(value.FindMember("id"));
                node->nodeType = StringOf(value.FindMember("type"));
                node->position = PointOf(value.FindMember("position"));
                if (auto size = value.FindMember("size"))
                {
                    if (size->IsObject())
                        node->size = Vec2F(NumberOf(size->FindMember("width")), NumberOf(size->FindMember("height")));
                }

                node->config.Clear();
                if (auto config = value.FindMember("config"))
                    node->config = *config;

                if (!node->config.IsObject())
                    node->config.SetObject();

                node->inputs = PortsOf(value.FindMember("inputs"));
                node->outputs = PortsOf(value.FindMember("outputs"));
                if (node->id.IsEmpty() || node->nodeType.IsEmpty())
                    continue;

                bundle.graph.nodes.Add(node);
            }

            for (int i = 0; i < edges->GetElementsCount(); i++)
            {
                auto& value = edges->GetElement(i);
                if (!value.IsObject())
                    continue;

                auto edge = mmake<PipelineEdge>();
                edge->id = StringOf(value.FindMember("id"));
                edge->fromNodeId = StringOf(value.FindMember("fromNodeId"));
                edge->fromPortId = StringOf(value.FindMember("fromPortId"));
                edge->toNodeId = StringOf(value.FindMember("toNodeId"));
                edge->toPortId = StringOf(value.FindMember("toPortId"));
                if (auto points = value.FindMember("points"))
                {
                    if (points->IsArray())
                    {
                        for (int k = 0; k < points->GetElementsCount(); k++)
                            edge->points.Add(PointOf(&points->GetElement(k)));
                    }
                }

                if (edge->id.IsEmpty())
                    edge->id = PipelineNode::GenerateId();

                if (bundle.graph.FindNode(edge->fromNodeId) && bundle.graph.FindNode(edge->toNodeId))
                    bundle.graph.edges.Add(edge);
            }

            if (auto results = doc.FindMember("results"))
            {
                if (results->IsObject())
                {
                    for (auto it = results->BeginMember(); it != results->EndMember(); ++it)
                    {
                        String nodeId = it->name.GetString();
                        if (!it->value.IsObject() || !bundle.graph.FindNode(nodeId))
                            continue;

                        PipelineValue value = ResultOf(it->value);
                        if (value.IsValid())
                            bundle.results[nodeId] = value;
                    }
                }
            }

            bundle.graph.id = PipelineNode::GenerateId();
            bundle.ok = true;
            return bundle;
        }

        Bundle ParseZip(const String& bytes)
        {
            Bundle bundle;
            Vector<PipelineZip::Entry> entries;
            if (!PipelineZip::Read(bytes, entries, bundle.error))
                return bundle;

            auto find = [&](const String& name) -> const PipelineZip::Entry*
            {
                for (auto& entry : entries)
                {
                    if (entry.name == name)
                        return &entry;
                }
                return nullptr;
            };

            DataDocument manifest;
            String pipelineFile = "pipeline.json";
            if (auto entry = find("manifest.json"))
            {
                if (manifest.LoadFromData(entry->data) && manifest.IsObject())
                    pipelineFile = StringOf(manifest.FindMember("pipeline"), pipelineFile);
            }

            const PipelineZip::Entry* pipeline = find(pipelineFile);
            for (auto& entry : entries)
            {
                if (pipeline)
                    break;

                if (entry.name.EndsWith(".json") && !entry.name.Contains("/") && entry.name != "manifest.json")
                    pipeline = &entry;
            }

            if (!pipeline)
            {
                bundle.error = "pipeline.json not found in the archive";
                return bundle;
            }

            bundle = Parse(pipeline->data);
            if (!bundle.ok)
                return bundle;

            if (manifest.IsObject())
            {
                if (auto results = manifest.FindMember("results"))
                {
                    if (results->IsObject())
                    {
                        for (auto it = results->BeginMember(); it != results->EndMember(); ++it)
                        {
                            if (!it->value.IsObject())
                                continue;

                            // A part of a per-port node is keyed "<node id>#<port id>" and names them in its entry
                            String key = it->name.GetString();
                            String nodeId = StringOf(it->value.FindMember("nodeId"), key.Contains("#") ? key.SubStr(0, key.Find("#")) : key);
                            String portId = StringOf(it->value.FindMember("portId"));
                            if (!bundle.graph.FindNode(nodeId))
                                continue;

                            auto file = find(StringOf(it->value.FindMember("file")));
                            if (!file)
                                continue;

                            PipelineValue value = MakeValue(MediaTypeOf(StringOf(it->value.FindMember("mediaType"), "text")),
                                                            StringOf(it->value.FindMember("mime")), file->data);
                            if (!value.IsValid())
                                continue;

                            if (portId.IsEmpty())
                                bundle.results[nodeId] = value;
                            else
                                bundle.portResults[nodeId][portId] = value;
                        }
                    }
                }
            }

            // Source images and sounds are uploads, not results: without them the imported sources are empty
            if (manifest.IsObject())
            {
                if (auto uploads = manifest.FindMember("uploads"))
                {
                    if (uploads->IsObject())
                    {
                        for (auto it = uploads->BeginMember(); it != uploads->EndMember(); ++it)
                        {
                            if (!it->value.IsObject())
                                continue;

                            if (auto file = find(StringOf(it->value.FindMember("file"))))
                                bundle.uploads[String(it->name.GetString())] = file->data;
                        }
                    }
                }
            }

            // Files not listed by a manifest are keyed by their name: results/<nodeId>.<ext>
            for (auto& entry : entries)
            {
                if (!entry.name.StartsWith("results/"))
                    continue;

                String file = entry.name.SubStr(8);
                int dot = file.FindLast(".");
                String nodeId = dot > 0 ? file.SubStr(0, dot) : file;
                String ext = dot > 0 ? file.SubStr(dot + 1) : String();
                if (bundle.results.ContainsKey(nodeId) || !bundle.graph.FindNode(nodeId))
                    continue;

                String mime = ext == "txt" ? String("text/plain") : PipelineUtils::MimeForExtension(ext);
                PipelineValue value = MakeValue(TypeOfMime(mime), mime, entry.data);
                if (value.IsValid())
                    bundle.results[nodeId] = value;
            }

            return bundle;
        }

        Bundle ParseBytes(const String& bytes)
        {
            return PipelineZip::IsZip(bytes) ? ParseZip(bytes) : Parse(bytes);
        }

        int StoreResults(const String& pipelineId, const Bundle& bundle)
        {
            // The source files first: a source node hashes what it reads, so they belong in the signatures below
            for (auto& upload : bundle.uploads)
            {
                String path = PipelineUtils::GetUploadPath(upload.first);
                if (!upload.second.IsEmpty() && !o2FileSystem.IsFileExist(path))
                    PipelineUtils::WriteFileBytes(path, upload.second);
            }

            auto signatures = bundle.graph.ComputeSignatures();
            auto seeds = bundle.graph.ResolveSeeds();
            int stored = 0;

            // The parts of a per-port node: each gets its own preview and cache entry, so nothing is re-generated
            for (auto& nodeParts : bundle.portResults)
            {
                auto node = bundle.graph.FindNode(nodeParts.first);
                if (!node)
                    continue;

                auto upstream = bundle.graph.UpstreamSignatures(*node, signatures);
                int seed = -1;
                seeds.TryGetValue(node->id, seed);

                for (auto& part : nodeParts.second)
                {
                    if (!node->FindOutput(part.first))
                        continue;

                    const PipelineValue& value = part.second;
                    if (!PipelineUtils::WriteFileBytes(PipelineExecutor::GetPortPreviewPath(pipelineId, node->id, part.first,
                                                                                            value.GetExtension()), value.data))
                        continue;

                    PipelineExecutor::SaveContent(pipelineId, PipelineExecutor::PortSignature(*node, upstream, seed, part.first), value);
                    stored++;
                }
            }
            for (auto& kv : bundle.results)
            {
                auto node = bundle.graph.FindNode(kv.first);
                if (!node)
                    continue;

                const PipelineValue& value = kv.second;
                String ext = value.GetExtension();
                for (auto& other : PipelineExecutor::GetAudioExtensions())
                    o2FileSystem.FileDelete(PipelineExecutor::GetPreviewPath(pipelineId, node->id, other));

                if (!PipelineUtils::WriteFileBytes(PipelineExecutor::GetPreviewPath(pipelineId, node->id, ext), value.data))
                    continue;

                String sig;
                if (signatures.TryGetValue(node->id, sig) && !sig.IsEmpty())
                {
                    PipelineExecutor::SaveContent(pipelineId, sig, value);
                    PipelineExecutor::MarkRan(pipelineId, sig);
                }

                stored++;
            }

            return stored;
        }

        String SafeAssetName(const String& name)
        {
            String safe;
            for (int i = 0; i < name.Length(); i++)
            {
                char c = name[i];
                bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_';
                safe += ok ? c : '_';
            }

            return safe.Trimed(" _");
        }

        String ImportFile(const String& filePath, const String& folder, String& error)
        {
            String bytes = PipelineUtils::ReadFileBytes(filePath);
            if (bytes.IsEmpty())
            {
                error = "cannot read " + filePath;
                return "";
            }

            Bundle bundle = ParseBytes(bytes);
            if (!bundle.ok)
            {
                error = bundle.error;
                return "";
            }

            String name = SafeAssetName(bundle.name);
            if (name.IsEmpty())
                name = SafeAssetName(o2FileSystem.GetFileNameWithoutExtension(filePath));

            if (name.IsEmpty())
                name = "Imported";

            String prefix = folder;
            if (!prefix.IsEmpty() && !prefix.EndsWith("/"))
                prefix += "/";

            String assetPath = prefix + name + ".pipeline";
            for (int i = 2; o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + assetPath); i++)
                assetPath = prefix + name + " " + (String)i + ".pipeline";

            if (!prefix.IsEmpty())
                o2FileSystem.FolderCreate(o2Assets.GetAssetsPath() + prefix, true);

            auto asset = mmake<PipelineAsset>();
            bundle.graph.SaveToAsset(*asset);
            asset->Save(assetPath);

            StoreResults(bundle.graph.id, bundle);
            o2Assets.RebuildAssets();
            return assetPath;
        }
    }
}
