# Fork features (copilot/infinipaint)

Upstream: [ErrorAtLine0/infinipaint](https://github.com/ErrorAtLine0/infinipaint)  
Forgejo: https://forgejo.fsociety00.cc/copilot/infinipaint (public)  
GitHub (CI / Windows zip): https://github.com/littlecodedragon/infinipaint

Save format: `INFPNT000007` / version `0.7.0-fork` (not network-compatible with stock 0.6.x peers).

## CI

| Workflow | Trigger | Runner | Output |
|----------|---------|--------|--------|
| `.github/workflows/windows_release.yml` | every push to `main` | GitHub-hosted **`windows-latest`** | prerelease tag `windows-continuous` |
| `.forgejo/workflows/windows_release.yml` | every push to `main` | Forgejo self-hosted `windows-release` (optional) | same tag on Forgejo |

Primary path: GitHub Actions — see [docs/windows-runner.md](docs/windows-runner.md).  
Download: https://github.com/littlecodedragon/infinipaint/releases/tag/windows-continuous

## Bring Everyone Here (host)

In the player list, the lobby host gets **Bring Everyone Here**. Clients receive `CLIENT_FORCE_CAMERA_JUMP` and run the same `smooth_move_to` path as **Jump To**.

## Group / Ungroup (canvas objects)

With a multi-selection (images, brush strokes, text boxes, shapes, …), use **Group** / **Ungroup** in the selection panel. Grouped objects auto-expand into the selection so they transform together. Not related to players.

## Image compression

Settings → **Images**: compress on insert (WebP/PNG/Keep). Selection: **Compress Images**.

## Freistellen

Settings → **Images**: white-key and optional rembg (Tower `silueta` / `bria-rmbg`). Selection: **Freistellen**.
