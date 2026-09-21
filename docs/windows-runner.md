# Windows Forgejo runner (`windows-release`)

Tower dual-boots Windows. Native InfiniPaint Windows builds need **MSVC 2022**
(see `docs/BUILDING.md` + `windowsinstall/`). There is no Linux cross-build path.

## One-time setup (on Windows)

1. Install Visual Studio 2022 Build Tools (C++), CMake, Git, Python 3.
2. `pip install "conan>=2,<3"`.
3. Download [forgejo-runner](https://code.forgejo.org/forgejo/runner/releases) for Windows.
4. Register against `https://forgejo.fsociety00.cc` with label **`windows-release`**
   (repo or org runner; Actions must be enabled — already on for `copilot/infinipaint`).
5. Run the runner as a service or in a logged-on session when you want CI to drain.

## Behaviour

Workflow: `.forgejo/workflows/windows_release.yml`

- Triggers on every push to `main` (and `workflow_dispatch`)
- Builds x86_64 portable zip via `windowsinstall/*_x86_64.bat`
- Publishes/updates prerelease tag **`windows-continuous`**

Jobs stay queued in Forgejo until a `windows-release` runner is online
(same pattern as `desktop-release` capacity/queue on Tower Linux).
