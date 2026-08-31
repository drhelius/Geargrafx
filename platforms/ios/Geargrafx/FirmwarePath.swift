import Foundation

extension PathUtils {
    static var getFirmwareDir: URL {
        let dir = getDataDir.appendingPathComponent("BIOS")
        let legacyDir = getDataDir.appendingPathComponent("Firmware")
        let fileManager = FileManager.default

        do {
            if fileManager.fileExists(atPath: legacyDir.path),
               !fileManager.fileExists(atPath: dir.path) {
                try fileManager.moveItem(at: legacyDir, to: dir)
            } else if !fileManager.fileExists(atPath: dir.path) {
                try fileManager.createDirectory(at: dir, withIntermediateDirectories: true)
            }
        } catch {
            debugPrint("ERROR: Cannot prepare /BIOS")
        }

        return dir
    }
}
