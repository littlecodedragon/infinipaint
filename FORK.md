# Fork features (copilot/infinipaint)

Upstream: [ErrorAtLine0/infinipaint](https://github.com/ErrorAtLine0/infinipaint)  
Forgejo: https://forgejo.fsociety00.cc/copilot/infinipaint

Save format: `INFPNT000007` / version `0.7.0-fork` (not network-compatible with stock 0.6.x peers).

## Bring Everyone Here (host)

In the player list, the lobby host gets **Bring Everyone Here**. Clients receive `CLIENT_FORCE_CAMERA_JUMP` and run the same `smooth_move_to` path as **Jump To**.

## Group / Ungroup

With a multi-selection (images, text boxes, shapes, …), use **Group** / **Ungroup** in the selection panel. Grouped objects auto-expand into the selection so they transform together. Membership syncs via `SET_COMPONENT_GROUP_IDS`.

## Image compression

Settings → **Images**:

- Compress on insert (default on)
- Format: Keep / WebP / PNG
- WebP quality

Selection toolbar: **Compress Images** re-encodes already placed images.

## Freistellen

Settings → **Images**:

- Freistellen on insert
- Modes: White background (always), or rembg **Schnell** (`silueta`) / **Allgemein** (`bria-rmbg`) when Tower rembg is installed (`~/.local/share/tower-rembg` or `rembg` on `PATH`)

Selection: **Freistellen** applies the configured mode to selected images (then compresses).
