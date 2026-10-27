# Upstream

`image-to-ui/` and `LICENSE` here are an unmodified copy of
[XuToWei/Image-To-UI](https://github.com/XuToWei/Image-To-UI) (MIT), commit
`3aa6527dcf09d8c28fe22936dc051d98b331d545` (2026-09-23), made with
`git archive 3aa6527 image-to-ui LICENSE`.

Do not edit these files: the AI image to UI node runs this skill as it is —
its scripts do the preparing, rendering, checking and finalizing, and its
`SKILL.md` + `references/` are the instructions the model follows as the
agent (`../agent/agent.py`). To update, replace the whole folder from a newer
commit, record the commit here, and run both test suites:

```bash
python3 -m unittest discover -s upstream/image-to-ui/tests -t upstream/image-to-ui/tests
python3 -m unittest discover -s agent/tests -t agent/tests
```

The upstream `Unity/` package (the UGUI prefab builder) lives in the Unity
plugin, not here.
