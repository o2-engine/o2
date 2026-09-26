#include "o2Editor/stdafx.h"
#include "AssetsLineSyncInternal.h"

#include <ctime>
#include "o2/Assets/Assets.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/UID.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#if defined(PLATFORM_WASM)
#include <emscripten.h>
#endif

DECLARE_SINGLETON(Editor::AssetsLineSync);

namespace Editor
{
    static void OpenInBrowser(const String& url)
    {
        // Only a plain http(s) address reaches the shell
        if (!(url.StartsWith("https://") || url.StartsWith("http://")) || url.Contains("'") || url.Contains("\"") ||
            url.Contains("\\") || url.Contains(" ") || url.Contains("`") || url.Contains("$"))
            return;

#if defined(PLATFORM_WASM)
        EM_ASM({ window.open(UTF8ToString($0), '_blank'); }, url.Data());
#elif defined(PLATFORM_MAC)
        String command = "open '" + url + "'";
        [[maybe_unused]] int res = system(command.Data());
#elif defined(PLATFORM_WINDOWS)
        String command = "start \"\" \"" + url + "\"";
        [[maybe_unused]] int res = system(command.Data());
#elif defined(PLATFORM_LINUX)
        String command = "xdg-open '" + url + "' >/dev/null 2>&1 &";
        [[maybe_unused]] int res = system(command.Data());
#endif
    }

    static String Str(const DataValue* value)
    {
        return value ? PipelineUtils::ValueToString(*value) : String();
    }

    static String Str(const DataValue& object, const char* key)
    {
        return object.IsObject() ? Str(object.FindMember(key)) : String();
    }

    static String ClockTime()
    {
        std::time_t now = std::time(nullptr);
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
        return buf;
    }

    AssetsLineSync::AssetsLineSync(RefCounter* refCounter):
        Singleton<AssetsLineSync>(refCounter)
    {
        rebuildAssets = []() { o2Assets.RebuildAssets(); };
        openUrl = [](const String& url) { OpenInBrowser(url); };
    }

    AssetsLineSync::~AssetsLineSync()
    {}

    const AssetsLineConfig& AssetsLineSync::GetConfig() const
    {
        if (mHasOverride || !ProjectConfig::IsSingletonInitialzed())
            return mConfigOverride;

        return o2Config.assetsLine;
    }

    void AssetsLineSync::SetConfig(const AssetsLineConfig& config)
    {
        if (mHasOverride || !ProjectConfig::IsSingletonInitialzed())
            mConfigOverride = config;
        else
        {
            o2Config.assetsLine = config;
            o2Config.Save();
        }

        if (onStatusChanged)
            onStatusChanged();
    }

    void AssetsLineSync::SetConfigOverride(const AssetsLineConfig& config)
    {
        mConfigOverride = config;
        mHasOverride = true;
    }

    void AssetsLineSync::SetAssetsPath(const String& path)
    {
        mAssetsPath = path;
    }

    String AssetsLineSync::GetAssetsPath() const
    {
        String path = mAssetsPath.IsEmpty() ? o2Assets.GetAssetsPath() : mAssetsPath;
        if (!path.EndsWith("/"))
            path += "/";

        return path;
    }

    String AssetsLineSync::GetStatePath() const
    {
        return PipelineUtils::GetWorkPath() + "sync/";
    }

    AssetsLineClient AssetsLineSync::MakeClient() const
    {
        AssetsLineClient client;
        client.serverUrl = GetConfig().serverUrl;
        client.token = mToken;
        client.projectId = GetConfig().projectId;
        client.clientId = mClientId;
        return client;
    }

    bool AssetsLineSync::IsConnected() const
    {
        auto& config = GetConfig();
        return !mToken.IsEmpty() && mTokenServer == config.serverUrl && mTokenProject == config.projectId;
    }

    void AssetsLineSync::ReloadState()
    {
        mStateLoaded = false;
        LoadState();
    }

    void AssetsLineSync::LoadState()
    {
        mStateLoaded = true;
        mToken = mTokenServer = mTokenProject = mUserEmail = "";
        mPipelines.Clear();
        mResultsSeq = 0;

        DataDocument credentials;
        if (credentials.LoadFromFile(GetStatePath() + "credentials.json") && credentials.IsObject())
        {
            mToken = Str(credentials, "token");
            mTokenServer = Str(credentials, "serverUrl");
            mTokenProject = Str(credentials, "projectId");
            mUserEmail = Str(credentials, "email");
        }

        DataDocument state;
        if (!state.LoadFromFile(GetStatePath() + "state.json") || !state.IsObject())
            state.SetObject();

        mClientId = Str(state, "clientId");
        if (mClientId.IsEmpty())
        {
            UID uid;
            uid.Randomize();
            mClientId = "o2-" + (String)uid;
        }

        // The state belongs to one linked project; another link starts from scratch
        auto& config = GetConfig();
        if (Str(state, "serverUrl") != config.serverUrl || Str(state, "projectId") != config.projectId)
            return;

        if (auto seq = state.FindMember("resultsSeq"); seq && seq->IsNumber())
            mResultsSeq = (double)*seq;
        auto pipelines = state.FindMember("pipelines");
        if (!pipelines || !pipelines->IsObject())
            return;

        for (auto it = pipelines->BeginMember(); it != pipelines->EndMember(); ++it)
        {
            auto& value = it->value;
            AssetsLinePipelineState item;
            if (!value.IsObject())
                continue;

            auto rev = value.FindMember("rev");
            item.rev = rev && rev->IsNumber() ? (int)*rev : 0;
            item.name = Str(value, "name");
            item.file = Str(value, "file");
            auto baseIsServer = value.FindMember("baseIsServer");
            item.baseIsServer = !baseIsServer || !baseIsServer->IsBoolean() || (bool)*baseIsServer;
            if (auto results = value.FindMember("results"); results && results->IsObject())
            {
                for (auto r = results->BeginMember(); r != results->EndMember(); ++r)
                {
                    AssetsLineResultState result;
                    result.v = Str(r->value, "v");
                    result.stamp = Str(r->value, "stamp");
                    item.results[r->name.GetString()] = result;
                }
            }

            mPipelines[it->name.GetString()] = item;
        }
    }

    void AssetsLineSync::SaveState() const
    {
        auto& config = GetConfig();
        DataDocument state;
        state.SetObject();
        state["clientId"] = mClientId;
        state["serverUrl"] = config.serverUrl;
        state["projectId"] = config.projectId;
        state["resultsSeq"] = mResultsSeq;
        auto& pipelines = state["pipelines"];
        pipelines.SetObject();
        for (auto& kv : mPipelines)
        {
            auto& item = pipelines.AddMember(kv.first.Data());
            item.SetObject();
            item["rev"] = kv.second.rev;
            item["name"] = kv.second.name;
            item["file"] = kv.second.file;
            item["baseIsServer"] = kv.second.baseIsServer;
            auto& results = item["results"];
            results.SetObject();
            for (auto& r : kv.second.results)
            {
                auto& result = results.AddMember(r.first.Data());
                result.SetObject();
                result["v"] = r.second.v;
                result["stamp"] = r.second.stamp;
            }
        }

        PipelineUtils::WriteFileBytes(GetStatePath() + "state.json", state.SaveAsString());
    }

    void AssetsLineSync::SaveCredentials() const
    {
        String path = GetStatePath() + "credentials.json";
        if (mToken.IsEmpty())
        {
            o2FileSystem.FileDelete(path);
            return;
        }

        DataDocument credentials;
        credentials.SetObject();
        credentials["serverUrl"] = mTokenServer;
        credentials["projectId"] = mTokenProject;
        credentials["token"] = mToken;
        credentials["email"] = mUserEmail;
        PipelineUtils::WriteFileBytes(path, credentials.SaveAsString());
    }

    String AssetsLineSync::ReadBase(const String& id) const
    {
        return PipelineUtils::ReadFileBytes(GetStatePath() + "base/" + id + ".json");
    }

    void AssetsLineSync::WriteBase(const String& id, const String& text) const
    {
        String path = GetStatePath() + "base/" + id + ".json";
        if (text.IsEmpty())
            o2FileSystem.FileDelete(path);
        else
            PipelineUtils::WriteFileBytes(path, text);
    }

    const AssetsLinePipelineState* AssetsLineSync::GetPipelineState(const String& id) const
    {
        auto it = mPipelines.find(id);
        return it == mPipelines.end() ? nullptr : &it->second;
    }

    const String& AssetsLineSync::GetClientId() const
    {
        return mClientId;
    }

    void AssetsLineSync::SetStatus(AssetsLineStatus status, const String& text)
    {
        if (mStatus == status && mStatusText == text)
            return;

        mStatus = status;
        mStatusText = text;
        if (onStatusChanged)
            onStatusChanged();
    }

    void AssetsLineSync::Log(const String& message)
    {
        mLog.Add(ClockTime() + " " + message);
        while (mLog.Count() > 100)
            mLog.RemoveAt(0);

        o2Debug.Log("AssetsLine: " + message);
    }

    AssetsLineStatus AssetsLineSync::GetStatus() const { return mStatus; }
    const String& AssetsLineSync::GetStatusText() const { return mStatusText; }
    const String& AssetsLineSync::GetConnectCode() const { return mConnectCode; }
    const String& AssetsLineSync::GetApproveUrl() const { return mApproveUrl; }
    const String& AssetsLineSync::GetUserEmail() const { return mUserEmail; }
    const Vector<String>& AssetsLineSync::GetLog() const { return mLog; }

    bool AssetsLineSync::IsBusy() const
    {
        return mPassRunning || mConnecting;
    }

    void AssetsLineSync::AdoptToken(const String& serverUrl, const String& token, const String& email,
                                    const String& projectId, const String& projectName)
    {
        auto config = GetConfig();
        bool otherProject = config.serverUrl != serverUrl || config.projectId != projectId;

        mToken = token;
        mTokenServer = serverUrl;
        mTokenProject = projectId;
        mUserEmail = email;
        SaveCredentials();

        config.serverUrl = serverUrl;
        config.projectId = projectId;
        config.projectName = projectName;
        config.enabled = true;
        if (config.folder.Trimed(" /").IsEmpty())
            config.folder = "Pipelines";

        SetConfig(config);
        if (otherProject)
        {
            mPipelines.Clear();
            mResultsSeq = 0;
            SaveState();
        }

        Log("connected to " + serverUrl + " as " + email + ", project " + projectName);
        SetStatus(AssetsLineStatus::Syncing, "Connected, syncing...");
    }

    void AssetsLineSync::StartConnect(const String& serverUrl)
    {
        if (mConnecting)
            return;

        if (!mStateLoaded)
            LoadState();

        mConnecting = true;
        mConnectCancelled = false;
        mConnectCoroutine = ConnectCoroutine(AssetsLineClient::NormalizeServerUrl(serverUrl));
        mConnectCoroutine.Start(JobThread::Main);
    }

    void AssetsLineSync::ConnectWithToken(const String& serverUrl, const String& token)
    {
        if (mConnecting)
            return;

        if (!mStateLoaded)
            LoadState();

        mConnecting = true;
        mConnectCancelled = false;
        mConnectCoroutine = TokenCoroutine(AssetsLineClient::NormalizeServerUrl(serverUrl), token.Trimed(" \n\r\t"));
        mConnectCoroutine.Start(JobThread::Main);
    }

    void AssetsLineSync::CancelConnect()
    {
        mConnectCancelled = true;
    }

    void AssetsLineSync::Disconnect()
    {
        mToken = "";
        mTokenServer = "";
        mTokenProject = "";
        SaveCredentials();
        Log("disconnected");
        SetStatus(AssetsLineStatus::NotConnected, "Not connected");
    }

    Coroutine<void> AssetsLineSync::ConnectCoroutine(String serverUrl)
    {
        Ref<AssetsLineSync> self(this);
        AssetsLineClient client;
        client.serverUrl = serverUrl;

        DataDocument body;
        body.SetObject();
        String project = ProjectConfig::IsSingletonInitialzed() ? o2Config.GetProjectName() : String();
        body["clientName"] = "o2 editor" + (project.IsEmpty() ? String() : " (" + project + ")");
        auto start = co_await client.SendJson(HttpMethod::Post, "/api/connect/start", body);
        if (!start.ok || !start.json.IsObject())
        {
            mConnecting = false;
            SetStatus(start.offline ? AssetsLineStatus::Offline : AssetsLineStatus::Error, "Could not start connecting: " + start.error);
            co_return;
        }

        mConnectCode = Str(start.json, "code");
        mApproveUrl = Str(start.json, "approveUrl");
        String pollSecret = Str(start.json, "pollSecret");
        auto number = [&](const char* key, float def)
        {
            auto value = start.json.FindMember(key);
            return value ? PipelineUtils::ValueToNumber(*value, def) : def;
        };
        float interval = Math::Max(1.0f, number("interval", 2.0f));
        int polls = (int)(number("expiresIn", 600.0f) / interval);

        Log("waiting for the approval of code " + mConnectCode);
        SetStatus(AssetsLineStatus::Connecting, "Approve the connection in the browser, code " + mConnectCode);
        if (openUrl)
            openUrl(mApproveUrl);

        DataDocument poll;
        poll.SetObject();
        poll["code"] = mConnectCode;
        poll["pollSecret"] = pollSecret;

        String outcome = "expired";
        for (int i = 0; i < polls && !mConnectCancelled; i++)
        {
            co_await WaitTime(interval);
            co_await SwitchToMain();
            if (mConnectCancelled)
                break;

            auto answer = co_await client.SendJson(HttpMethod::Post, "/api/connect/poll", poll);
            if (!answer.ok || !answer.json.IsObject())
                continue;

            String status = Str(answer.json, "status");
            if (status == "pending")
                continue;

            if (status == "approved")
            {
                auto user = answer.json.FindMember("user");
                auto linked = answer.json.FindMember("project");
                AdoptToken(serverUrl, Str(answer.json, "token"), user ? Str(*user, "email") : String(),
                           linked ? Str(*linked, "id") : String(), linked ? Str(*linked, "name") : String());
                outcome = "";
            }
            else
                outcome = status;

            break;
        }

        mConnecting = false;
        mConnectCode = "";
        mApproveUrl = "";
        if (mConnectCancelled)
            SetStatus(AssetsLineStatus::NotConnected, "Not connected");
        else if (outcome == "denied")
            SetStatus(AssetsLineStatus::Error, "The connection was denied in the browser");
        else if (!outcome.IsEmpty())
            SetStatus(AssetsLineStatus::Error, "The connection request expired, connect again");
        else
            SyncNow();
    }

    Coroutine<void> AssetsLineSync::TokenCoroutine(String serverUrl, String token)
    {
        Ref<AssetsLineSync> self(this);
        AssetsLineClient client;
        client.serverUrl = serverUrl;
        client.token = token;
        auto me = co_await client.Request(HttpMethod::Get, "/api/sync/me");
        mConnecting = false;
        if (!me.ok || !me.json.IsObject())
        {
            SetStatus(me.offline ? AssetsLineStatus::Offline : AssetsLineStatus::Error, "The token was not accepted: " + me.error);
            co_return;
        }

        auto project = me.json.FindMember("project");
        if (!project || !project->IsObject())
        {
            SetStatus(AssetsLineStatus::Error, "This token does not belong to a project");
            co_return;
        }

        auto user = me.json.FindMember("user");
        AdoptToken(serverUrl, token, user ? Str(*user, "email") : String(), Str(*project, "id"), Str(*project, "name"));
        SyncNow();
    }

    void AssetsLineSync::SyncNow()
    {
        mPassRequested = true;
    }

    void AssetsLineSync::NotifyLocalChange()
    {
        mChangeDelay = 1.0f;
    }

    void AssetsLineSync::Update(float dt)
    {
        if (!mStateLoaded)
            LoadState();

        if (mConnecting)
            return;

        auto& config = GetConfig();
        if (!config.enabled)
        {
            SetStatus(AssetsLineStatus::Off, "Sync is off");
            return;
        }

        if (!IsConnected())
        {
            if (mStatus != AssetsLineStatus::Error)
                SetStatus(AssetsLineStatus::NotConnected, "Not connected");
            return;
        }

        if (mPassRunning)
            return;

        mSinceLastPass += dt;
        if (mChangeDelay >= 0.0f)
        {
            mChangeDelay -= dt;
            if (mChangeDelay < 0.0f)
                mPassRequested = true;
        }

        // An unreachable server is asked less often
        float interval = Math::Max(2.0f, config.pollInterval);
        if (mStatus == AssetsLineStatus::Offline)
            interval = Math::Min(60.0f, interval * 3.0f);

        if (!mPassRequested && mSinceLastPass < interval)
            return;

        mPassRequested = false;
        mPassRunning = true;
        mPassCoroutine = RunPass();
        mPassCoroutine.Start(JobThread::Main);
    }

    Coroutine<void> AssetsLineSync::RunPass()
    {
        Ref<AssetsLineSync> self(this);
        co_await SyncPass();
        mPassRunning = false;
        mSinceLastPass = 0.0f;
    }

    Coroutine<bool> AssetsLineSync::SyncPass()
    {
        Ref<AssetsLineSync> self(this);
        if (!mStateLoaded)
            LoadState();

        if (!IsConnected())
            co_return false;

        Pass pass;
        pass.config = GetConfig();
        pass.client = MakeClient();
        pass.assetsPath = GetAssetsPath();
        pass.folder = pass.config.folder.Trimed(" /\\");
        pass.folder.ReplaceAll("\\", "/");
        pass.folder = pass.folder.IsEmpty() ? String() : pass.folder + "/";

        auto state = co_await pass.client.Request(HttpMethod::Get, "/api/sync/state");
        if (!state.ok || !state.json.IsObject())
        {
            if (state.offline)
                SetStatus(AssetsLineStatus::Offline, "Offline: " + state.error);
            else if (state.status == 401)
                SetStatus(AssetsLineStatus::Error, "The access token is no longer valid, connect again");
            else if (state.status == 403)
                SetStatus(AssetsLineStatus::Error, "No access to the linked project: " + state.error);
            else
                SetStatus(AssetsLineStatus::Error, "Sync failed: " + state.error);

            co_return false;
        }

        if (mStatus != AssetsLineStatus::Synced)
            SetStatus(AssetsLineStatus::Syncing, "Syncing...");

        if (auto seq = state.json.FindMember("resultsSeq"); seq && seq->IsNumber())
            pass.resultsSeq = (double)*seq;

        if (auto list = state.json.FindMember("pipelines"); list && list->IsArray())
        {
            for (auto& item : *list)
            {
                String id = Str(item, "id");
                auto rev = item.IsObject() ? item.FindMember("rev") : nullptr;
                pass.remoteRev[id] = rev && rev->IsNumber() ? (int)*rev : 0;
                pass.remoteName[id] = Str(item, "name");
            }
        }

        bool ok = co_await SyncDocuments(pass);
        if (ok)
        {
            Vector<String> ids;
            for (auto& kv : mPipelines)
                ids.Add(kv.first);

            for (auto& id : ids)
            {
                if (!pass.local.ContainsKey(id) || pass.busyIds.Contains(id))
                    continue;

                bool resultsOk = co_await SyncResults(pass, id);
                ok = ok && resultsOk;
            }
        }

        if (ok)
            mResultsSeq = pass.resultsSeq;

        SaveState();
        if (pass.assetsChanged && rebuildAssets)
            rebuildAssets();

        if (pass.offline)
            SetStatus(AssetsLineStatus::Offline, "Offline: " + pass.error);
        else if (!ok)
            SetStatus(AssetsLineStatus::Error, pass.error.IsEmpty() ? String("Sync failed") : pass.error);
        else
        {
            String text = "Synced at " + ClockTime() + " - " + (String)mPipelines.Count() + " pipeline" + (mPipelines.Count() == 1 ? "" : "s");
            if (!pass.busyIds.IsEmpty())
                text += ", waiting for unsaved edits";

            SetStatus(AssetsLineStatus::Synced, text);
        }

        // Unsaved edits get their own pass once they are saved
        if (!pass.busyIds.IsEmpty())
            mChangeDelay = Math::Max(mChangeDelay, 2.0f);

        co_return ok;
    }
}
// --- META ---

ENUM_META(Editor::AssetsLineStatus, Editor__AssetsLineStatus)
{
    ENUM_ENTRY(Connecting);
    ENUM_ENTRY(Error);
    ENUM_ENTRY(NotConnected);
    ENUM_ENTRY(Off);
    ENUM_ENTRY(Offline);
    ENUM_ENTRY(Synced);
    ENUM_ENTRY(Syncing);
}
END_ENUM_META;
// --- END META ---
