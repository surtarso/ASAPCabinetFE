### Instructions:

The flatpak requires rust-vendor.tar.gz (vpin crate dependencies)
 - in flatpak/ folder:  (this folder)

```sh
cp -var ../src/vpin_ffi_wrapper/src/ .
cargo vendor --manifest-path rust-vendor.toml
tar czf rust-vendor.tar.gz vendor/
sha256sum rust-vendor.tar.gz
```

 - The SHA256 must be updated in [`io.asapcabinetfe.ASAPCabinetFE.yml`](./io.asapcabinetfe.ASAPCabinetFE.yml) **sources** section.

e.g.:
```yaml
  sources:
    - type: file  # cargo vendor archive (vpin crate dependencies)
      path: rust-vendor.tar.gz
      sha256: c896ffc30b0a572f62589cd71da50c6f4c1712465c3af8bc2056aaa15befa494 # <- update here!
```

 - In the same file, you may chose current build or tag release

 ```yaml
  sources:
      # local source code
      # - type: dir
      #   path: ../../ASAPCabinetFE

      # git source code (for release builds)
      - type: git
        url: https://github.com/surtarso/ASAPCabinetFE.git
        tag: v1.2.15  # <- 'main' for latest, or use a release tag
 ```
