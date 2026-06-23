import React from 'react';

export function SettingsButton({ onClick }: { onClick: () => void }): JSX.Element {
  return (
    <button onClick={onClick} title="Settings">
      ⚙ Settings
    </button>
  );
}
