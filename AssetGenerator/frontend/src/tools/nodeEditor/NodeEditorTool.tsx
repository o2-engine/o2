import React, { useEffect, useRef, useState } from 'react';
import { api } from '../../api/client.js';
import { useEditorStore } from './editorStore.js';
import { useExecutionStore } from './executionStore.js';
import { useToastStore } from '../../common/toastStore.js';
import { useErrorDialogStore } from '../../common/errorDialogStore.js';
import { Canvas } from './Canvas.js';
import { ContextMenus } from './ContextMenus.js';
import { PipelineSidebar } from './PipelineSidebar.js';

const LAST_PIPELINE_KEY = 'assetgen.editor.lastPipeline';
const AUTOSAVE_DEBOUNCE_MS = 600;

export function NodeEditorTool(): JSX.Element {
  const loadSchemas = useEditorStore((s) => s.loadSchemas);
  const newPipeline = useEditorStore((s) => s.newPipeline);
  const loadPipeline = useEditorStore((s) => s.loadPipeline);
  const revertToSnapshot = useEditorStore((s) => s.revertToSnapshot);
  const toPipeline = useEditorStore((s) => s.toPipeline);
  const pipelineName = useEditorStore((s) => s.pipelineName);
  const dirty = useEditorStore((s) => s.dirty);
  const nodes = useEditorStore((s) => s.nodes);
  const edges = useEditorStore((s) => s.edges);

  const execClearNode = useExecutionStore((s) => s.clearNodeState);
  const execSetQueued = useExecutionStore((s) => s.setQueued);
  const execSetState = useExecutionStore((s) => s.setState);
  const execSetOutput = useExecutionStore((s) => s.setOutput);
  const execLog = useExecutionStore((s) => s.appendLog);
  const execStartRequest = useExecutionStore((s) => s.startRequest);
  const execEndRequest = useExecutionStore((s) => s.endRequest);
  const execAbortRequest = useExecutionStore((s) => s.abortRequest);

  const pushToast = useToastStore((s) => s.push);
  const pushError = useErrorDialogStore((s) => s.push);

  const [listRefresh, setListRefresh] = useState(0);
  const lastSavedNameRef = useRef<string>('');

  useEffect(() => {
    (async () => {
      const list = await api.listNodeTypes();
      loadSchemas(list);
      const last = localStorage.getItem(LAST_PIPELINE_KEY);
      if (last) {
        try {
          const p = await api.getPipeline(last);
          loadPipeline(p);
          lastSavedNameRef.current = p.name;
          return;
        } catch {
          localStorage.removeItem(LAST_PIPELINE_KEY);
        }
      }
      if (useEditorStore.getState().nodes.length === 0) newPipeline();
    })().catch((err) => pushError({ message: 'Schema load failed', details: String(err) }));
  }, [loadSchemas, newPipeline, loadPipeline, pushError]);

  // Debounced autosave: writes on any state change after 600ms of inactivity.
  // Renames are handled by deleting the previously-saved file when the name changes.
  useEffect(() => {
    if (!dirty) return;
    const name = pipelineName.trim();
    if (!name || name === 'untitled') return;

    const t = setTimeout(async () => {
      try {
        const p = useEditorStore.getState().toPipeline();
        p.name = name;
        const oldName = lastSavedNameRef.current;
        const isRename = oldName !== name;
        await api.savePipeline(name, p);
        if (isRename) {
          if (oldName && oldName !== 'untitled') {
            try { await api.deletePipeline(oldName); } catch { /* ignore */ }
          }
          // Only nudge the list when files were actually created/renamed/deleted,
          // not on every content edit — otherwise the sidebar refetches and flickers.
          setListRefresh((x) => x + 1);
        }
        lastSavedNameRef.current = name;
        localStorage.setItem(LAST_PIPELINE_KEY, name);
      } catch (e) {
        pushError({ message: 'Autosave failed', details: String(e) });
      }
    }, AUTOSAVE_DEBOUNCE_MS);

    return () => clearTimeout(t);
  }, [nodes, edges, pipelineName, dirty, pushError]);

  async function onPickPipeline(name: string) {
    try {
      const p = await api.getPipeline(name);
      loadPipeline(p);
      lastSavedNameRef.current = name;
      localStorage.setItem(LAST_PIPELINE_KEY, name);
      pushToast({ level: 'info', message: `Loaded ${name}` });
    } catch (e) {
      pushError({ message: `Open ${name} failed`, details: String(e) });
    }
  }

  async function onCancelEdit() {
    const orig = useEditorStore.getState().originalState;
    if (!orig) return;
    const cur = lastSavedNameRef.current;
    const target = (orig.name && orig.name !== 'untitled') ? orig.name : '';

    // If reverting to untitled and we have an autosaved file, delete it ourselves
    // (autosave won't fire since target name is untitled).
    if (!target && cur && cur !== 'untitled') {
      try { await api.deletePipeline(cur); } catch {}
      lastSavedNameRef.current = '';
      localStorage.removeItem(LAST_PIPELINE_KEY);
      setListRefresh((x) => x + 1);
    }
    revertToSnapshot();
    pushToast({ level: 'info', message: 'Reverted' });
    // If target is non-untitled and differs from cur, autosave will save under target and clean up cur.
  }

  function onNew() {
    newPipeline();
    lastSavedNameRef.current = '';
    localStorage.removeItem(LAST_PIPELINE_KEY);
  }

  async function onDeletePipeline(name: string) {
    try {
      await api.deletePipeline(name);
      pushToast({ level: 'info', message: `Deleted ${name}` });
      if (pipelineName.trim() === name) {
        newPipeline();
        lastSavedNameRef.current = '';
        localStorage.removeItem(LAST_PIPELINE_KEY);
      }
      setListRefresh((x) => x + 1);
    } catch (e) {
      pushError({ message: `Delete ${name} failed`, details: String(e) });
    }
  }

  async function onPlay(targetNodeId: string) {
    if (useExecutionStore.getState().runningRequests.has(targetNodeId)) return;
    const p = toPipeline();
    const planned = new Set<string>([targetNodeId]);
    const stack = [targetNodeId];
    while (stack.length) {
      const id = stack.pop()!;
      for (const e of p.edges) {
        if (e.toNodeId === id && !planned.has(e.fromNodeId)) {
          planned.add(e.fromNodeId);
          stack.push(e.fromNodeId);
        }
      }
    }
    for (const id of planned) execClearNode(id);
    execSetQueued(planned);

    const ac = new AbortController();
    execStartRequest(targetNodeId, ac);
    try {
      await api.execute({ pipeline: p, targetNodeId }, (event) => {
        if (event.type === 'node-state') execSetState(event.nodeId, event.state, event.error);
        else if (event.type === 'node-output') execSetOutput(event.nodeId, { mediaType: event.mediaType, url: event.url, text: event.text });
        else if (event.type === 'log') execLog(event.message);
        else if (event.type === 'fatal') {
          if (event.error === 'Cancelled') {
            pushToast({ level: 'info', message: 'Cancelled' });
          } else {
            pushError({ message: 'Pipeline failed', details: event.error });
          }
        }
      }, ac.signal);
    } catch (e: any) {
      if (e?.name !== 'AbortError') {
        pushError({ message: 'Execute failed', details: String(e) });
      }
    } finally {
      execEndRequest(targetNodeId);
    }
  }

  function onStop(nodeId: string) {
    execAbortRequest(nodeId);
  }

  function playAllFinishes(p: ReturnType<typeof toPipeline>) {
    const finishes = p.nodes.filter((n) => n.type === 'finishText' || n.type === 'finishImage');
    if (!finishes.length) {
      pushToast({ level: 'info', message: 'No finish nodes in pipeline' });
      return;
    }
    for (const f of finishes) void onPlay(f.id);
  }

  function onPlayAll() { playAllFinishes(toPipeline()); }

  async function onPlayPipeline(name: string) {
    const trimmed = name.trim();
    const current = pipelineName.trim();
    if (!trimmed || trimmed === current) {
      playAllFinishes(toPipeline());
      return;
    }
    try {
      const p = await api.getPipeline(trimmed);
      loadPipeline(p);
      lastSavedNameRef.current = trimmed;
      localStorage.setItem(LAST_PIPELINE_KEY, trimmed);
      playAllFinishes(p);
    } catch (e) {
      pushError({ message: `Play ${trimmed} failed`, details: String(e) });
    }
  }

  return (
    <div className="editor-root">
      <PipelineSidebar
        refreshTrigger={listRefresh}
        onPickPipeline={onPickPipeline}
        onCancelEdit={onCancelEdit}
        onNew={onNew}
        onDelete={onDeletePipeline}
        onPlayAll={onPlayAll}
        onPlayPipeline={onPlayPipeline}
      />
      <div className="editor-toolbar">
        <span style={{ color: 'var(--fg-dim)' }}>
          Right-click for menu · Play on every node · Auto-saves while you edit
        </span>
        <div className="spacer" />
        {dirty && <span style={{ color: 'var(--yellow)' }}>● unsaved revert available</span>}
        <span style={{ color: 'var(--fg-dim)' }}>{pipelineName.trim() || 'untitled'}</span>
      </div>

      <Canvas onPlay={onPlay} onStop={onStop} />
      <ContextMenus />
    </div>
  );
}
