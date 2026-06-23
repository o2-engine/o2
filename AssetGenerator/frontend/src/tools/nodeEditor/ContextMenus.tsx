import React from 'react';
import type { NodeTypeSchema } from '../../../../shared/types.js';
import { useEditorStore } from './editorStore.js';
import { copySelection, hasClipboard, pasteClipboard } from './clipboard.js';
import { GRID_STEP } from './constants.js';
import { nodeIcon } from './icons.js';

const CATEGORY_ORDER: NodeTypeSchema['category'][] = ['source', 'transform', 'ai', 'output', 'flow'];
const CATEGORY_LABEL: Record<NodeTypeSchema['category'], string> = {
  flow: 'Flow',
  source: 'Source',
  transform: 'Transform',
  ai: 'AI',
  output: 'Output',
};

export function ContextMenus(): JSX.Element | null {
  const menu = useEditorStore((s) => s.contextMenu);
  const close = useEditorStore((s) => s.closeContextMenu);
  const nodes = useEditorStore((s) => s.nodes);
  const edges = useEditorStore((s) => s.edges);
  const selection = useEditorStore((s) => s.selection);
  const schemas = useEditorStore((s) => s.schemas);
  const removeNodes = useEditorStore((s) => s.removeNodes);
  const removeEdges = useEditorStore((s) => s.removeEdges);
  const appendPasted = useEditorStore((s) => s.appendPasted);
  const setSelection = useEditorStore((s) => s.setSelection);
  const addNodeAt = useEditorStore((s) => s.addNodeAt);
  const addNodeFromPendingEdge = useEditorStore((s) => s.addNodeFromPendingEdge);

  if (!menu) return null;

  const stop: React.MouseEventHandler = (e) => e.stopPropagation();

  const doCopy = (ids: string[]) => { copySelection(nodes, edges, new Set(ids)); close(); };
  const doPaste = () => {
    const { nodes: nn, edges: ne, newIds } = pasteClipboard({ x: GRID_STEP, y: GRID_STEP });
    appendPasted(nn, ne, newIds);
    close();
  };
  const doDuplicate = (ids: string[]) => {
    copySelection(nodes, edges, new Set(ids));
    const { nodes: nn, edges: ne, newIds } = pasteClipboard({ x: GRID_STEP, y: GRID_STEP });
    appendPasted(nn, ne, newIds);
    close();
  };
  const doDelete = (ids: string[]) => { removeNodes(ids); close(); };

  const onAddType = (type: string) => {
    if (menu.pendingEdge) {
      addNodeFromPendingEdge(type, { x: menu.worldX, y: menu.worldY }, menu.pendingEdge);
    } else {
      addNodeAt(type, { x: menu.worldX, y: menu.worldY });
    }
    close();
  };

  const compatibleFilter = (s: NodeTypeSchema): boolean => {
    if (!menu.pendingEdge) return true;
    if (menu.pendingEdge.side === 'output') {
      return s.inputs.some((p) => p.type === menu.pendingEdge!.portType) || s.type === 'promptCompose';
    }
    return s.outputs.some((p) => p.type === menu.pendingEdge!.portType);
  };

  const visibleSchemas = schemas.filter(compatibleFilter);
  const grouped = new Map<NodeTypeSchema['category'], NodeTypeSchema[]>();
  for (const s of visibleSchemas) {
    const arr = grouped.get(s.category) ?? [];
    arr.push(s);
    grouped.set(s.category, arr);
  }

  const showAddSection = menu.kind === 'background' || menu.kind === 'create-from-edge';

  return (
    <div className="context-menu" style={{ left: menu.screenX, top: menu.screenY }} onMouseDown={stop}>
      {menu.kind === 'create-from-edge' && (
        <div className="section">Add node {menu.pendingEdge?.side === 'output' ? '(accepts ' : '(produces '}{menu.pendingEdge?.portType})</div>
      )}

      {showAddSection && CATEGORY_ORDER.map((cat) => {
        const items = grouped.get(cat);
        if (!items?.length) return null;
        return (
          <React.Fragment key={cat}>
            {menu.kind === 'background' && <div className="section">Add: {CATEGORY_LABEL[cat]}</div>}
            {items.map((s) => (
              <button key={s.type} onClick={() => onAddType(s.type)} title={s.description}>
                <span className="item-icon">{nodeIcon(s.type, { size: 14 })}</span>
                {s.label}
              </button>
            ))}
          </React.Fragment>
        );
      })}

      {menu.kind === 'background' && (
        <>
          <hr />
          <button onClick={doPaste} disabled={!hasClipboard()}>
            <span className="item-icon" />
            Paste<span className="shortcut">Ctrl+V</span>
          </button>
          <button onClick={() => { setSelection(nodes.map((n) => n.id)); close(); }}>
            <span className="item-icon" />
            Select all<span className="shortcut">Ctrl+A</span>
          </button>
          {selection.size > 0 && <>
            <button onClick={() => doCopy([...selection])}>
              <span className="item-icon" />
              Copy selection<span className="shortcut">Ctrl+C</span>
            </button>
            <button onClick={() => doDelete([...selection])}>
              <span className="item-icon" />
              Delete selection<span className="shortcut">Del</span>
            </button>
          </>}
        </>
      )}

      {menu.kind === 'node' && menu.targetId && (
        <>
          <button onClick={() => doDuplicate([menu.targetId!])}>
            <span className="item-icon" />
            Duplicate<span className="shortcut">Ctrl+D</span>
          </button>
          <button onClick={() => doCopy([menu.targetId!])}>
            <span className="item-icon" />
            Copy<span className="shortcut">Ctrl+C</span>
          </button>
          <hr />
          <button onClick={() => doDelete([menu.targetId!])}>
            <span className="item-icon" />
            Delete<span className="shortcut">Del</span>
          </button>
        </>
      )}

      {menu.kind === 'edge' && menu.targetId && (
        <button onClick={() => { removeEdges([menu.targetId!]); close(); }}>
          <span className="item-icon" />
          Delete edge<span className="shortcut">Del</span>
        </button>
      )}
    </div>
  );
}

