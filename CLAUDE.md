# o2 engine — Claude project memory

## Documentation first

Full engine documentation lives in `Docs/` in two languages:

- English entry point: `Docs/en/main.md`
- Russian entry point: `Docs/ru/main.md`

Structure (same in both languages): `main.md` — general concept and doc index → `Architecture/architecture.md` — survey of all subsystems with links → detail pages per subsystem in `Architecture/Utils/` (memory, reflection, serialization, properties, delegates, math, containers, strings, file system, logging, debug, tasks, time), `Architecture/LowLevel/` (application, assets, render, animations, input, physics, scripting, sound), `Architecture/HighLevel/` (scene, components, UI) → editor manual in `Editor/editor.md` with per-window pages (scene, tree, properties, assets, animation, game, log).

When asked how the engine works or how to use a subsystem, consult these docs before digging through sources. When engine behavior or API changes, update the affected doc — always in **both** `Docs/en` and `Docs/ru`, keeping them in sync. Doc style: short, dry, reference-like; code examples inside `<details>` blocks; inter-doc links are repo-root-absolute (`/Docs/en/...`, `/Docs/ru/...`).
