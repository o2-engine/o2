import React from 'react';
import type { NodeData, NodeTypeSchema } from '../../../../shared/types.js';
import { NODE_HEADER_HEIGHT, NODE_PAD_BOTTOM, NODE_PAD_TOP, NODE_PORT_ROW, PORT_RADIUS } from './constants.js';
import { bodyAreaHeight, nodeHeight, nodeWidth, portsRowsHeight } from './geometry.js';
import { renderBodyFor } from './nodeRenderers.js';
import { nodeIcon, PlayIcon, AlertIcon, StopIcon } from './icons.js';
import { useExecutionStore } from './executionStore.js';
import type { NodeRunState } from './executionStore.js';

export interface ResizeEdges { n?: boolean; s?: boolean; e?: boolean; w?: boolean }

interface PortMouseInfo {
  nodeId: string;
  portId: string;
  side: 'input' | 'output';
}

interface Props {
  node: NodeData;
  schema?: NodeTypeSchema;
  selected: boolean;
  runState: NodeRunState;
  runError?: string;
  onNodeMouseDown(e: React.MouseEvent, nodeId: string): void;
  onNodeContext(e: React.MouseEvent, nodeId: string): void;
  onPortMouseDown(e: React.MouseEvent, info: PortMouseInfo): void;
  onPortMouseUp(e: React.MouseEvent, info: PortMouseInfo): void;
  onResizeMouseDown(e: React.MouseEvent, nodeId: string, edges: ResizeEdges): void;
  onPlay?(nodeId: string): void;
  onStop?(nodeId: string): void;
  onShowNodeError?(nodeId: string, error: string): void;
}

export function NodeView({
  node, schema, selected, runState, runError,
  onNodeMouseDown, onNodeContext, onPortMouseDown, onPortMouseUp, onResizeMouseDown, onPlay, onStop, onShowNodeError,
}: Props): JSX.Element {
  const isBusy = useExecutionStore((s) => s.runningRequests.has(node.id));
  const w = nodeWidth(node);
  const h = nodeHeight(node);
  const label = schema?.label ?? node.type;
  const stateClass = runState === 'idle' ? '' : `state-${runState}`;
  const portsH = portsRowsHeight(node);
  const bodyH = bodyAreaHeight(node);
  const bodyY = NODE_HEADER_HEIGHT + NODE_PAD_TOP + portsH;
  const icon = nodeIcon(node.type, { size: 14, color: 'white' });

  const startResize = (edges: ResizeEdges) => (e: React.MouseEvent) => onResizeMouseDown(e, node.id, edges);

  return (
    <g className="node" transform={`translate(${node.position.x}, ${node.position.y})`}>
      <rect
        className={`node-rect ${stateClass}`}
        x={0} y={0} width={w} height={h} rx={6}
        onMouseDown={(e) => onNodeMouseDown(e, node.id)}
        onContextMenu={(e) => { e.preventDefault(); onNodeContext(e, node.id); }}
      >
        {runError && <title>{runError}</title>}
      </rect>
      <path
        d={`M 0,6 a 6,6 0 0 1 6,-6 L ${w - 6},0 a 6,6 0 0 1 6,6 L ${w},${NODE_HEADER_HEIGHT} L 0,${NODE_HEADER_HEIGHT} Z`}
        className="node-header"
        onMouseDown={(e) => onNodeMouseDown(e, node.id)}
      />
      {icon && (
        <g transform={`translate(8, 7)`} pointerEvents="none">
          {icon}
        </g>
      )}
      <text className="node-title" x={icon ? 28 : 10} y={NODE_HEADER_HEIGHT - 9} pointerEvents="none">{label}</text>

      {runState === 'error' && runError && onShowNodeError && (
        <g
          transform={`translate(${w - 60}, 4)`}
          className="node-error-btn"
          onClick={(e) => { e.stopPropagation(); onShowNodeError(node.id, runError); }}
          onMouseDown={(e) => e.stopPropagation()}
        >
          <circle cx={10} cy={10} r={10} />
          <g transform="translate(3 3)" pointerEvents="none">
            <AlertIcon size={14} color="white" />
          </g>
        </g>
      )}

      {onPlay && (
        <g
          transform={`translate(${w - 36}, 4)`}
          className="node-play-btn"
          onClick={(e) => { e.stopPropagation(); isBusy ? onStop?.(node.id) : onPlay(node.id); }}
          onMouseDown={(e) => e.stopPropagation()}
        >
          <title>{isBusy ? 'Stop' : 'Run pipeline up to this node'}</title>
          <rect x={0} y={0} width={32} height={20} rx={3} className={isBusy ? 'stop-btn' : 'play-btn'} />
          <g transform="translate(10 4)" pointerEvents="none">
            {isBusy ? <StopIcon size={12} color="white" /> : <PlayIcon size={12} color="white" />}
          </g>
        </g>
      )}

      {bodyH > 0 && (
        <foreignObject x={0} y={bodyY} width={w} height={bodyH + NODE_PAD_BOTTOM}>
          <div className="node-body-host" onMouseDown={(e) => e.stopPropagation()}>
            {renderBodyFor(node)}
          </div>
        </foreignObject>
      )}

      {/* Resize handles — 4 edges + 4 corners, mostly outside the rect */}
      <g pointerEvents="all" className="resize-handles">
        <rect className="resize-edge h" x={6} y={-3} width={w - 12} height={4} style={{ cursor: 'ns-resize' }} onMouseDown={startResize({ n: true })} />
        <rect className="resize-edge h" x={6} y={h - 1} width={w - 12} height={4} style={{ cursor: 'ns-resize' }} onMouseDown={startResize({ s: true })} />
        <rect className="resize-edge v" x={-3} y={6} width={4} height={h - 12} style={{ cursor: 'ew-resize' }} onMouseDown={startResize({ w: true })} />
        <rect className="resize-edge v" x={w - 1} y={6} width={4} height={h - 12} style={{ cursor: 'ew-resize' }} onMouseDown={startResize({ e: true })} />
        <rect className="resize-corner" x={-3} y={-3} width={9} height={9} style={{ cursor: 'nwse-resize' }} onMouseDown={startResize({ n: true, w: true })} />
        <rect className="resize-corner" x={w - 6} y={-3} width={9} height={9} style={{ cursor: 'nesw-resize' }} onMouseDown={startResize({ n: true, e: true })} />
        <rect className="resize-corner" x={-3} y={h - 6} width={9} height={9} style={{ cursor: 'nesw-resize' }} onMouseDown={startResize({ s: true, w: true })} />
        <rect className="resize-corner" x={w - 6} y={h - 6} width={9} height={9} style={{ cursor: 'nwse-resize' }} onMouseDown={startResize({ s: true, e: true })} />
        {/* Visual SE chevron */}
        <path d={`M ${w - 10} ${h - 2} L ${w - 2} ${h - 10} M ${w - 6} ${h - 2} L ${w - 2} ${h - 6}`}
          stroke="var(--fg-dim)" strokeWidth={1.5} pointerEvents="none" />
      </g>

      {/* Ports rendered LAST so their hit zones beat resize handles and edge hit zones */}
      {node.inputs.map((p, i) => {
        const cy = NODE_HEADER_HEIGHT + NODE_PAD_TOP + (i + 0.5) * NODE_PORT_ROW;
        return (
          <g key={p.id} className={`port ${p.type}`}>
            <circle
              cx={0} cy={cy} r={14}
              className="port-hit"
              onMouseDown={(e) => { e.stopPropagation(); onPortMouseDown(e, { nodeId: node.id, portId: p.id, side: 'input' }); }}
              onMouseUp={(e) => { e.stopPropagation(); onPortMouseUp(e, { nodeId: node.id, portId: p.id, side: 'input' }); }}
            />
            <circle
              cx={0} cy={cy} r={PORT_RADIUS}
              className={`port-circle ${p.type}`}
              pointerEvents="none"
            />
            <text className={`port-${p.type}`} x={12} y={cy + 4} pointerEvents="none">{p.name}</text>
          </g>
        );
      })}
      {node.outputs.map((p, i) => {
        const cy = NODE_HEADER_HEIGHT + NODE_PAD_TOP + (i + 0.5) * NODE_PORT_ROW;
        return (
          <g key={p.id} className={`port ${p.type}`}>
            <circle
              cx={w} cy={cy} r={14}
              className="port-hit"
              onMouseDown={(e) => { e.stopPropagation(); onPortMouseDown(e, { nodeId: node.id, portId: p.id, side: 'output' }); }}
              onMouseUp={(e) => { e.stopPropagation(); onPortMouseUp(e, { nodeId: node.id, portId: p.id, side: 'output' }); }}
            />
            <circle
              cx={w} cy={cy} r={PORT_RADIUS}
              className={`port-circle ${p.type}`}
              pointerEvents="none"
            />
            <text className={`port-${p.type}`} x={w - 12} y={cy + 4} textAnchor="end" pointerEvents="none">{p.name}</text>
          </g>
        );
      })}

      {selected && (
        <rect
          className="node-selection-ring"
          x={-4} y={-4} width={w + 8} height={h + 8} rx={9}
        />
      )}
    </g>
  );
}
