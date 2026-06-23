import React, { useState } from 'react';
import { SettingsButton } from './settings/SettingsButton.js';
import { SettingsDialog } from './settings/SettingsDialog.js';
import { ToolsSidebar } from './tools/ToolsSidebar.js';
import { TOOLS } from './tools/ToolRegistry.js';
import { ErrorDialog } from './common/ErrorDialog.js';
import { ToastContainer } from './common/ToastContainer.js';

export function App(): JSX.Element {
  const [activeToolId, setActiveToolId] = useState(TOOLS[0]?.id ?? '');
  const [settingsOpen, setSettingsOpen] = useState(false);

  const ActiveTool = TOOLS.find((t) => t.id === activeToolId)?.component;

  return (
    <div className="app-root">
      <div className="topbar">
        <div className="topbar-title">AssetGenerator</div>
        <SettingsButton onClick={() => setSettingsOpen(true)} />
      </div>
      <ToolsSidebar
        tools={TOOLS}
        activeId={activeToolId}
        onSelect={setActiveToolId}
      />
      <div className="main-area">
        {ActiveTool ? <ActiveTool /> : <div style={{ padding: 20 }}>No tool selected.</div>}
      </div>
      {settingsOpen && <SettingsDialog onClose={() => setSettingsOpen(false)} />}
      <ToastContainer />
      <ErrorDialog />
    </div>
  );
}
