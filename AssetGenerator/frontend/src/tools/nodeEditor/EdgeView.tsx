import React from 'react';
import type { EdgeData, NodeData } from '../../../../shared/types.js';
import { portPosition } from './geometry.js';
import type { NodeRunState } from './executionStore.js';

interface Props {
  edge: EdgeData;
  nodes: NodeData[];
  fromState?: NodeRunState;
  toState?: NodeRunState;
  onContext?(e: React.MouseEvent, edgeId: string): void;
}

export function EdgeView({ edge, nodes, fromState, toState, onContext }: Props): JSX.Element | null {
  const from = nodes.find((n) => n.id === edge.fromNodeId);
  const to = nodes.find((n) => n.id === edge.toNodeId);
  if (!from || !to) return null;
  const fromPort = from.outputs.find((p) => p.id === edge.fromPortId);
  const toPort = to.inputs.find((p) => p.id === edge.toPortId);
  if (!fromPort || !toPort) return null;

  const a = portPosition(from, fromPort, 'output');
  const b = portPosition(to, toPort, 'input');
  const dx = Math.max(40, Math.abs(b.x - a.x) * 0.5);
  const d = `M ${a.x} ${a.y} C ${a.x + dx} ${a.y}, ${b.x - dx} ${b.y}, ${b.x} ${b.y}`;

  // Highlight only when BOTH endpoints are part of an active execution.
  // Otherwise a node feeding many consumers would light up all its outgoing edges
  // when only one consumer actually requested its output.
  const fromActive = !!fromState && fromState !== 'idle';
  const toActive = !!toState && toState !== 'idle';
  const stateCls = fromActive && toActive ? `edge-state-${fromState}` : '';
  const cls = `edge-path ${fromPort.type} ${stateCls}`;

  return (
    <g className="edge">
      <path
        d={d}
        className="edge-hit"
        onContextMenu={(e) => { e.preventDefault(); onContext?.(e, edge.id); }}
      />
      <path d={d} className={cls} pointerEvents="none" />
    </g>
  );
}

export function PreviewEdge({ start, end, type }: { start: { x: number; y: number }; end: { x: number; y: number }; type: 'text' | 'image' }): JSX.Element {
  const dx = Math.max(40, Math.abs(end.x - start.x) * 0.5);
  const d = `M ${start.x} ${start.y} C ${start.x + dx} ${start.y}, ${end.x - dx} ${end.y}, ${end.x} ${end.y}`;
  return <path d={d} className={`edge-path ${type} preview`} pointerEvents="none" />;
}
