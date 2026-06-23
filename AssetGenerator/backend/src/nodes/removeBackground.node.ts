import type { NodeImpl, PortValue } from './types.js';
import { removeBackgroundTwoPass } from '../utils/images.js';

export const removeBackgroundNode: NodeImpl = {
  schema: {
    type: 'removeBackground',
    label: 'Remove background (two-pass)',
    category: 'transform',
    description: 'Recover alpha from two identical shots on white and black backgrounds.',
    inputs: [
      { id: 'white', name: 'white', type: 'image' },
      { id: 'black', name: 'black', type: 'image' },
    ],
    outputs: [
      { id: 'out', name: 'out', type: 'image' },
    ],
    configSchema: [],
  },
  async run(_ctx, inputs) {
    const white = inputs.get('white');
    const black = inputs.get('black');
    if (!white || !black) throw new Error('removeBackground: both "white" and "black" inputs are required');
    if (white.type !== 'image' || black.type !== 'image') throw new Error('removeBackground: inputs must be images');

    const png = await removeBackgroundTwoPass(white.data, black.data);
    const value: PortValue = { type: 'image', data: png, mimeType: 'image/png' };
    return new Map([['out', value]]);
  },
};
