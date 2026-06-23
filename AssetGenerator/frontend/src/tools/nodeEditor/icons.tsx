import React from 'react';

interface IconProps {
  size?: number;
  color?: string;
}

const sw = 1.6;

export function FinishIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M5 4h11l3 3v13a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1V5a1 1 0 0 1 1-1z" />
      <path d="M7 4v6h9V4" />
      <rect x="7" y="14" width="10" height="6" />
    </svg>
  );
}

export function TextIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M5 5h14" />
      <path d="M12 5v14" />
      <path d="M9 19h6" />
    </svg>
  );
}

export function ImageIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <rect x="3" y="4" width="18" height="16" rx="2" />
      <circle cx="9" cy="10" r="1.8" />
      <path d="M21 17l-5-6-4 5-3-3-6 6" />
    </svg>
  );
}

export function PromptIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M9 5c-2 0-3 1-3 3v2c0 1-1 2-2 2 1 0 2 1 2 2v2c0 2 1 3 3 3" />
      <path d="M15 5c2 0 3 1 3 3v2c0 1 1 2 2 2-1 0-2 1-2 2v2c0 2-1 3-3 3" />
    </svg>
  );
}

export function BananaIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M4 5c1 8 7 14 15 15-1-2-1-4-1-6 4 0-2-9-9-9-2 0-4 0-5 0z" />
      <path d="M4 5l-1-2" />
      <path d="M18 14h3" />
    </svg>
  );
}

export function ScissorsIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <circle cx="6" cy="6" r="3" />
      <circle cx="6" cy="18" r="3" />
      <path d="M9 8l11 11" />
      <path d="M9 16l11-11" />
    </svg>
  );
}

export function PlayIcon({ size = 14, color = 'white' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill={color} stroke="none">
      <polygon points="6,4 6,20 20,12" />
    </svg>
  );
}

export function FolderIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M3 6a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" />
    </svg>
  );
}

export function FileIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M6 3h8l4 4v13a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1z" />
      <path d="M14 3v4h4" />
    </svg>
  );
}

export function InfoIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <circle cx="12" cy="12" r="9" />
      <path d="M12 11v6" />
      <circle cx="12" cy="7.8" r="0.8" fill={color} />
    </svg>
  );
}

export function AlertIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M12 3l10 18H2z" />
      <path d="M12 10v5" />
      <circle cx="12" cy="18" r="0.8" fill={color} />
    </svg>
  );
}

export function CloseIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M6 6l12 12" />
      <path d="M18 6L6 18" />
    </svg>
  );
}

export function TrashIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M4 7h16" />
      <path d="M9 7V4h6v3" />
      <path d="M6 7l1 13a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1l1-13" />
      <path d="M10 11v6" />
      <path d="M14 11v6" />
    </svg>
  );
}

export function StopIcon({ size = 12, color = 'white' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill={color} stroke="none">
      <rect x="5" y="5" width="14" height="14" rx="1.5" />
    </svg>
  );
}

export function CheckIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw + 0.4} strokeLinejoin="round" strokeLinecap="round">
      <polyline points="5 12 10 17 19 7" />
    </svg>
  );
}

export function RefreshIcon({ size = 14, color = 'white' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw + 0.2} strokeLinejoin="round" strokeLinecap="round">
      <path d="M20 12a8 8 0 1 1 -2.5 -5.8" />
      <polyline points="20 4 20 9 15 9" />
    </svg>
  );
}

export function DownloadIcon({ size = 14, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M12 3v12" />
      <path d="M7 10l5 5 5-5" />
      <path d="M5 21h14" />
    </svg>
  );
}

export function LinkIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M10 14a4 4 0 0 1 0-6l2-2a4 4 0 0 1 6 6l-1.5 1.5" />
      <path d="M14 10a4 4 0 0 1 0 6l-2 2a4 4 0 0 1-6-6l1.5-1.5" />
    </svg>
  );
}

export function SparkleIcon({ size = 16, color = 'currentColor' }: IconProps): JSX.Element {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth={sw} strokeLinejoin="round" strokeLinecap="round">
      <path d="M12 3l2 5 5 2-5 2-2 5-2-5-5-2 5-2z" />
      <path d="M19 14l1 2 2 1-2 1-1 2-1-2-2-1 2-1z" />
      <path d="M5 14l0.6 1.4L7 16l-1.4 0.6L5 18l-0.6-1.4L3 16l1.4-0.6z" />
    </svg>
  );
}

export const NODE_ICONS: Record<string, React.ComponentType<IconProps>> = {
  finishText: FinishIcon,
  finishImage: FinishIcon,
  sourceText: TextIcon,
  sourceTextFile: TextIcon,
  sourceImage: ImageIcon,
  textCompose: PromptIcon,
  textConcat: LinkIcon,
  nanoBananaGen: BananaIcon,
  aiText: SparkleIcon,
  removeBackground: ScissorsIcon,
};

export function nodeIcon(type: string, props?: IconProps): JSX.Element | null {
  const Comp = NODE_ICONS[type];
  return Comp ? <Comp {...props} /> : null;
}
