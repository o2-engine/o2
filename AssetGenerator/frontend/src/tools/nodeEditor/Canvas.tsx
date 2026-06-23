import React, { useEffect, useRef, useState } from 'react';
import type { PortType } from '../../../../shared/types.js';
import { useEditorStore } from './editorStore.js';
import { useExecutionStore } from './executionStore.js';
import { screenToWorld, zoomAt } from './camera.js';
import { GRID_STEP } from './constants.js';
import { NODE_MIN_HEIGHT, NODE_MIN_WIDTH, nodeBounds, portPosition, rectsIntersect } from './geometry.js';
import { NodeView } from './NodeView.js';
import { EdgeView, PreviewEdge } from './EdgeView.js';
import { copySelection, hasClipboard, pasteClipboard } from './clipboard.js';
import type { ResizeEdges } from './NodeView.js';
import { useToastStore } from '../../common/toastStore.js';

type DragMode =
  | null
  | { kind: 'pan'; startScreen: { x: number; y: number }; startCam: { x: number; y: number } }
  | { kind: 'select-box'; startWorld: { x: number; y: number }; current: { x: number; y: number } }
  | { kind: 'drag-nodes'; startWorld: { x: number; y: number }; last: { x: number; y: number } }
  | { kind: 'edge'; fromNodeId: string; fromPortId: string; fromPortType: PortType; fromSide: 'output' | 'input'; cursorWorld: { x: number; y: number } }
  | { kind: 'resize-node'; nodeId: string; startWorld: { x: number; y: number }; startWidth: number; startHeight: number; startPos: { x: number; y: number }; edges: ResizeEdges };

interface Props {
  onPlay(targetNodeId: string): void;
  onStop(nodeId: string): void;
}

export function Canvas({ onPlay, onStop }: Props): JSX.Element {
  const svgRef = useRef<SVGSVGElement>(null);
  const hostRef = useRef<HTMLDivElement>(null);

  const nodes = useEditorStore((s) => s.nodes);
  const edges = useEditorStore((s) => s.edges);
  const selection = useEditorStore((s) => s.selection);
  const camera = useEditorStore((s) => s.camera);
  const schemas = useEditorStore((s) => s.schemas);
  const setCamera = useEditorStore((s) => s.setCamera);
  const setSelection = useEditorStore((s) => s.setSelection);
  const toggleSelection = useEditorStore((s) => s.toggleSelection);
  const clearSelection = useEditorStore((s) => s.clearSelection);
  const moveNodes = useEditorStore((s) => s.moveNodes);
  const snapSelectionToGrid = useEditorStore((s) => s.snapSelectionToGrid);
  const addEdge = useEditorStore((s) => s.addEdge);
  const resizeNode = useEditorStore((s) => s.resizeNode);
  const openContextMenu = useEditorStore((s) => s.openContextMenu);
  const closeContextMenu = useEditorStore((s) => s.closeContextMenu);
  const removeNodes = useEditorStore((s) => s.removeNodes);
  const appendPasted = useEditorStore((s) => s.appendPasted);

  const runStates = useExecutionStore((s) => s.states);
  const runErrors = useExecutionStore((s) => s.errors);

  const [drag, setDragReact] = useState<DragMode>(null);
  const dragRef = useRef<DragMode>(null);
  const suppressNextContextRef = useRef(false);

  // Synchronous drag setter: updates ref immediately so listeners see fresh state
  // without waiting for React re-render.
  const updateDrag = (next: DragMode) => { dragRef.current = next; setDragReact(next); };

  useEffect(() => { hostRef.current?.focus(); }, []);

  // Refs to the latest move/up impls so window listeners use fresh closures.
  const moveRef = useRef<(cx: number, cy: number) => void>(() => {});
  const upRef = useRef<(cx: number, cy: number) => void>(() => {});

  // Single global listener registered once on mount.
  // No double-fire with React's SVG handlers because we removed those.
  useEffect(() => {
    const onMove = (ev: MouseEvent) => {
      if (!dragRef.current) return;
      moveRef.current(ev.clientX, ev.clientY);
    };
    const onUp = (ev: MouseEvent) => {
      if (!dragRef.current) return;
      upRef.current(ev.clientX, ev.clientY);
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, []);

  function svgPoint(e: { clientX: number; clientY: number }): { x: number; y: number } {
    const rect = svgRef.current!.getBoundingClientRect();
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  }

  function pointFromClient(clientX: number, clientY: number): { x: number; y: number } {
    return svgPoint({ clientX, clientY });
  }

  function onWheel(e: React.WheelEvent) {
    e.preventDefault();
    const p = svgPoint(e);
    const factor = e.deltaY < 0 ? 1.1 : 1 / 1.1;
    setCamera(zoomAt(camera, p.x, p.y, factor));
  }

  function onMouseDownSvg(e: React.MouseEvent) {
    if (e.button === 2) {
      const start = svgPoint(e);
      updateDrag({ kind: 'pan', startScreen: start, startCam: { x: camera.x, y: camera.y } });
      suppressNextContextRef.current = false;
      closeContextMenu();
      return;
    }
    if (e.button !== 0) return;
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    closeContextMenu();
    clearSelection();
    updateDrag({ kind: 'select-box', startWorld: world, current: world });
  }

  function onNodeMouseDown(e: React.MouseEvent, nodeId: string) {
    if (e.button !== 0) return;
    e.stopPropagation();
    closeContextMenu();
    if (e.shiftKey) { toggleSelection(nodeId); return; }
    if (!selection.has(nodeId)) setSelection([nodeId]);
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    updateDrag({ kind: 'drag-nodes', startWorld: world, last: world });
  }

  function onResizeMouseDown(e: React.MouseEvent, nodeId: string, edges: ResizeEdges) {
    if (e.button !== 0) return;
    e.stopPropagation();
    const node = nodes.find((n) => n.id === nodeId);
    if (!node) return;
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    const b = nodeBounds(node);
    updateDrag({
      kind: 'resize-node',
      nodeId,
      startWorld: world,
      startWidth: b.w,
      startHeight: b.h,
      startPos: { x: node.position.x, y: node.position.y },
      edges,
    });
    closeContextMenu();
  }

  function onShowNodeError(nodeId: string, error: string) {
    useToastStore.getState().push({ level: 'error', message: `Node error`, details: error });
  }

  function onPortMouseDown(e: React.MouseEvent, info: { nodeId: string; portId: string; side: 'input' | 'output' }) {
    if (e.button !== 0) return;
    const node = nodes.find((n) => n.id === info.nodeId);
    if (!node) return;
    const port = info.side === 'output'
      ? node.outputs.find((p) => p.id === info.portId)
      : node.inputs.find((p) => p.id === info.portId);
    if (!port) return;
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    updateDrag({ kind: 'edge', fromNodeId: info.nodeId, fromPortId: info.portId, fromPortType: port.type, fromSide: info.side, cursorWorld: world });
    closeContextMenu();
  }

  function onPortMouseUp(e: React.MouseEvent, info: { nodeId: string; portId: string; side: 'input' | 'output' }) {
    const drag = dragRef.current;
    if (drag?.kind !== 'edge') return;
    if (info.nodeId === drag.fromNodeId) { updateDrag(null); return; }
    const targetNode = nodes.find((n) => n.id === info.nodeId);
    if (!targetNode) { updateDrag(null); return; }

    if (drag.fromSide === 'output' && info.side === 'input') {
      const port = targetNode.inputs.find((p) => p.id === info.portId);
      if (!port || port.type !== drag.fromPortType) { updateDrag(null); return; }
      addEdge(drag.fromNodeId, drag.fromPortId, info.nodeId, info.portId);
    } else if (drag.fromSide === 'input' && info.side === 'output') {
      const port = targetNode.outputs.find((p) => p.id === info.portId);
      if (!port || port.type !== drag.fromPortType) { updateDrag(null); return; }
      addEdge(info.nodeId, info.portId, drag.fromNodeId, drag.fromPortId);
    }
    updateDrag(null);
  }

  function onMouseMoveImpl(clientX: number, clientY: number) {
    const drag = dragRef.current;
    if (!drag) return;
    const screen = pointFromClient(clientX, clientY);
    const world = screenToWorld(camera, screen.x, screen.y);
    if (drag.kind === 'pan') {
      const dx = screen.x - drag.startScreen.x;
      const dy = screen.y - drag.startScreen.y;
      if (dx * dx + dy * dy > 9) suppressNextContextRef.current = true;
      setCamera({ ...camera, x: drag.startCam.x + dx, y: drag.startCam.y + dy });
    } else if (drag.kind === 'select-box') {
      updateDrag({ ...drag, current: world });
    } else if (drag.kind === 'drag-nodes') {
      const dx = world.x - drag.last.x;
      const dy = world.y - drag.last.y;
      moveNodes([...selection], dx, dy);
      updateDrag({ ...drag, last: world });
    } else if (drag.kind === 'edge') {
      updateDrag({ ...drag, cursorWorld: world });
    } else if (drag.kind === 'resize-node') {
      const dx = world.x - drag.startWorld.x;
      const dy = world.y - drag.startWorld.y;
      let w = drag.startWidth;
      let h = drag.startHeight;
      let px = drag.startPos.x;
      let py = drag.startPos.y;
      if (drag.edges.e) w = drag.startWidth + dx;
      if (drag.edges.w) { w = drag.startWidth - dx; px = drag.startPos.x + dx; }
      if (drag.edges.s) h = drag.startHeight + dy;
      if (drag.edges.n) { h = drag.startHeight - dy; py = drag.startPos.y + dy; }
      if (w < NODE_MIN_WIDTH) {
        const diff = NODE_MIN_WIDTH - w;
        w = NODE_MIN_WIDTH;
        if (drag.edges.w) px -= diff;
      }
      if (h < NODE_MIN_HEIGHT) {
        const diff = NODE_MIN_HEIGHT - h;
        h = NODE_MIN_HEIGHT;
        if (drag.edges.n) py -= diff;
      }
      resizeNode(drag.nodeId, w, h, { x: px, y: py });
    }
  }

  function onMouseUpImpl(clientX: number, clientY: number) {
    const drag = dragRef.current;
    if (!drag) return;
    if (drag.kind === 'select-box') {
      const a = drag.startWorld;
      const b = drag.current;
      const box = { x: Math.min(a.x, b.x), y: Math.min(a.y, b.y), w: Math.abs(a.x - b.x), h: Math.abs(a.y - b.y) };
      const picked = nodes.filter((n) => rectsIntersect(nodeBounds(n), box)).map((n) => n.id);
      if (picked.length || box.w > 2 || box.h > 2) setSelection(picked);
    } else if (drag.kind === 'drag-nodes') {
      snapSelectionToGrid();
    } else if (drag.kind === 'edge') {
      const screen = pointFromClient(clientX, clientY);
      const world = screenToWorld(camera, screen.x, screen.y);
      openContextMenu({
        screenX: clientX,
        screenY: clientY,
        kind: 'create-from-edge',
        worldX: world.x,
        worldY: world.y,
        pendingEdge: {
          fromNodeId: drag.fromNodeId,
          fromPortId: drag.fromPortId,
          portType: drag.fromPortType,
          side: drag.fromSide,
        },
      });
    }
    updateDrag(null);
  }

  // Keep refs pointing at latest impls so window listeners use fresh closures
  moveRef.current = onMouseMoveImpl;
  upRef.current = onMouseUpImpl;

  function onSvgContext(e: React.MouseEvent) {
    e.preventDefault();
    if (suppressNextContextRef.current) { suppressNextContextRef.current = false; return; }
    if (drag?.kind === 'pan') return;
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    openContextMenu({ screenX: e.clientX, screenY: e.clientY, kind: 'background', worldX: world.x, worldY: world.y });
  }

  function onNodeContext(e: React.MouseEvent, nodeId: string) {
    e.preventDefault();
    e.stopPropagation();
    if (suppressNextContextRef.current) { suppressNextContextRef.current = false; return; }
    if (!selection.has(nodeId)) setSelection([nodeId]);
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    openContextMenu({ screenX: e.clientX, screenY: e.clientY, kind: 'node', targetId: nodeId, worldX: world.x, worldY: world.y });
  }

  function onEdgeContext(e: React.MouseEvent, edgeId: string) {
    e.preventDefault();
    e.stopPropagation();
    if (suppressNextContextRef.current) { suppressNextContextRef.current = false; return; }
    const screen = svgPoint(e);
    const world = screenToWorld(camera, screen.x, screen.y);
    openContextMenu({ screenX: e.clientX, screenY: e.clientY, kind: 'edge', targetId: edgeId, worldX: world.x, worldY: world.y });
  }

  function onKeyDown(e: React.KeyboardEvent) {
    const target = e.target as HTMLElement | null;
    const tag = target?.tagName?.toLowerCase();
    if (tag === 'input' || tag === 'textarea' || tag === 'select' || target?.isContentEditable) {
      return;
    }
    const ctrl = e.ctrlKey || e.metaKey;
    if (ctrl && e.key.toLowerCase() === 'a') {
      e.preventDefault();
      setSelection(nodes.map((n) => n.id));
    } else if (ctrl && e.key.toLowerCase() === 'c') {
      copySelection(nodes, edges, selection);
    } else if (ctrl && e.key.toLowerCase() === 'v') {
      if (!hasClipboard()) return;
      const { nodes: nn, edges: ne, newIds } = pasteClipboard({ x: GRID_STEP, y: GRID_STEP });
      appendPasted(nn, ne, newIds);
    } else if (ctrl && e.key.toLowerCase() === 'd') {
      e.preventDefault();
      copySelection(nodes, edges, selection);
      const { nodes: nn, edges: ne, newIds } = pasteClipboard({ x: GRID_STEP, y: GRID_STEP });
      appendPasted(nn, ne, newIds);
    } else if (e.key === 'Delete' || e.key === 'Backspace') {
      if (selection.size) removeNodes([...selection]);
    }
  }

  const t = `translate(${camera.x} ${camera.y}) scale(${camera.scale})`;
  const selBox = drag?.kind === 'select-box' ? {
    x: Math.min(drag.startWorld.x, drag.current.x),
    y: Math.min(drag.startWorld.y, drag.current.y),
    w: Math.abs(drag.startWorld.x - drag.current.x),
    h: Math.abs(drag.startWorld.y - drag.current.y),
  } : null;

  const previewEdge = drag?.kind === 'edge'
    ? (() => {
        const n = nodes.find((nn) => nn.id === drag.fromNodeId);
        if (!n) return null;
        const p = drag.fromSide === 'output'
          ? n.outputs.find((pp) => pp.id === drag.fromPortId)
          : n.inputs.find((pp) => pp.id === drag.fromPortId);
        if (!p) return null;
        const portPos = portPosition(n, p, drag.fromSide);
        const start = drag.fromSide === 'output' ? portPos : drag.cursorWorld;
        const end = drag.fromSide === 'output' ? drag.cursorWorld : portPos;
        return { start, end, type: drag.fromPortType };
      })()
    : null;

  return (
    <div className="canvas-host" ref={hostRef} tabIndex={0} onKeyDown={onKeyDown}>
      <svg
        ref={svgRef}
        onWheel={onWheel}
        onMouseDown={onMouseDownSvg}
        onContextMenu={onSvgContext}
      >
        <defs>
          <pattern id="grid-pattern" width={GRID_STEP * camera.scale} height={GRID_STEP * camera.scale} patternUnits="userSpaceOnUse" x={camera.x} y={camera.y}>
            <circle cx={1} cy={1} r={1} fill="#3a3d44" />
          </pattern>
        </defs>
        <rect x={0} y={0} width="100%" height="100%" fill="url(#grid-pattern)" />

        <g transform={t}>
          {edges.map((e) => (
            <EdgeView
              key={e.id}
              edge={e}
              nodes={nodes}
              fromState={runStates.get(e.fromNodeId)}
              toState={runStates.get(e.toNodeId)}
              onContext={onEdgeContext}
            />
          ))}

          {previewEdge && (
            <PreviewEdge start={previewEdge.start} end={previewEdge.end} type={previewEdge.type} />
          )}

          {nodes.map((n) => {
            const schema = schemas.find((s) => s.type === n.type);
            return (
              <NodeView
                key={n.id}
                node={n}
                schema={schema}
                selected={selection.has(n.id)}
                runState={runStates.get(n.id) ?? 'idle'}
                runError={runErrors.get(n.id)}
                onNodeMouseDown={onNodeMouseDown}
                onNodeContext={onNodeContext}
                onPortMouseDown={onPortMouseDown}
                onPortMouseUp={onPortMouseUp}
                onResizeMouseDown={onResizeMouseDown}
                onPlay={onPlay}
                onStop={onStop}
                onShowNodeError={onShowNodeError}
              />
            );
          })}

          {selBox && (
            <rect className="selection-rect" x={selBox.x} y={selBox.y} width={selBox.w} height={selBox.h} />
          )}
        </g>
      </svg>
    </div>
  );
}
