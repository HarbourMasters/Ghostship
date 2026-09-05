# macOS app icon

`GhostshipIcon.icon` is the icon source, an Apple Icon Composer package (a JSON manifest plus the layer art). On macOS 26 and later the system renders it with the Liquid Glass material.

`Assets.car` is the compiled catalog, checked in prebuilt so contributors and CI do not need a new Xcode (older `actool` cannot compile `.icon` packages). Info.plist points at it through `CFBundleIconName`. Older macOS versions keep using the `Ghostship.icns` generated at build time from `logo.png`.

## Regenerating Assets.car after changing the artwork

Requires Xcode 26 or later.

```
mkdir -p out
xcrun actool GhostshipIcon.icon --compile out --app-icon GhostshipIcon \
  --output-partial-info-plist out/partial.plist \
  --platform macosx --target-device mac --minimum-deployment-target 10.15 \
  --errors --warnings
cp out/Assets.car Assets.car
```

`--app-icon` must match the `.icon` basename, and that name must match `CFBundleIconName`. Keep the layers as PNG: SVG layers compile without the glass material.
