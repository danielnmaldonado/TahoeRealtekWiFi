// SPDX-License-Identifier: GPL-2.0-only
import AppKit
let destination = CommandLine.arguments[1]
try FileManager.default.createDirectory(atPath: destination, withIntermediateDirectories: true)
for size in [16, 32, 128, 256, 512] {
    for scale in [1, 2] {
        let pixels = size * scale
        let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: pixels, pixelsHigh: pixels,
            bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
            colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
        rep.size = NSSize(width: 512, height: 512)
        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
        let shape = NSBezierPath(roundedRect: NSRect(x: 38, y: 38, width: 436, height: 436), xRadius: 98, yRadius: 98)
        NSGradient(starting: NSColor(srgbRed: 0.11, green: 0.45, blue: 0.95, alpha: 1),
            ending: NSColor(srgbRed: 0.06, green: 0.20, blue: 0.52, alpha: 1))!.draw(in: shape, angle: -70)
        let symbol = NSImage(systemSymbolName: "wifi", accessibilityDescription: nil)!
            .withSymbolConfiguration(NSImage.SymbolConfiguration(paletteColors: [.white]))!
        symbol.draw(in: NSRect(x: 115, y: 143, width: 282, height: 228))
        NSGraphicsContext.restoreGraphicsState()
        let suffix = scale == 1 ? "" : "@2x"
        try rep.representation(using: .png, properties: [:])!.write(to:
            URL(fileURLWithPath: "\(destination)/icon_\(size)x\(size)\(suffix).png"))
    }
}
