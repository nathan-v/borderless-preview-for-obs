# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| latest  | Yes       |

## Reporting a Vulnerability

This plugin is a native library loaded into the OBS Studio process, so a memory safety bug here is a bug inside OBS.

For non-sensitive issues, open a regular GitHub issue. If you believe you have found a security vulnerability that should be disclosed privately, please [contact](https://www.nathanv.com/contact) the maintainer directly.

Please include:

- A description of the issue
- Steps to reproduce
- Potential impact
- A suggested fix (if you have one)

You should receive a response within 72 hours.

## Known Limitations

- **In-process, full trust.** OBS plugins run inside the OBS process with OBS's own privileges; there is no sandbox. Install builds only from the GitHub releases page, verified against the SHA-256 checksums listed on each release, or from source you have read.
- **macOS builds are ad-hoc signed and not notarized.** Release builds come from GitHub Actions without a Developer ID, so Gatekeeper warns on the `.pkg` and you have to open it deliberately. The signature says nothing about who built it; the SHA-256 checksums on the release do. Windows and Linux builds carry no signature at all.
- **One config file.** The plugin reads and writes `config.json` under OBS's `plugin_config` directory and touches no other files.
- **No network access.** The plugin opens no sockets and makes no HTTP requests.
- **No phoning home.** This project does not collect analytics or metrics and does not call home in any way.
