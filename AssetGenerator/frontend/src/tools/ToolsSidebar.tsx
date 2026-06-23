import React from 'react';
import type { ToolDef } from './ToolRegistry.js';

interface Props {
  tools: ToolDef[];
  activeId: string;
  onSelect(id: string): void;
}

export function ToolsSidebar({ tools, activeId, onSelect }: Props): JSX.Element {
  return (
    <div className="tools-sidebar">
      {tools.map((t) => (
        <button
          key={t.id}
          className={`tool-tab ${t.id === activeId ? 'active' : ''}`}
          onClick={() => onSelect(t.id)}
        >
          {t.label}
        </button>
      ))}
    </div>
  );
}
