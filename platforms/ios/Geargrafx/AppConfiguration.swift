import Foundation

enum AppConfiguration {
    static let libraryTitleLocalizationKey = "Common::Geargrafx"
    static let thumbnailBaseURL = URL(string: "https://www.drhelius.com/thumbnails/geargrafx/")!

    static func romCRC(inArchiveAt url: URL) -> String? {
        GeargrafxEmulator.romCRC(inArchiveAt: url)
    }
}
