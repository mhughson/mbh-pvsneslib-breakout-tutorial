# Local PVSnesLib SDK

Download the [PVSnesLib 4.6.0 release](https://github.com/alekmaul/pvsneslib/releases/tag/4.6.0)
for your operating system and extract it here. Use the release archive, not
GitHub's source-code archive.

After extraction, this directory should contain:

```text
devkitsnes/
pvsneslib/
snes-examples/
```

If the archive has an outer `pvsneslib` folder, copy its contents here rather
than creating an extra directory level.

Run `make` from the repository root in MSYS2 UCRT64 on Windows. The Makefile
finds this SDK automatically; no `PVSNESLIB_HOME` environment variable is
required.

Only this README is tracked by Git. The downloaded SDK stays local.