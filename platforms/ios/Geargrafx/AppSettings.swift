import Foundation

enum GeargrafxConsoleOption: Int, CaseIterable {
    case automatic
    case pcEngine
    case superGrafx
    case turboGrafx16

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .pcEngine: return "PC Engine (Japan)"
        case .superGrafx: return "SuperGrafx (Japan)"
        case .turboGrafx16: return "TurboGrafx-16 (USA)"
        }
    }

    var summaryTitle: String {
        switch self {
        case .automatic: return title
        case .pcEngine: return "PC Engine"
        case .superGrafx: return "SuperGrafx"
        case .turboGrafx16: return "TurboGrafx-16"
        }
    }
}

enum GeargrafxPSGRevisionOption: Int, CaseIterable {
    case automatic
    case huc6280
    case huc6280A

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .huc6280: return "HuC6280"
        case .huc6280A: return "HuC6280A"
        }
    }
}

enum GeargrafxPaletteOption: Int, CaseIterable {
    case standardRGB
    case turboxray
    case kitrinx

    var title: String {
        switch self {
        case .standardRGB: return "Standard RGB"
        case .turboxray: return "Turboxray"
        case .kitrinx: return "Kitrinx"
        }
    }
}

enum GeargrafxLowpassCutoffOption: Int, CaseIterable {
    case mhz30 = 30
    case mhz35 = 35
    case mhz40 = 40
    case mhz45 = 45
    case mhz50 = 50
    case mhz55 = 55
    case mhz60 = 60
    case mhz65 = 65
    case mhz70 = 70

    var title: String {
        String(format: "%.1f MHz", Double(rawValue) / 10.0)
    }
}

enum GeargrafxScanlineOption: Int, CaseIterable {
    case lines224
    case lines240
    case lines242
    case manual

    var title: String {
        switch self {
        case .lines224: return "224p"
        case .lines240: return "240p"
        case .lines242: return "242p"
        case .manual: return L10n("Settings::Manual")
        }
    }
}

enum GeargrafxControllerOption: Int, CaseIterable {
    case standard
    case avenuePad3
    case avenuePad6

    var title: String {
        switch self {
        case .standard: return L10n("Settings::StandardPad")
        case .avenuePad3: return "Avenue Pad 3"
        case .avenuePad6: return "Avenue Pad 6"
        }
    }

    var actionCount: Int {
        switch self {
        case .standard: return 2
        case .avenuePad3: return 3
        case .avenuePad6: return 6
        }
    }
}

enum GeargrafxAvenuePad3ButtonOption: Int, CaseIterable {
    case automatic
    case select
    case run

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .select: return "SELECT"
        case .run: return "RUN"
        }
    }
}

enum GeargrafxCDROMTypeOption: Int, CaseIterable {
    case automatic
    case standard
    case superCDROM
    case arcadeCDROM

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .standard: return L10n("Settings::Standard")
        case .superCDROM: return "Super CD-ROM"
        case .arcadeCDROM: return "Arcade CD-ROM"
        }
    }
}

enum GeargrafxCDROMBIOSOption: Int, CaseIterable {
    case systemCard3
    case systemCard2
    case systemCard1
    case forceGameExpress

    var title: String {
        switch self {
        case .systemCard3: return "System Card 3"
        case .systemCard2: return "System Card 2"
        case .systemCard1: return "System Card 1"
        case .forceGameExpress: return "Force Game Express"
        }
    }
}

enum AppSettings {
    private enum Key {
        static let audioEnabled = "settings.audioEnabled"
        static let hapticsEnabled = "settings.hapticsEnabled"
        static let smoothingEnabled = "settings.smoothingEnabled"
        static let screenSize = "settings.screenSize"
        static let console = "settings.console"
        static let palette = "settings.palette"
        static let overscanEnabled = "settings.overscanEnabled"
        static let scanlineMode = "settings.scanlineMode"
        static let scanlineStart = "settings.scanlineStart"
        static let scanlineEnd = "settings.scanlineEnd"
        static let noSpriteLimitEnabled = "settings.noSpriteLimitEnabled"
        static let lowpassFilterEnabled = "settings.lowpassFilterEnabled"
        static let lowpassIntensity = "settings.lowpassIntensity"
        static let lowpassCutoff = "settings.lowpassCutoff"
        static let safeVDCDefaultsEnabled = "settings.safeVDCDefaultsEnabled"
        static let controller = "settings.controller"
        static let avenuePad3MainButton = "settings.avenuePad3MainButton"
        static let softResetEnabled = "settings.softResetEnabled"
        static let turboIEnabled = "settings.turboIEnabled"
        static let turboISpeed = "settings.turboISpeed"
        static let turboIIEnabled = "settings.turboIIEnabled"
        static let turboIISpeed = "settings.turboIISpeed"
        static let huc6280AEnabled = "settings.huc6280AEnabled"
        static let psgRevision = "settings.psgRevision"
        static let psgVolume = "settings.psgVolume"
        static let cdromVolume = "settings.cdromVolume"
        static let adpcmVolume = "settings.adpcmVolume"
        static let cdromType = "settings.cdromType"
        static let cdromBIOS = "settings.cdromBIOS"
        static let preloadCDROMEnabled = "settings.preloadCDROMEnabled"
        static let saveStateSlot = "settings.saveStateSlot"
    }

    static func registerDefaults() {
        let defaults = UserDefaults.standard

        if defaults.object(forKey: Key.psgRevision) == nil,
           defaults.object(forKey: Key.huc6280AEnabled) != nil {
            let revision: GeargrafxPSGRevisionOption = defaults.bool(forKey: Key.huc6280AEnabled) ?
                .huc6280A : .huc6280
            defaults.set(revision.rawValue, forKey: Key.psgRevision)
        }

        defaults.register(defaults: [
            Key.audioEnabled: true,
            Key.hapticsEnabled: true,
            Key.smoothingEnabled: false,
            Key.screenSize: ScreenSizeOption.fitToWidth.rawValue,
            Key.console: GeargrafxConsoleOption.automatic.rawValue,
            Key.palette: GeargrafxPaletteOption.standardRGB.rawValue,
            Key.overscanEnabled: false,
            Key.scanlineMode: GeargrafxScanlineOption.lines224.rawValue,
            Key.scanlineStart: 11,
            Key.scanlineEnd: 234,
            Key.noSpriteLimitEnabled: false,
            Key.lowpassFilterEnabled: false,
            Key.lowpassIntensity: 100,
            Key.lowpassCutoff: GeargrafxLowpassCutoffOption.mhz50.rawValue,
            Key.safeVDCDefaultsEnabled: false,
            Key.controller: GeargrafxControllerOption.standard.rawValue,
            Key.avenuePad3MainButton: GeargrafxAvenuePad3ButtonOption.automatic.rawValue,
            Key.softResetEnabled: true,
            Key.turboIEnabled: false,
            Key.turboISpeed: 4,
            Key.turboIIEnabled: false,
            Key.turboIISpeed: 4,
            Key.psgRevision: GeargrafxPSGRevisionOption.automatic.rawValue,
            Key.psgVolume: 100,
            Key.cdromVolume: 100,
            Key.adpcmVolume: 100,
            Key.cdromType: GeargrafxCDROMTypeOption.automatic.rawValue,
            Key.cdromBIOS: GeargrafxCDROMBIOSOption.systemCard3.rawValue,
            Key.preloadCDROMEnabled: false,
            Key.saveStateSlot: 1
        ])
    }

    static var audioEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.audioEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.audioEnabled) }
    }

    static var hapticsEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.hapticsEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.hapticsEnabled) }
    }

    static var smoothingEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.smoothingEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.smoothingEnabled) }
    }

    static var screenSize: ScreenSizeOption {
        get { ScreenSizeOption(rawValue: UserDefaults.standard.integer(forKey: Key.screenSize)) ?? .fitToWidth }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.screenSize) }
    }

    static var console: GeargrafxConsoleOption {
        get { GeargrafxConsoleOption(rawValue: UserDefaults.standard.integer(forKey: Key.console)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.console) }
    }

    static var palette: GeargrafxPaletteOption {
        get { GeargrafxPaletteOption(rawValue: UserDefaults.standard.integer(forKey: Key.palette)) ?? .standardRGB }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.palette) }
    }

    static var overscanEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.overscanEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.overscanEnabled) }
    }

    static var scanlineMode: GeargrafxScanlineOption {
        get { GeargrafxScanlineOption(rawValue: UserDefaults.standard.integer(forKey: Key.scanlineMode)) ?? .lines224 }
        set {
            UserDefaults.standard.set(newValue.rawValue, forKey: Key.scanlineMode)
            switch newValue {
            case .lines224:
                scanlineStart = 11
                scanlineEnd = 234
            case .lines240:
                scanlineStart = 2
                scanlineEnd = 241
            case .lines242:
                scanlineStart = 0
                scanlineEnd = 241
            case .manual:
                break
            }
        }
    }

    static var scanlineStart: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.scanlineStart), 0), 30) }
        set { UserDefaults.standard.set(min(max(newValue, 0), 30), forKey: Key.scanlineStart) }
    }

    static var scanlineEnd: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.scanlineEnd), 220), 241) }
        set { UserDefaults.standard.set(min(max(newValue, 220), 241), forKey: Key.scanlineEnd) }
    }

    static var noSpriteLimitEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.noSpriteLimitEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.noSpriteLimitEnabled) }
    }

    static var lowpassFilterEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.lowpassFilterEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.lowpassFilterEnabled) }
    }

    static var lowpassIntensity: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.lowpassIntensity), 0), 100) }
        set { UserDefaults.standard.set(min(max(newValue, 0), 100), forKey: Key.lowpassIntensity) }
    }

    static var lowpassCutoff: GeargrafxLowpassCutoffOption {
        get {
            GeargrafxLowpassCutoffOption(rawValue: UserDefaults.standard.integer(forKey: Key.lowpassCutoff)) ?? .mhz50
        }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.lowpassCutoff) }
    }

    static var safeVDCDefaultsEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.safeVDCDefaultsEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.safeVDCDefaultsEnabled) }
    }

    static var controller: GeargrafxControllerOption {
        get { GeargrafxControllerOption(rawValue: UserDefaults.standard.integer(forKey: Key.controller)) ?? .standard }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.controller) }
    }

    static var avenuePad3MainButton: GeargrafxAvenuePad3ButtonOption {
        get {
            GeargrafxAvenuePad3ButtonOption(
                rawValue: UserDefaults.standard.integer(forKey: Key.avenuePad3MainButton)
            ) ?? .automatic
        }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.avenuePad3MainButton) }
    }

    static var softResetEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.softResetEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.softResetEnabled) }
    }

    static var turboIEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.turboIEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.turboIEnabled) }
    }

    static var turboISpeed: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.turboISpeed), 1), 20) }
        set { UserDefaults.standard.set(min(max(newValue, 1), 20), forKey: Key.turboISpeed) }
    }

    static var turboIIEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.turboIIEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.turboIIEnabled) }
    }

    static var turboIISpeed: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.turboIISpeed), 1), 20) }
        set { UserDefaults.standard.set(min(max(newValue, 1), 20), forKey: Key.turboIISpeed) }
    }

    static var psgRevision: GeargrafxPSGRevisionOption {
        get {
            GeargrafxPSGRevisionOption(rawValue: UserDefaults.standard.integer(forKey: Key.psgRevision)) ?? .automatic
        }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.psgRevision) }
    }

    static var psgVolume: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.psgVolume), 0), 200) }
        set { UserDefaults.standard.set(min(max(newValue, 0), 200), forKey: Key.psgVolume) }
    }

    static var cdromVolume: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.cdromVolume), 0), 200) }
        set { UserDefaults.standard.set(min(max(newValue, 0), 200), forKey: Key.cdromVolume) }
    }

    static var adpcmVolume: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.adpcmVolume), 0), 200) }
        set { UserDefaults.standard.set(min(max(newValue, 0), 200), forKey: Key.adpcmVolume) }
    }

    static var cdromType: GeargrafxCDROMTypeOption {
        get { GeargrafxCDROMTypeOption(rawValue: UserDefaults.standard.integer(forKey: Key.cdromType)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.cdromType) }
    }

    static var cdromBIOS: GeargrafxCDROMBIOSOption {
        get {
            GeargrafxCDROMBIOSOption(rawValue: UserDefaults.standard.integer(forKey: Key.cdromBIOS)) ?? .systemCard3
        }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.cdromBIOS) }
    }

    static var preloadCDROMEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.preloadCDROMEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.preloadCDROMEnabled) }
    }

    static var saveStateSlot: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.saveStateSlot), 1), 5) }
        set { UserDefaults.standard.set(min(max(newValue, 1), 5), forKey: Key.saveStateSlot) }
    }
}
