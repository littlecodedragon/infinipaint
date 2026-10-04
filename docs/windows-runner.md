# Windows continuous release (GitHub Actions)

Native InfiniPaint Windows builds need **MSVC** (see `docs/BUILDING.md` +
`windowsinstall/`). There is no Linux cross-build path.

The fork publishes a portable zip from a **GitHub** repository using the
GitHub-hosted runner `windows-latest` (Visual Studio / MSVC preinstalled).

## Workflow

File: `.github/workflows/windows_release.yml`

| | |
|---|---|
| Trigger | every push to `main`, plus `workflow_dispatch` |
| Runner | `windows-latest` (GitHub-hosted, MSVC ~195) |
| Build | Conan profile `win-ci` (detected) + `windowsinstall/build_x86_64.bat` |
| Output | prerelease tag **`windows-continuous`** with a portable zip |

Local Tower Windows can keep using `windowsinstall/conan_init_x86_64.bat`
(`conan/profiles/win-x86_64`, MSVC 194). CI generates a `win-ci` profile for
whatever MSVC `windows-latest` ships.

## GitHub remote

Recommended remote name: `github`

```bash
git remote add github https://github.com/littlecodedragon/infinipaint.git
git push -u github main
```

Releases appear under:
https://github.com/littlecodedragon/infinipaint/releases/tag/windows-continuous

## Forgejo note

`.forgejo/workflows/windows_release.yml` still targets a Forgejo self-hosted
label `windows-release` (Tower dual-boot). Prefer the GitHub workflow above for
unattended continuous Windows zips; keep Forgejo for the public forge mirror if
desired.
