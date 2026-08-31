import Foundation

enum FirmwareStore {
    static func url(for firmware: Firmware) -> URL {
        PathUtils.getFirmwareDir.appendingPathComponent(firmware.rawValue)
    }

    static func isInstalled(_ firmware: Firmware) -> Bool {
        guard let data = try? Data(contentsOf: url(for: firmware)) else { return false }
        return data.count == firmware.expectedSize
    }

    static func importFile(at sourceURL: URL, as firmware: Firmware) throws {
        let didAccess = sourceURL.startAccessingSecurityScopedResource()
        defer {
            if didAccess {
                sourceURL.stopAccessingSecurityScopedResource()
            }
        }

        let data = try Data(contentsOf: sourceURL)
        guard data.count == firmware.expectedSize else {
            throw CocoaError(.fileReadCorruptFile)
        }

        try data.write(to: url(for: firmware), options: .atomic)
    }
}
