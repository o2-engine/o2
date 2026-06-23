import React, { useRef, useState } from 'react';
import type { NodeData } from '../../../../shared/types.js';
import { useEditorStore } from './editorStore.js';
import { ImagePreview } from '../../common/ImagePreview.js';
import { useToastStore } from '../../common/toastStore.js';
import { useExecutionStore } from './executionStore.js';
import { systemSaveFromUrl } from '../../common/systemSave.js';
import { TrashIcon, DownloadIcon } from './icons.js';
import { newSlot, type Slot } from './dynamicPorts.js';

export interface NodeRenderer {
  bodyHeight(node: NodeData): number;
  renderBody(node: NodeData): JSX.Element | null;
}

function useUpdate(nodeId: string) {
  const fn = useEditorStore((s) => s.updateNodeConfig);
  return (patch: Record<string, unknown>) => fn(nodeId, patch);
}

function NoBody(): null { return null; }

const stop = (e: React.MouseEvent | React.PointerEvent | React.WheelEvent) => e.stopPropagation();
const commonProps = () => ({
  onMouseDown: stop, onMouseUp: stop, onMouseMove: stop,
  onWheel: stop, onPointerDown: stop,
});

function readAsBase64(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const r = new FileReader();
    r.onload = () => {
      const result = String(r.result ?? '');
      const comma = result.indexOf(',');
      resolve(comma >= 0 ? result.slice(comma + 1) : result);
    };
    r.onerror = () => reject(r.error);
    r.readAsDataURL(file);
  });
}

function readAsText(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const r = new FileReader();
    r.onload = () => resolve(String(r.result ?? ''));
    r.onerror = () => reject(r.error);
    r.readAsText(file);
  });
}

function sanitizeForPath(name: string): string {
  return name.replace(/[^a-zA-Z0-9._-]/g, '_');
}

function FinishCommonBody({ node, accept, suggestSuffix, valueKind }: {
  node: NodeData;
  accept: string;
  suggestSuffix: string;
  valueKind: 'image' | 'text';
}): JSX.Element {
  const update = useUpdate(node.id);
  const pushToast = useToastStore((s) => s.push);
  const output = useExecutionStore((s) => s.outputs.get(node.id));
  const root = String(node.config.targetRoot ?? (valueKind === 'image' ? 'assets' : 'contentDb')) as 'assets' | 'contentDb';
  const path = String(node.config.path ?? '');

  const onSaveCopy = async () => {
    if (!output?.url) {
      pushToast({ level: 'error', message: 'No output yet — run the pipeline first' });
      return;
    }
    const lastSlash = path.lastIndexOf('/');
    const suggested = (lastSlash >= 0 ? path.slice(lastSlash + 1) : path) || `output${suggestSuffix}`;
    try {
      const ok = await systemSaveFromUrl(output.url, suggested, accept);
      if (ok) pushToast({ level: 'info', message: `Saved copy as ${suggested}` });
    } catch (e) {
      pushToast({ level: 'error', message: 'System save failed', details: String(e) });
    }
  };

  return (
    <div className="node-body">
      <div className="row">
        <label>Root</label>
        <select value={root} onChange={(e) => update({ targetRoot: e.target.value })} {...commonProps()}>
          <option value="assets">assets</option>
          <option value="contentDb">contentDb</option>
        </select>
      </div>
      <div className="row">
        <label>Path</label>
        <input
          type="text"
          value={path}
          placeholder={valueKind === 'image' ? 'sprites/cat.png' : 'NodeImageGen/out/text.txt'}
          onChange={(e) => update({ path: e.target.value })}
          {...commonProps()}
        />
      </div>
      <div className="picker-actions">
        <button onClick={onSaveCopy} disabled={!output?.url} {...commonProps()} title="System Save As dialog">
          <span style={{ display: 'inline-flex', verticalAlign: 'middle', marginRight: 4 }}><DownloadIcon size={12} /></span>
          Save copy to system…
        </button>
      </div>
      {valueKind === 'image' ? (
        output?.url ? (
          <div
            className="node-output image"
            style={{
              backgroundImage: `url("${output.url}")`,
              backgroundSize: 'contain', backgroundRepeat: 'no-repeat', backgroundPosition: 'center',
            }}
            title="Last saved input"
          />
        ) : (
          <div className="node-output empty">no preview yet — Play to render</div>
        )
      ) : (
        output?.text != null
          ? <pre className="node-output text">{output.text}</pre>
          : <div className="node-output empty">no preview yet — Play to render</div>
      )}
    </div>
  );
}

function FinishTextBody({ node }: { node: NodeData }): JSX.Element {
  return <FinishCommonBody node={node} accept="text/plain" suggestSuffix=".txt" valueKind="text" />;
}

function FinishImageBody({ node }: { node: NodeData }): JSX.Element {
  return <FinishCommonBody node={node} accept="image/png" suggestSuffix=".png" valueKind="image" />;
}

function SourceTextBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const text = String(node.config.text ?? '');
  return (
    <div className="node-body">
      <textarea
        value={text}
        placeholder="Inline text…"
        onChange={(e) => update({ text: e.target.value })}
        {...commonProps()}
        style={{ flex: 1, minHeight: 0 }}
      />
    </div>
  );
}

function SourceTextFileBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const pushToast = useToastStore((s) => s.push);
  const fileRef = useRef<HTMLInputElement>(null);
  const text = String(node.config.text ?? '');
  const sourceName = String(node.config.sourceName ?? '');

  const onPick = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    try {
      const content = await readAsText(file);
      update({ text: content, sourceName: file.name });
      pushToast({ level: 'info', message: `Loaded ${file.name} (${content.length} chars)` });
    } catch (err) {
      pushToast({ level: 'error', message: 'Load failed', details: String(err) });
    } finally {
      if (fileRef.current) fileRef.current.value = '';
    }
  };

  return (
    <div className="node-body">
      <div className="picker-actions">
        <button onClick={() => fileRef.current?.click()} {...commonProps()}>
          📂 {sourceName ? 'Reload…' : 'Pick text file…'}
        </button>
        <input
          ref={fileRef}
          type="file"
          accept=".txt,.md,.json,.yaml,.yml,.csv,.tsv,.log,.xml,.html,.css,.js,.ts"
          style={{ display: 'none' }}
          onChange={onPick}
          {...commonProps()}
        />
      </div>
      {sourceName && <div className="muted-line">📄 {sourceName} · {text.length} chars</div>}
      <textarea
        value={text}
        readOnly
        placeholder="(pick a file to load text)"
        {...commonProps()}
        style={{ flex: 1, minHeight: 0, color: 'var(--fg-dim)' }}
      />
    </div>
  );
}

function SourceImageBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const pushToast = useToastStore((s) => s.push);
  const fileRef = useRef<HTMLInputElement>(null);
  const [uploading, setUploading] = useState(false);
  const root = String(node.config.sourceRoot ?? 'contentDb') as 'assets' | 'contentDb';
  const path = String(node.config.filePath ?? '');

  const onPick = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setUploading(true);
    try {
      const base64 = await readAsBase64(file);
      const targetPath = `_uploads/${Date.now()}-${sanitizeForPath(file.name)}`;
      const res = await fetch('/api/files/write', {
        method: 'PUT',
        headers: { 'content-type': 'application/json' },
        body: JSON.stringify({ root: 'contentDb', path: targetPath, contentBase64: base64 }),
      });
      if (!res.ok) throw new Error(`HTTP ${res.status}: ${await res.text()}`);
      update({ sourceRoot: 'contentDb', filePath: targetPath });
      pushToast({ level: 'info', message: `Uploaded ${file.name}` });
    } catch (err) {
      pushToast({ level: 'error', message: 'Upload failed', details: String(err) });
    } finally {
      setUploading(false);
      if (fileRef.current) fileRef.current.value = '';
    }
  };

  return (
    <div className="node-body image-body">
      <div className="row">
        <label>Root</label>
        <select value={root} onChange={(e) => update({ sourceRoot: e.target.value })} {...commonProps()}>
          <option value="assets">assets</option>
          <option value="contentDb">contentDb</option>
        </select>
      </div>
      <div className="row">
        <label>Path</label>
        <input
          type="text"
          value={path}
          placeholder="NodeImageGen/refs/sample.png"
          onChange={(e) => update({ filePath: e.target.value })}
          {...commonProps()}
        />
      </div>
      <div className="picker-actions">
        <button onClick={() => fileRef.current?.click()} disabled={uploading} {...commonProps()}>
          {uploading ? '…uploading' : '📂 Pick & upload image…'}
        </button>
        <input
          ref={fileRef}
          type="file"
          accept="image/*"
          style={{ display: 'none' }}
          onChange={onPick}
          {...commonProps()}
        />
      </div>
      <div className="image-preview-wrap" {...commonProps()}>
        <ImagePreview root={root} path={path} fill />
      </div>
    </div>
  );
}

function SlotList({ slots, addLabel, placeholder, onChange }: {
  slots: Slot[];
  addLabel: string;
  placeholder: string;
  onChange(next: Slot[]): void;
}): JSX.Element {
  const setName = (id: string, name: string) => onChange(slots.map((s) => s.id === id ? { ...s, name } : s));
  const remove = (id: string) => onChange(slots.filter((s) => s.id !== id));
  const add = () => onChange([...slots, newSlot('')]);
  return (
    <div className="slot-list">
      {slots.map((s) => (
        <div key={s.id} className="slot-row">
          <input
            type="text"
            className="slot-name"
            value={s.name}
            placeholder={placeholder}
            onChange={(e) => setName(s.id, e.target.value)}
            {...commonProps()}
          />
          <button className="kv2-del" onClick={() => remove(s.id)} {...commonProps()} title="Remove">
            <TrashIcon size={12} />
          </button>
        </div>
      ))}
      <button className="kv-add" onClick={add} {...commonProps()}>{addLabel}</button>
    </div>
  );
}

function TextComposeBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const vars = (node.config.vars as Slot[] | undefined) ?? [];
  return (
    <div className="node-body">
      <div className="muted-line">Template via <b>template</b> input · each variable becomes a text input</div>
      <SlotList
        slots={vars}
        addLabel="+ var"
        placeholder="variable name (used as {name})"
        onChange={(next) => update({ vars: next })}
      />
      <NodeOutputPreview nodeId={node.id} kind="text" />
    </div>
  );
}

function TextConcatBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const parts = (node.config.parts as Slot[] | undefined) ?? [];
  const sep = String(node.config.separator ?? '');
  return (
    <div className="node-body">
      <div className="row">
        <label>Separator</label>
        <input
          type="text"
          value={sep}
          placeholder="(empty)"
          onChange={(e) => update({ separator: e.target.value })}
          {...commonProps()}
        />
      </div>
      <div className="muted-line">Parts joined in this order · each becomes a text input</div>
      <SlotList
        slots={parts}
        addLabel="+ part"
        placeholder="part name"
        onChange={(next) => update({ parts: next })}
      />
      <NodeOutputPreview nodeId={node.id} kind="text" />
    </div>
  );
}

const IMAGE_MODEL_PRESETS = [
  'gemini-2.5-flash-image',
  'gemini-2.5-flash-image-preview',
  'gemini-3-pro-image-preview',
  'imagen-4.0-generate-001',
  'imagen-3.0-generate-001',
];

const TEXT_MODEL_PRESETS = [
  'gemini-2.5-flash',
  'gemini-2.5-pro',
  'gemini-3-pro',
];

function ModelDropdown({ value, presets, onChange }: { value: string; presets: string[]; onChange(v: string): void }): JSX.Element {
  const isPreset = presets.includes(value);
  const [custom, setCustom] = useState(!isPreset);
  if (custom) {
    return <input type="text" value={value} onChange={(e) => onChange(e.target.value)} {...commonProps()} />;
  }
  return (
    <select
      value={value}
      onChange={(e) => {
        const v = e.target.value;
        if (v === '__custom__') { setCustom(true); return; }
        onChange(v);
      }}
      {...commonProps()}
    >
      {presets.map((m) => <option key={m} value={m}>{m}</option>)}
      <option value="__custom__">Custom…</option>
    </select>
  );
}

function NodeOutputPreview({ nodeId, kind }: { nodeId: string; kind: 'image' | 'text' }): JSX.Element {
  const output = useExecutionStore((s) => s.outputs.get(nodeId));
  if (!output || output.mediaType !== kind) {
    return <div className="node-output empty">{kind === 'image' ? 'no output yet' : ''}</div>;
  }
  if (kind === 'image' && output.url) {
    return (
      <div
        className="node-output image"
        style={{
          backgroundImage: `url("${output.url}")`,
          backgroundSize: 'contain',
          backgroundRepeat: 'no-repeat',
          backgroundPosition: 'center',
        }}
        title="Last run output"
      />
    );
  }
  if (kind === 'text' && output.text != null) {
    return <pre className="node-output text">{output.text}</pre>;
  }
  return <div className="node-output empty">no output</div>;
}

function NanoBananaBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const model = String(node.config.model ?? IMAGE_MODEL_PRESETS[0]);
  const extra = String(node.config.extraPrompt ?? '');

  return (
    <div className="node-body">
      <div className="row">
        <label>Model</label>
        <ModelDropdown value={model} presets={IMAGE_MODEL_PRESETS} onChange={(v) => update({ model: v })} />
      </div>
      <textarea
        value={extra}
        placeholder="Extra prompt (optional style hints)"
        rows={2}
        onChange={(e) => update({ extraPrompt: e.target.value })}
        {...commonProps()}
      />
      <NodeOutputPreview nodeId={node.id} kind="image" />
    </div>
  );
}

function AiTextBody({ node }: { node: NodeData }): JSX.Element {
  const update = useUpdate(node.id);
  const model = String(node.config.model ?? TEXT_MODEL_PRESETS[0]);
  const sys = String(node.config.systemPrompt ?? '');
  return (
    <div className="node-body">
      <div className="row">
        <label>Model</label>
        <ModelDropdown value={model} presets={TEXT_MODEL_PRESETS} onChange={(v) => update({ model: v })} />
      </div>
      <textarea
        value={sys}
        placeholder="System prompt (optional)"
        rows={2}
        onChange={(e) => update({ systemPrompt: e.target.value })}
        {...commonProps()}
      />
      <NodeOutputPreview nodeId={node.id} kind="text" />
    </div>
  );
}

function RemoveBgBody({ node }: { node: NodeData }): JSX.Element {
  return (
    <div className="node-body">
      <NodeOutputPreview nodeId={node.id} kind="image" />
    </div>
  );
}

export const NODE_RENDERERS: Record<string, NodeRenderer> = {
  finishText: {
    bodyHeight: () => 200,
    renderBody: (node) => <FinishTextBody node={node} />,
  },
  finishImage: {
    bodyHeight: () => 220,
    renderBody: (node) => <FinishImageBody node={node} />,
  },
  sourceText: {
    bodyHeight: () => 120,
    renderBody: (node) => <SourceTextBody node={node} />,
  },
  sourceTextFile: {
    bodyHeight: () => 180,
    renderBody: (node) => <SourceTextFileBody node={node} />,
  },
  sourceImage: {
    bodyHeight: () => 230,
    renderBody: (node) => <SourceImageBody node={node} />,
  },
  textCompose: {
    bodyHeight: (node) => {
      const vars = (node.config.vars as Slot[] | undefined) ?? [];
      return 50 + vars.length * 30 + 32 + 70;
    },
    renderBody: (node) => <TextComposeBody node={node} />,
  },
  textConcat: {
    bodyHeight: (node) => {
      const parts = (node.config.parts as Slot[] | undefined) ?? [];
      return 80 + parts.length * 30 + 32 + 70;
    },
    renderBody: (node) => <TextConcatBody node={node} />,
  },
  nanoBananaGen: {
    bodyHeight: () => 230,
    renderBody: (node) => <NanoBananaBody node={node} />,
  },
  aiText: {
    bodyHeight: () => 200,
    renderBody: (node) => <AiTextBody node={node} />,
  },
  removeBackground: {
    bodyHeight: () => 180,
    renderBody: (node) => <RemoveBgBody node={node} />,
  },
};

export function bodyHeightFor(node: NodeData): number {
  return NODE_RENDERERS[node.type]?.bodyHeight(node) ?? 0;
}

export function renderBodyFor(node: NodeData): JSX.Element | null {
  return NODE_RENDERERS[node.type]?.renderBody(node) ?? null;
}
