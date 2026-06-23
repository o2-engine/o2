import type { NodeImpl } from './types.js';
import { finishTextNode } from './finishText.node.js';
import { finishImageNode } from './finishImage.node.js';
import { sourceTextNode } from './sourceText.node.js';
import { sourceTextFileNode } from './sourceTextFile.node.js';
import { sourceImageNode } from './sourceImage.node.js';
import { textComposeNode } from './textCompose.node.js';
import { textConcatNode } from './textConcat.node.js';
import { nanoBananaGenNode } from './nanoBananaGen.node.js';
import { aiTextNode } from './aiText.node.js';
import { removeBackgroundNode } from './removeBackground.node.js';

const all: NodeImpl[] = [
  finishImageNode,
  finishTextNode,
  sourceTextNode,
  sourceTextFileNode,
  sourceImageNode,
  textComposeNode,
  textConcatNode,
  aiTextNode,
  nanoBananaGenNode,
  removeBackgroundNode,
];

const byType = new Map(all.map((n) => [n.schema.type, n]));

export function getNodeImpl(type: string): NodeImpl {
  const impl = byType.get(type);
  if (!impl) throw new Error(`Unknown node type: ${type}`);
  return impl;
}

export function listNodeSchemas() {
  return all.map((n) => n.schema);
}
