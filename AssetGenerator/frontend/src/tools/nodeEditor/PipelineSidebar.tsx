import React, { useEffect, useState } from 'react';
import { api } from '../../api/client.js';
import { useEditorStore } from './editorStore.js';
import { useErrorDialogStore } from '../../common/errorDialogStore.js';
import { ConfirmDialog } from '../../common/ConfirmDialog.js';
import { CloseIcon, PlayIcon, TrashIcon } from './icons.js';

interface Props {
  refreshTrigger: number;
  onPickPipeline(name: string): Promise<void>;
  onCancelEdit(): Promise<void>;
  onNew(): void;
  onDelete(name: string): Promise<void>;
  onPlayAll(): void;
  onPlayPipeline(name: string): Promise<void>;
}

interface Item {
  name: string;
  virtual?: boolean;
}

export function PipelineSidebar({ refreshTrigger, onPickPipeline, onCancelEdit, onNew, onDelete, onPlayAll, onPlayPipeline }: Props): JSX.Element {
  const pipelineName = useEditorStore((s) => s.pipelineName);
  const setName = useEditorStore((s) => s.setName);
  const dirty = useEditorStore((s) => s.dirty);

  const pushError = useErrorDialogStore((s) => s.push);
  const [list, setList] = useState<{ name: string }[]>([]);
  const [confirmDelete, setConfirmDelete] = useState<string | null>(null);

  // Local draft for the name input. Only committed (and persisted via setName,
  // which triggers autosave/rename) on blur or Enter — typing alone doesn't
  // produce intermediate files that would shuffle the list.
  const [draftName, setDraftName] = useState(pipelineName);
  useEffect(() => { setDraftName(pipelineName); }, [pipelineName]);

  const commitName = () => {
    const trimmed = draftName.trim();
    if (!trimmed) { setDraftName(pipelineName); return; }
    if (trimmed !== pipelineName.trim()) setName(trimmed);
  };

  const handleCancelClick = async () => {
    await onCancelEdit();
    setDraftName(useEditorStore.getState().pipelineName);
  };

  useEffect(() => {
    // Keep showing the previous list during refetch — no loading placeholder,
    // so the sidebar doesn't blank out and flicker on every refresh.
    api.listPipelines()
      .then((l) => setList(l))
      .catch((e) => pushError({ message: 'List failed', details: String(e) }));
  }, [refreshTrigger, pushError]);

  const currentName = pipelineName.trim();
  const isUntitled = !currentName || currentName === 'untitled';
  const currentInList = list.some((p) => p.name === currentName);

  const items: Item[] = [
    ...(currentInList ? [] : [{ name: isUntitled ? '' : currentName, virtual: true }]),
    ...list,
  ];

  const isSelected = (item: Item) => item.virtual ? true : item.name === currentName;

  return (
    <div className="pipeline-sidebar">
      <div className="sidebar-header sidebar-header-row">
        <span>Pipelines</span>
        <button className="sidebar-icon-btn play" onClick={onPlayAll} title="Play all finish nodes (parallel)">
          <PlayIcon size={11} color="white" />
        </button>
      </div>
      <div className="pipeline-list">
        {items.length === 0 && <div className="empty">No pipelines.</div>}

        {items.map((item) => {
          const selected = isSelected(item);
          return (
            <div key={item.virtual ? '__virtual__' : item.name} className={`pipeline-entry ${selected ? 'active' : ''}`}>
              {selected ? (
                <div className="entry-row">
                  <input
                    type="text"
                    value={draftName}
                    placeholder={item.virtual ? 'untitled' : 'name'}
                    onChange={(e) => setDraftName(e.target.value)}
                    onBlur={commitName}
                    onKeyDown={(e) => {
                      if (e.key === 'Enter') { (e.target as HTMLInputElement).blur(); }
                      else if (e.key === 'Escape') { setDraftName(pipelineName); (e.target as HTMLInputElement).blur(); }
                    }}
                  />
                  {dirty && <span className="dirty-dot" title="Modifications can be reverted">●</span>}
                  <button
                    className="sidebar-icon-btn play"
                    onClick={() => onPlayPipeline(item.virtual ? '' : item.name)}
                    title="Play all finish nodes"
                  >
                    <PlayIcon size={11} color="white" />
                  </button>
                  {dirty && (
                    <button className="sidebar-icon-btn cancel" onClick={handleCancelClick} title="Revert changes">
                      <CloseIcon size={12} />
                    </button>
                  )}
                  <button className="sidebar-icon-btn delete" onClick={() => setConfirmDelete(item.virtual ? '' : item.name)} title="Delete">
                    <TrashIcon size={11} />
                  </button>
                </div>
              ) : (
                <>
                  <button className="entry-btn" onClick={() => onPickPipeline(item.name)} title={`Open ${item.name}`}>
                    {item.name || '(untitled)'}
                  </button>
                  <button
                    className="sidebar-icon-btn play"
                    onClick={() => onPlayPipeline(item.name)}
                    title={`Play ${item.name}`}
                  >
                    <PlayIcon size={11} color="white" />
                  </button>
                  <button className="sidebar-icon-btn delete" onClick={() => setConfirmDelete(item.name)} title="Delete">
                    <TrashIcon size={11} />
                  </button>
                </>
              )}
            </div>
          );
        })}

        <button className="pipeline-new-btn" onClick={onNew} title="Create a fresh pipeline">+ New pipeline</button>
      </div>

      {confirmDelete !== null && (
        <ConfirmDialog
          title="Delete pipeline"
          message={confirmDelete
            ? `Delete pipeline "${confirmDelete}" permanently? This cannot be undone.`
            : 'Discard the current (untitled) pipeline?'}
          confirmLabel={confirmDelete ? 'Delete' : 'Discard'}
          destructive
          onConfirm={async () => {
            if (confirmDelete) {
              await onDelete(confirmDelete);
            } else {
              onNew();
            }
            setConfirmDelete(null);
          }}
          onCancel={() => setConfirmDelete(null)}
        />
      )}
    </div>
  );
}
