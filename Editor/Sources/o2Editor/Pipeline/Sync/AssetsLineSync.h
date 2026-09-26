#pragma once

#include "o2/Config/AssetsLineConfig.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/Sync/AssetsLineClient.h"

using namespace o2;

// AssetsLine sync access macro
#define o2AssetsLineSync Editor::AssetsLineSync::Instance()

namespace Editor
{
    class PipelineExecutor;

    // State of the link to AssetsLine, as the UI shows it
    enum class AssetsLineStatus { Off, NotConnected, Connecting, Syncing, Synced, Offline, Error };

    // -----------------------------------------------------------------------------
    // One result file of a synced pipeline as last seen equal on both sides: the
    // server's version of it and the local file's stamp at that moment
    // -----------------------------------------------------------------------------
    struct AssetsLineResultState
    {
        String v;     // Server version (mtime-size)
        String stamp; // Local file stamp (size and edit time)
    };

    // -----------------------------------------------------------------------
    // What the sync remembers about one pipeline between passes: the stored
    // revision its base corresponds to, where the local file is, and the result
    // files already equal on both sides. The base document itself is a file
    // -----------------------------------------------------------------------
    struct AssetsLinePipelineState
    {
        int    rev = 0;             // Stored revision of the base
        String name;                // Pipeline name at the base
        String file;                // Local file, relative to the assets folder
        bool   baseIsServer = true; // The base is exactly the stored revision (else it is what was last sent)

        Map<String, AssetsLineResultState> results; // Synced result files by key (see SyncResultInfo in AssetsLine)
    };

    // ----------------------------------------------------------------------------
    // Two-way sync of the project's pipelines with a linked AssetsLine project.
    // Pipeline assets in the configured folder are the AssetsLine documents; each
    // pass pushes local edits (the server merges them with edits made meanwhile),
    // pulls remote ones, and exchanges result files so that a result generated on
    // either side is available and fresh on the other. Results that reach finish
    // nodes are written into the project's assets. Local state lives in
    // Work/Pipelines/sync: the access token, revisions, bases and file versions
    // ----------------------------------------------------------------------------
    class AssetsLineSync : public Singleton<AssetsLineSync>
    {
    public:
        Function<void(const String&)> onPipelineFileChanged; // A synced pipeline file was rewritten; the asset path
        Function<bool(const String&)> isPipelineFileBusy;    // True while the editor holds unsaved edits of that asset
        Function<void()>              rebuildAssets;         // Makes the asset system pick up the files a pass wrote
        Function<void()>              onStatusChanged;       // Status, its text or the connection changed
        Function<void(const String&)> onResultsChanged;      // Result files of a pipeline changed here; the pipeline id
        Function<void(const String&)> openUrl;               // Opens the approval page; the system browser by default

    public:
        // Default constructor
        AssetsLineSync(RefCounter* refCounter);

        // Destructor
        ~AssetsLineSync();

        // Returns the link settings in use: the project's, or the override
        const AssetsLineConfig& GetConfig() const;

        // Changes the link settings; stored in the project settings unless overridden
        void SetConfig(const AssetsLineConfig& config);

        // Uses these settings instead of the project's, and keeps changes to them in memory (tests)
        void SetConfigOverride(const AssetsLineConfig& config);

        // Sets the assets folder the synced pipelines and finish results live in; empty for the project's
        void SetAssetsPath(const String& path);

        // Returns the assets folder in use, with a trailing slash
        String GetAssetsPath() const;

        // Starts connecting: asks the server for a code and opens its approval page in the browser
        void StartConnect(const String& serverUrl);

        // Connects with an access token made in the AssetsLine settings
        void ConnectWithToken(const String& serverUrl, const String& token);

        // Stops waiting for an approval
        void CancelConnect();

        // Forgets the access token; the link settings stay for a later connect
        void Disconnect();

        // Returns true when an access token for the linked project is stored
        bool IsConnected() const;

        // Runs a pass as soon as possible
        void SyncNow();

        // Tells that a synced pipeline was saved here: a pass runs shortly
        void NotifyLocalChange();

        // Starts passes on schedule and polls a pending approval; called every frame
        void Update(float dt);

        // Runs one full pass: pipelines, sources, results; returns false when it failed
        Coroutine<bool> SyncPass();

        // Returns true while a pass or a connection attempt is running
        bool IsBusy() const;

        // Returns the link state
        AssetsLineStatus GetStatus() const;

        // Returns a line describing the state: what is synced, when, or what went wrong
        const String& GetStatusText() const;

        // Returns the code the approval page shows, while connecting
        const String& GetConnectCode() const;

        // Returns the approval page address, while connecting
        const String& GetApproveUrl() const;

        // Returns the signed-in account of the stored token
        const String& GetUserEmail() const;

        // Returns recent sync messages, oldest first
        const Vector<String>& GetLog() const;

        // Returns the state kept for a pipeline id, null when it is not synced
        const AssetsLinePipelineState* GetPipelineState(const String& id) const;

        // Returns the id the sync sends as X-Sync-Client
        const String& GetClientId() const;

        // Drops the in-memory state and reads it again from the work folder (tests switch work folders)
        void ReloadState();

        struct LocalPipeline;
        struct Pass;

    protected:
        AssetsLineConfig mConfigOverride;    // Settings used instead of the project's
        bool             mHasOverride = false; // Whether the override is used
        String           mAssetsPath;        // Assets folder override

        String mToken;        // Stored access token
        String mTokenServer;  // Server the token belongs to
        String mTokenProject; // Project the token opens
        String mUserEmail;    // Account of the token
        String mClientId;     // X-Sync-Client of this editor

        Map<String, AssetsLinePipelineState> mPipelines; // Synced pipelines by id
        Map<String, String>                  mAssetUploadIds; // "<asset path>|<file stamp>" -> upload id of that content
        double mResultsSeq = 0;                           // Server's result counter at the last complete pass
        bool   mStateLoaded = false;                      // Whether the state was read from the work folder

        AssetsLineStatus mStatus = AssetsLineStatus::Off; // Link state
        String           mStatusText;                     // Line describing the state
        Vector<String>   mLog;                            // Recent messages

        Coroutine<void> mPassCoroutine;    // Running pass
        bool            mPassRunning = false; // Whether a pass is running
        float           mSinceLastPass = 1e9f; // Seconds since the last pass ended
        float           mChangeDelay = -1.0f;  // Seconds left before a pass for a local change, negative when none
        bool            mPassRequested = false; // A pass was asked for

        Coroutine<void> mConnectCoroutine; // Running connection attempt
        bool            mConnecting = false; // Whether a connection attempt is running
        bool            mConnectCancelled = false; // The attempt was cancelled
        String          mConnectCode;        // Code shown on the approval page
        String          mApproveUrl;         // Approval page address

    protected:
        // Returns the folder the sync state lives in
        String GetStatePath() const;

        // Returns the client for the linked project
        AssetsLineClient MakeClient() const;

        // Reads the token and the pipeline states from the work folder
        void LoadState();

        // Writes the pipeline states
        void SaveState() const;

        // Writes or removes the stored token
        void SaveCredentials() const;

        // Reads the base document of a pipeline
        String ReadBase(const String& id) const;

        // Writes the base document of a pipeline
        void WriteBase(const String& id, const String& text) const;

        // Sets the state and its text
        void SetStatus(AssetsLineStatus status, const String& text);

        // Adds a line to the log and the editor log
        void Log(const String& message);

        // Stores a token the server handed out and links the project it opens
        void AdoptToken(const String& serverUrl, const String& token, const String& email, const String& projectId,
                        const String& projectName);

        // The pass as a coroutine owned by the sync
        Coroutine<void> RunPass();

        // Polls the approval of a connect request
        Coroutine<void> ConnectCoroutine(String serverUrl);

        // Checks a hand-made token and links its project
        Coroutine<void> TokenCoroutine(String serverUrl, String token);

        // --- pipelines (AssetsLineSyncPipelines.cpp) ---

        // Reads every pipeline asset in the synced folder; gives files without an id (or a copied one) their own
        Map<String, LocalPipeline> ScanLocal(Pass& pass);

        // Exchanges pipeline documents: create, push, pull, rename and delete on either side
        Coroutine<bool> SyncDocuments(Pass& pass);

        // Pushes a local document; applies the answer when it brings changes made elsewhere
        Coroutine<bool> PushDocument(Pass& pass, LocalPipeline& local, AssetsLinePipelineState* state);

        // Pulls a stored document into its local file
        Coroutine<bool> PullDocument(Pass& pass, const String& id, LocalPipeline* local);

        // Writes a document into a local file, renaming the file when the name changed
        bool WriteLocal(Pass& pass, const String& id, const PipelineGraph& graph, const String& currentFile, String& writtenFile);

        // Returns the text of a document the change detection compares: the graph without the view
        static String ContentText(const PipelineGraph& graph);

        // --- results (AssetsLineSyncResults.cpp) ---

        // Exchanges the source files, result files and freshness of one pipeline
        Coroutine<bool> SyncResults(Pass& pass, const String& id);

        // Gives asset-backed source nodes the upload id of their current file content
        bool AssignAssetUploadIds(PipelineGraph& graph);

    };
}
// --- META ---

PRE_ENUM_META(Editor::AssetsLineStatus);
// --- END META ---
