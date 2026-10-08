# Source releases

A consumer release is the pair of uploaded lvglpp and LVGL source archives,
plus `manifest.json`, `SHA256SUMS`, and `lvglpp.hash`. GitHub's automatic
"Source code" downloads omit submodule contents and are not the consumer
package. Keep the uploaded bytes unchanged when caching or mirroring them.

The root [MIT license](LICENSE) covers lvglpp. LVGL's `LICENCE.txt`,
`COPYRIGHTS.md`, and bundled third-party notices remain in the LVGL archive;
the manifest and Buildroot hash file record the principal license hashes.
Consumers must retain applicable third-party notices too.

## Source policy

[`release/source.json`](release/source.json) records the approved LVGL
commit, version, and optional upstream release tag. Packaging requires the
committed gitlink and version macros to match this policy. If a release tag
is declared, it must resolve to that same commit and describe stable version
macros. No moving branch is accepted as a source identity.

The consumer package uses upstream **LVGL v9.6.0**, pinned to
`80ca777e37a2b176770726a02e07a6fb79ef0b39`. Its public headers use the
canonical `lvgl/include/lvgl/` paths. The host fallback selects
`LV_COLOR_FORMAT_DEFAULT=LV_COLOR_FORMAT_ARGB8888`; consumer RGB565 is tested
independently. Downstream headers should use `LV_COLOR_FORMAT_DEFAULT`:
`LV_COLOR_DEPTH` is deprecated upstream and triggers strict-build warnings.

The source-baseline amendment in
[LPAR-CPP-01](docs/lvgl-parity/01-baseline.md#0-authority-policy) must land
before this dependent gitlink/configuration migration. Future LVGL upgrades
follow the same baseline amendment and qualification order in
[AGENTS.md](AGENTS.md#execution-discipline).

## Workflow

[Source release](.github/workflows/source-release.yml) runs on PRs and main,
and for pushed version tags or manual dispatch. It packages committed Git
objects only; working-tree edits and the Rust reference are excluded. Archive
members are sorted and their modes, owners, timestamps, and gzip timestamp
are normalized. Checksums describe the actual uploaded compressed bytes.

CI checks the extracted source pair with no Git metadata or Rust checkout:

- Host tests and C++ warnings as errors.
- CMake 3.20.5 and the Ubuntu runner's CMake.
- A library-only downstream CMake consumer using its own configuration,
  with Wayland SHM, FBDEV, and EVDEV enabled, including a driver link probe.
- Cortex-M7 cross-built library archives with embedded posture.

These checks establish source/build compatibility. They do not establish
DisplayManager behavior on its Buildroot sysroot, a live compositor, or board
hardware. Those remain downstream acceptance checks.

Only the final job has `contents: write`. It creates a **draft** release after
all checks pass. The version tag must already exist, match `project(VERSION)`
and point to a commit on `main`. It checks the remote tag and main again
before creating the draft. It never creates tags, publishes a release, moves
a tag, or replaces an existing asset. An exact existing draft can be accepted
on retry only when GitHub reports matching asset digests. Partial or different
releases require maintainer review; automatic repair is refused.

## First release procedure

1. Merge the consumer CMake changes and this workflow. Run the Source release
   checks on the resulting main commit. Set the next project version in
   `CMakeLists.txt` before tagging; the initial version is `0.1.0`.
2. In GitHub **Settings → Releases**, enable **release immutability** before
   publishing the first release. A draft remains editable while all assets
   are assembled. Protect version tags against update/deletion through the
   repository's tag ruleset policy.
3. Create an annotated tag on the validated main commit and push it through
   the writable remote. For the initial version:

   ```bash
   git tag -a v0.1.0 <validated-main-commit> -m 'lvglpp v0.1.0'
   git push writable refs/tags/v0.1.0
   ```

4. Review the draft's manifest, checksums, licenses, and successful CI runs.
   Check the downstream consumer against the exact uploaded files. Publish
   the draft after that review. Immutability then locks the release assets
   and tag. Do not delete and recreate an existing version to replace bytes.

For an existing tag, manually run Source release **from main**, supplying the
same version tag. This supports a failed run retry without creating a new tag.
The workflow must have been merged before the tag is created.

GitHub settings are administrative steps, not files in this PR. Require the
Source release checks in the branch ruleset if release validation is to be a
merge gate; the workflow itself cannot configure repository protections.

## Local candidate check

Requires Git and Python 3.9+ to package; consumers need only the extracted
sources, CMake >= 3.20, and a C++20 toolchain. Use an empty output directory:

```bash
git submodule update --init lvgl  # no recursion, no rlvgl checkout needed
python3 -m unittest discover -s scripts/release -p 'test_*.py' -v
python3 scripts/release/source_release.py package --output /tmp/lvglpp-assets
python3 scripts/release/source_release.py verify /tmp/lvglpp-assets
python3 scripts/release/source_release.py extract /tmp/lvglpp-assets /tmp/lvglpp-source
```

A local candidate has no release tag in its manifest. For release-mode checks,
add `--release-tag v0.1.0` from that tag's committed checkout with a current
`origin/main` tracking ref. Packaging reads `HEAD`, including the committed
release helpers/license/policy; commit changes before checking a candidate.

## Buildroot consumption

Use the uploaded asset URLs under
`https://github.com/SoftOboros/lvglpp/releases/download/<tag>/<asset>`.
`manifest.json` supplies the exact filenames and revisions. The released
`lvglpp.hash` contains SHA-256 entries for both archives, the manifest, and the
extracted principal licenses. Copy its relevant entries into the consuming
package's `.hash` file and retain MIT/third-party notices. `SHA256SUMS` also
covers `lvglpp.hash`.

When adapting an existing DisplayManager package, add the LVGL archive to
`<PKG>_EXTRA_DOWNLOADS` and explicitly extract it in the post-extract hook to
`$(@D)/lvgl`, stripping its single archive root. Extra downloads are not
extracted automatically. Verify both source hashes before extraction; do not
try to initialize submodules from a GitHub source archive. A separate
Buildroot package is unnecessary for the initial source-inclusion layer.

Pass the target sysroot/toolchain, downstream `LV_BUILD_CONF_PATH` or
`LV_BUILD_CONF_DIR`, matching `CONFIG_LV_*` backend selections, and library-only
options described in [README.md](README.md#consumer-and-buildroot-builds).
Disable dependency fetching and supply target Wayland/xkbcommon/libevdev
libraries through the consumer toolchain/pkg-config setup. Source packaging
does not supply those system libraries or a CMake installed-package export.

References: [GitHub source archive guarantees](https://docs.github.com/en/repositories/working-with-files/using-files/downloading-source-code-archives),
[immutable releases](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases),
[release immutability settings](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/establish-provenance-and-integrity/prevent-release-changes),
and the [Buildroot manual](https://buildroot.org/downloads/manual/manual.html).
