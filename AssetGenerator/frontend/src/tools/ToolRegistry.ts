import type { ComponentType } from 'react';
import { NodeEditorTool } from './nodeEditor/NodeEditorTool.js';

export interface ToolDef {
  id: string;
  label: string;
  component: ComponentType;
}

export const TOOLS: ToolDef[] = [
  { id: 'nodeEditor', label: 'Node Image Editor', component: NodeEditorTool },
];
