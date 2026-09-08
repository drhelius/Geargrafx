import Foundation

enum Firmware: String, CaseIterable {
    case systemCard3 = "syscard3.pce"
    case systemCard2 = "syscard2.pce"
    case systemCard1 = "syscard1.pce"
    case gameExpress = "gexpress.pce"

    var title: String {
        switch self {
        case .systemCard3: return "System Card 3"
        case .systemCard2: return "System Card 2"
        case .systemCard1: return "System Card 1"
        case .gameExpress: return "Game Express"
        }
    }

    var expectedSize: Int {
        switch self {
        case .systemCard3, .systemCard2, .systemCard1:
            return 0x40000
        case .gameExpress:
            return 0x8000
        }
    }

    var validationErrorMessage: String {
        String(
            format: L10n("Settings::FirmwareSizeFormat"),
            rawValue,
            expectedSize / 1024
        )
    }
}
