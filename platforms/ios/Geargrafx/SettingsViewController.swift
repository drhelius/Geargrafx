import UIKit
import Combine
import UniformTypeIdentifiers

final class SettingsViewController: UITableViewController {
    private enum Section: Int, CaseIterable {
        case gameplay
        case system
        case video
        case audio
        case input
        case cdrom
        case firmware
        case library
        case about
    }

    private enum GameplayRow: Int, CaseIterable {
        case audio
        case saveStateSlot
    }

    private enum SystemRow: Int, CaseIterable {
        case console
        case safeVDCDefaults
    }

    private enum VideoRow: Int, CaseIterable {
        case palette
        case overscan
        case scanlineMode
        case scanlineStart
        case scanlineEnd
        case noSpriteLimit
        case lowpassFilter
        case lowpassIntensity
        case lowpassCutoff
        case screenSize
        case smoothing
    }

    private enum InputRow: Int, CaseIterable {
        case controller
        case avenuePad3MainButton
        case softReset
        case turboI
        case turboISpeed
        case turboII
        case turboIISpeed
        case haptics
    }

    private enum AudioRow: Int, CaseIterable {
        case psgRevision
        case psgVolume
        case cdromVolume
        case adpcmVolume
    }

    private enum CDROMRow: Int, CaseIterable {
        case type
        case bios
        case preload
    }

    private enum LibraryRow: Int, CaseIterable {
        case refresh
        case importedRoms
    }

    private var dataStoreSubscriber: AnyCancellable?
    private var pendingFirmware: Firmware?

    init() {
        super.init(style: .insetGrouped)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        title = L10n("Common::Settings")
        navigationItem.largeTitleDisplayMode = .always
        navigationController?.navigationBar.prefersLargeTitles = true

        dataStoreSubscriber = dataStore.$allRoms
            .removeDuplicates { $0.count == $1.count }
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.tableView.reloadData()
            }
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        tableView.reloadData()
    }

    override func numberOfSections(in tableView: UITableView) -> Int {
        Section.allCases.count
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch Section(rawValue: section) {
        case .gameplay: return GameplayRow.allCases.count
        case .system: return SystemRow.allCases.count
        case .video: return VideoRow.allCases.count
        case .audio: return AudioRow.allCases.count
        case .input: return InputRow.allCases.count
        case .cdrom: return CDROMRow.allCases.count
        case .firmware: return Firmware.allCases.count
        case .library: return LibraryRow.allCases.count
        case .about: return 1
        case nil: return 0
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch Section(rawValue: section) {
        case .gameplay: return L10n("Settings::Gameplay")
        case .system: return L10n("Settings::System")
        case .video: return L10n("Settings::Video")
        case .audio: return L10n("Settings::AudioSection")
        case .input: return L10n("Settings::Input")
        case .cdrom: return L10n("Settings::CDROM")
        case .firmware: return L10n("Settings::Firmware")
        case .library: return L10n("Settings::Library")
        case .about: return L10n("Settings::About")
        case nil: return nil
        }
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        switch Section(rawValue: section) {
        case .gameplay: return L10n("Settings::GameplayFooter")
        case .system: return L10n("Settings::SystemFooter")
        case .video: return L10n("Settings::VideoFooter")
        case .audio: return L10n("Settings::AudioMixingFooter")
        case .input: return L10n("Settings::InputFooter")
        case .cdrom: return L10n("Settings::CDROMFooter")
        case .firmware: return L10n("Settings::FirmwareFooter")
        default: return nil
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let section = Section(rawValue: indexPath.section) else {
            return UITableViewCell()
        }

        switch section {
        case .gameplay: return gameplayCell(row: indexPath.row)
        case .system: return systemCell(row: indexPath.row)
        case .video: return videoCell(row: indexPath.row)
        case .audio: return audioCell(row: indexPath.row)
        case .input: return inputCell(row: indexPath.row)
        case .cdrom: return cdromCell(row: indexPath.row)
        case .firmware: return firmwareCell(row: indexPath.row)
        case .library: return libraryCell(row: indexPath.row)
        case .about: return aboutCell()
        }
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        guard let section = Section(rawValue: indexPath.section) else { return }

        switch section {
        case .gameplay where GameplayRow(rawValue: indexPath.row) == .saveStateSlot:
            showOptions(
                title: L10n("Settings::SaveStateSlot"),
                options: (1...5).map { String(format: L10n("Settings::SlotFormat"), $0) },
                selectedIndex: AppSettings.saveStateSlot - 1
            ) { AppSettings.saveStateSlot = $0 + 1 }
        case .system where SystemRow(rawValue: indexPath.row) == .console:
            showOptions(
                title: L10n("Settings::Console"),
                options: GeargrafxConsoleOption.allCases.map(\.title),
                selectedIndex: AppSettings.console.rawValue
            ) { AppSettings.console = GeargrafxConsoleOption(rawValue: $0) ?? .automatic }
        case .video:
            showVideoOptions(row: indexPath.row)
        case .audio:
            showAudioOptions(row: indexPath.row)
        case .input:
            showInputOptions(row: indexPath.row)
        case .cdrom:
            showCDROMOptions(row: indexPath.row)
        case .firmware:
            guard Firmware.allCases.indices.contains(indexPath.row) else { return }
            importFirmware(Firmware.allCases[indexPath.row])
        case .library where LibraryRow(rawValue: indexPath.row) == .refresh:
            dataStore.updateAll()
        default:
            break
        }
    }

    private func showVideoOptions(row: Int) {
        switch VideoRow(rawValue: row) {
        case .palette:
            showOptions(
                title: L10n("Settings::Palette"),
                options: GeargrafxPaletteOption.allCases.map(\.title),
                selectedIndex: AppSettings.palette.rawValue
            ) { AppSettings.palette = GeargrafxPaletteOption(rawValue: $0) ?? .standardRGB }
        case .scanlineMode:
            showOptions(
                title: L10n("Settings::ScanlineMode"),
                options: GeargrafxScanlineOption.allCases.map(\.title),
                selectedIndex: AppSettings.scanlineMode.rawValue
            ) { [weak self] index in
                AppSettings.scanlineMode = GeargrafxScanlineOption(rawValue: index) ?? .lines224
                self?.tableView.reloadSections(IndexSet(integer: Section.video.rawValue), with: .automatic)
            }
        case .scanlineStart where AppSettings.scanlineMode == .manual:
            let lines = Array(0...30)
            showOptions(
                title: L10n("Settings::ScanlineStart"),
                options: lines.map { String($0) },
                selectedIndex: AppSettings.scanlineStart
            ) { AppSettings.scanlineStart = lines[$0] }
        case .scanlineEnd where AppSettings.scanlineMode == .manual:
            let lines = Array(220...241)
            showOptions(
                title: L10n("Settings::ScanlineEnd"),
                options: lines.map { String($0) },
                selectedIndex: AppSettings.scanlineEnd - lines[0]
            ) { AppSettings.scanlineEnd = lines[$0] }
        case .lowpassIntensity where AppSettings.lowpassFilterEnabled:
            let values = Array(stride(from: 0, through: 100, by: 10))
            showOptions(
                title: L10n("Settings::VideoFilterIntensity"),
                options: values.map { "\($0)%" },
                selectedIndex: AppSettings.lowpassIntensity / 10
            ) { AppSettings.lowpassIntensity = values[$0] }
        case .lowpassCutoff where AppSettings.lowpassFilterEnabled:
            let options = GeargrafxLowpassCutoffOption.allCases
            showOptions(
                title: L10n("Settings::VideoFilterCutoff"),
                options: options.map(\.title),
                selectedIndex: options.firstIndex(of: AppSettings.lowpassCutoff) ?? 4
            ) { AppSettings.lowpassCutoff = options[$0] }
        case .screenSize:
            showOptions(
                title: L10n("Settings::ScreenSize"),
                options: ScreenSizeOption.allCases.map(\.title),
                selectedIndex: AppSettings.screenSize.rawValue
            ) { AppSettings.screenSize = ScreenSizeOption(rawValue: $0) ?? .fitToWidth }
        default:
            break
        }
    }

    private func showInputOptions(row: Int) {
        switch InputRow(rawValue: row) {
        case .controller:
            showOptions(
                title: L10n("Settings::Controller"),
                options: GeargrafxControllerOption.allCases.map(\.title),
                selectedIndex: AppSettings.controller.rawValue
            ) { [weak self] index in
                AppSettings.controller = GeargrafxControllerOption(rawValue: index) ?? .standard
                self?.tableView.reloadSections(IndexSet(integer: Section.input.rawValue), with: .automatic)
            }
        case .avenuePad3MainButton where AppSettings.controller == .avenuePad3:
            showOptions(
                title: L10n("Settings::AvenuePad3Button"),
                options: GeargrafxAvenuePad3ButtonOption.allCases.map(\.title),
                selectedIndex: AppSettings.avenuePad3MainButton.rawValue
            ) { AppSettings.avenuePad3MainButton = GeargrafxAvenuePad3ButtonOption(rawValue: $0) ?? .automatic }
        case .turboISpeed where AppSettings.turboIEnabled:
            showTurboSpeedOptions(title: L10n("Settings::TurboISpeed"), currentValue: AppSettings.turboISpeed) {
                AppSettings.turboISpeed = $0
            }
        case .turboIISpeed where AppSettings.turboIIEnabled:
            showTurboSpeedOptions(title: L10n("Settings::TurboIISpeed"), currentValue: AppSettings.turboIISpeed) {
                AppSettings.turboIISpeed = $0
            }
        default:
            break
        }
    }

    private func showAudioOptions(row: Int) {
        switch AudioRow(rawValue: row) {
        case .psgRevision:
            showOptions(
                title: L10n("Settings::PSGRevision"),
                options: GeargrafxPSGRevisionOption.allCases.map(\.title),
                selectedIndex: AppSettings.psgRevision.rawValue
            ) { AppSettings.psgRevision = GeargrafxPSGRevisionOption(rawValue: $0) ?? .automatic }
        case .psgVolume:
            showVolumeOptions(title: L10n("Settings::PSGVolume"), currentValue: AppSettings.psgVolume) {
                AppSettings.psgVolume = $0
            }
        case .cdromVolume:
            showVolumeOptions(title: L10n("Settings::CDROMVolume"), currentValue: AppSettings.cdromVolume) {
                AppSettings.cdromVolume = $0
            }
        case .adpcmVolume:
            showVolumeOptions(title: L10n("Settings::ADPCMVolume"), currentValue: AppSettings.adpcmVolume) {
                AppSettings.adpcmVolume = $0
            }
        default:
            break
        }
    }

    private func showCDROMOptions(row: Int) {
        switch CDROMRow(rawValue: row) {
        case .type:
            showOptions(
                title: L10n("Settings::CDROMType"),
                options: GeargrafxCDROMTypeOption.allCases.map(\.title),
                selectedIndex: AppSettings.cdromType.rawValue
            ) { AppSettings.cdromType = GeargrafxCDROMTypeOption(rawValue: $0) ?? .automatic }
        case .bios:
            showOptions(
                title: L10n("Settings::CDROMBIOS"),
                options: GeargrafxCDROMBIOSOption.allCases.map(\.title),
                selectedIndex: AppSettings.cdromBIOS.rawValue
            ) { AppSettings.cdromBIOS = GeargrafxCDROMBIOSOption(rawValue: $0) ?? .systemCard3 }
        default:
            break
        }
    }

    private func gameplayCell(row: Int) -> UITableViewCell {
        switch GameplayRow(rawValue: row) {
        case .audio:
            return toggleCell(
                title: L10n("Settings::Audio"),
                detail: L10n("Settings::AudioDetail"),
                image: "speaker.wave.2",
                isOn: AppSettings.audioEnabled,
                action: #selector(audioChanged(_:))
            )
        case .saveStateSlot:
            return optionCell(
                title: L10n("Settings::SaveStateSlot"),
                value: String(format: L10n("Settings::SlotFormat"), AppSettings.saveStateSlot),
                image: "square.stack.3d.up"
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func systemCell(row: Int) -> UITableViewCell {
        switch SystemRow(rawValue: row) {
        case .console:
            return optionCell(title: L10n("Settings::Console"), value: AppSettings.console.summaryTitle, image: "gamecontroller")
        case .safeVDCDefaults:
            return toggleCell(
                title: L10n("Settings::SafeVDCDefaults"),
                detail: L10n("Settings::SafeVDCDefaultsDetail"),
                image: "shield",
                isOn: AppSettings.safeVDCDefaultsEnabled,
                action: #selector(safeVDCDefaultsChanged(_:))
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func videoCell(row: Int) -> UITableViewCell {
        switch VideoRow(rawValue: row) {
        case .palette:
            return optionCell(title: L10n("Settings::Palette"), value: AppSettings.palette.title, image: "paintpalette")
        case .overscan:
            return toggleCell(
                title: L10n("Settings::Overscan"),
                detail: L10n("Settings::OverscanDetail"),
                image: "rectangle.inset.filled",
                isOn: AppSettings.overscanEnabled,
                action: #selector(overscanChanged(_:))
            )
        case .scanlineMode:
            return optionCell(
                title: L10n("Settings::ScanlineMode"),
                value: AppSettings.scanlineMode.title,
                image: "rectangle.split.3x1"
            )
        case .scanlineStart:
            return optionCell(
                title: L10n("Settings::ScanlineStart"),
                value: String(AppSettings.scanlineStart),
                image: "arrow.down.to.line",
                isEnabled: AppSettings.scanlineMode == .manual
            )
        case .scanlineEnd:
            return optionCell(
                title: L10n("Settings::ScanlineEnd"),
                value: String(AppSettings.scanlineEnd),
                image: "arrow.up.to.line",
                isEnabled: AppSettings.scanlineMode == .manual
            )
        case .noSpriteLimit:
            return toggleCell(
                title: L10n("Settings::NoSpriteLimit"),
                detail: L10n("Settings::NoSpriteLimitDetail"),
                image: "sparkles.rectangle.stack",
                isOn: AppSettings.noSpriteLimitEnabled,
                action: #selector(noSpriteLimitChanged(_:))
            )
        case .lowpassFilter:
            return toggleCell(
                title: L10n("Settings::AnalogVideoFilter"),
                detail: L10n("Settings::AnalogVideoFilterDetail"),
                image: "waveform.path.ecg.rectangle",
                isOn: AppSettings.lowpassFilterEnabled,
                action: #selector(lowpassFilterChanged(_:))
            )
        case .lowpassIntensity:
            return optionCell(
                title: L10n("Settings::VideoFilterIntensity"),
                value: "\(AppSettings.lowpassIntensity)%",
                image: "slider.horizontal.3",
                isEnabled: AppSettings.lowpassFilterEnabled
            )
        case .lowpassCutoff:
            return optionCell(
                title: L10n("Settings::VideoFilterCutoff"),
                value: AppSettings.lowpassCutoff.title,
                image: "waveform",
                isEnabled: AppSettings.lowpassFilterEnabled
            )
        case .screenSize:
            return optionCell(
                title: L10n("Settings::ScreenSize"),
                value: AppSettings.screenSize.title,
                image: "arrow.up.left.and.arrow.down.right"
            )
        case .smoothing:
            return toggleCell(
                title: L10n("Settings::Smoothing"),
                detail: L10n("Settings::SmoothingDetail"),
                image: "square.resize",
                isOn: AppSettings.smoothingEnabled,
                action: #selector(smoothingChanged(_:))
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func inputCell(row: Int) -> UITableViewCell {
        switch InputRow(rawValue: row) {
        case .controller:
            return optionCell(
                title: L10n("Settings::Controller"),
                value: AppSettings.controller.title,
                image: "gamecontroller.fill"
            )
        case .avenuePad3MainButton:
            return optionCell(
                title: L10n("Settings::AvenuePad3Button"),
                value: AppSettings.avenuePad3MainButton.title,
                image: "button.horizontal",
                isEnabled: AppSettings.controller == .avenuePad3
            )
        case .softReset:
            return toggleCell(
                title: L10n("Settings::SoftReset"),
                detail: L10n("Settings::SoftResetDetail"),
                image: "arrow.counterclockwise",
                isOn: AppSettings.softResetEnabled,
                action: #selector(softResetChanged(_:))
            )
        case .turboI:
            return toggleCell(
                title: L10n("Settings::TurboI"),
                detail: L10n("Settings::TurboIDetail"),
                image: "bolt.fill",
                isOn: AppSettings.turboIEnabled,
                action: #selector(turboIChanged(_:))
            )
        case .turboISpeed:
            return optionCell(
                title: L10n("Settings::TurboISpeed"),
                value: String(AppSettings.turboISpeed),
                image: "speedometer",
                isEnabled: AppSettings.turboIEnabled
            )
        case .turboII:
            return toggleCell(
                title: L10n("Settings::TurboII"),
                detail: L10n("Settings::TurboIIDetail"),
                image: "bolt.fill",
                isOn: AppSettings.turboIIEnabled,
                action: #selector(turboIIChanged(_:))
            )
        case .turboIISpeed:
            return optionCell(
                title: L10n("Settings::TurboIISpeed"),
                value: String(AppSettings.turboIISpeed),
                image: "speedometer",
                isEnabled: AppSettings.turboIIEnabled
            )
        case .haptics:
            return toggleCell(
                title: L10n("Settings::Haptics"),
                detail: L10n("Settings::HapticsDetail"),
                image: "hand.tap",
                isOn: AppSettings.hapticsEnabled,
                action: #selector(hapticsChanged(_:))
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func audioCell(row: Int) -> UITableViewCell {
        switch AudioRow(rawValue: row) {
        case .psgRevision:
            return optionCell(
                title: L10n("Settings::PSGRevision"),
                value: AppSettings.psgRevision.title,
                image: "cpu"
            )
        case .psgVolume:
            return optionCell(
                title: L10n("Settings::PSGVolume"),
                value: "\(AppSettings.psgVolume)%",
                image: "waveform"
            )
        case .cdromVolume:
            return optionCell(
                title: L10n("Settings::CDROMVolume"),
                value: "\(AppSettings.cdromVolume)%",
                image: "opticaldisc"
            )
        case .adpcmVolume:
            return optionCell(
                title: L10n("Settings::ADPCMVolume"),
                value: "\(AppSettings.adpcmVolume)%",
                image: "waveform.badge.mic"
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func cdromCell(row: Int) -> UITableViewCell {
        switch CDROMRow(rawValue: row) {
        case .type:
            return optionCell(title: L10n("Settings::CDROMType"), value: AppSettings.cdromType.title, image: "opticaldisc")
        case .bios:
            return optionCell(title: L10n("Settings::CDROMBIOS"), value: AppSettings.cdromBIOS.title, image: "cpu")
        case .preload:
            return toggleCell(
                title: L10n("Settings::PreloadCDROM"),
                detail: L10n("Settings::PreloadCDROMDetail"),
                image: "memorychip",
                isOn: AppSettings.preloadCDROMEnabled,
                action: #selector(preloadCDROMChanged(_:))
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func firmwareCell(row: Int) -> UITableViewCell {
        guard Firmware.allCases.indices.contains(row) else { return UITableViewCell() }
        let firmware = Firmware.allCases[row]
        let installed = FirmwareStore.isInstalled(firmware)
        let cell = baseCell(
            title: String(format: L10n("Settings::ImportFirmware"), firmware.title),
            detail: installed ? L10n("Settings::Installed") : L10n("Settings::NotInstalled"),
            image: "square.and.arrow.down"
        )
        cell.textLabel?.textColor = view.tintColor
        cell.selectionStyle = .default
        cell.accessoryType = .disclosureIndicator
        return cell
    }

    private func libraryCell(row: Int) -> UITableViewCell {
        switch LibraryRow(rawValue: row) {
        case .refresh:
            let cell = baseCell(
                title: L10n("Settings::RefreshLibrary"),
                detail: L10n("Settings::RefreshLibraryDetail"),
                image: "arrow.clockwise"
            )
            cell.textLabel?.textColor = view.tintColor
            cell.selectionStyle = .default
            return cell
        case .importedRoms:
            let cell = baseCell(title: L10n("Settings::ImportedRoms"), detail: nil, image: "memorychip")
            cell.detailTextLabel?.text = String(dataStore.allRoms.count)
            return cell
        case nil:
            return UITableViewCell()
        }
    }

    private func aboutCell() -> UITableViewCell {
        let cell = baseCell(title: L10n("Settings::Version"), detail: nil, image: "info.circle")
        let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "-"
        let build = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "-"
        cell.detailTextLabel?.text = "\(version) (\(build))"
        return cell
    }

    private func importFirmware(_ firmware: Firmware) {
        pendingFirmware = firmware
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.data], asCopy: true)
        picker.delegate = self
        picker.allowsMultipleSelection = false
        present(picker, animated: true)
    }

    private func baseCell(title: String, detail: String?, image: String) -> UITableViewCell {
        let style: UITableViewCell.CellStyle = detail == nil ? .value1 : .subtitle
        let cell = UITableViewCell(style: style, reuseIdentifier: nil)
        cell.textLabel?.text = title
        cell.textLabel?.adjustsFontSizeToFitWidth = true
        cell.textLabel?.minimumScaleFactor = 0.78
        cell.textLabel?.allowsDefaultTighteningForTruncation = true
        cell.detailTextLabel?.text = detail
        cell.detailTextLabel?.adjustsFontSizeToFitWidth = true
        cell.detailTextLabel?.minimumScaleFactor = 0.75
        cell.imageView?.image = UIImage(systemName: image)
        cell.imageView?.tintColor = view.tintColor
        cell.selectionStyle = .none
        return cell
    }

    private func optionCell(title: String, value: String, image: String, isEnabled: Bool = true) -> UITableViewCell {
        let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)
        cell.textLabel?.text = title
        cell.textLabel?.adjustsFontSizeToFitWidth = true
        cell.textLabel?.minimumScaleFactor = 0.78
        cell.textLabel?.allowsDefaultTighteningForTruncation = true
        cell.detailTextLabel?.text = value
        cell.detailTextLabel?.adjustsFontSizeToFitWidth = true
        cell.detailTextLabel?.minimumScaleFactor = 0.7
        cell.imageView?.image = UIImage(systemName: image)
        cell.accessoryType = .disclosureIndicator
        cell.selectionStyle = isEnabled ? .default : .none
        applyEnabledState(isEnabled, to: cell)
        return cell
    }

    private func toggleCell(
        title: String,
        detail: String,
        image: String,
        isOn: Bool,
        action: Selector,
        isEnabled: Bool = true
    ) -> UITableViewCell {
        let cell = UITableViewCell(style: .subtitle, reuseIdentifier: nil)
        cell.textLabel?.text = title
        cell.textLabel?.adjustsFontSizeToFitWidth = true
        cell.textLabel?.minimumScaleFactor = 0.78
        cell.textLabel?.allowsDefaultTighteningForTruncation = true
        cell.detailTextLabel?.text = detail
        cell.detailTextLabel?.adjustsFontSizeToFitWidth = true
        cell.detailTextLabel?.minimumScaleFactor = 0.75
        cell.imageView?.image = UIImage(systemName: image)
        cell.selectionStyle = .none

        let toggle = UISwitch()
        toggle.isOn = isOn
        toggle.isEnabled = isEnabled
        toggle.accessibilityLabel = title
        toggle.addTarget(self, action: action, for: .valueChanged)
        cell.accessoryView = toggle
        applyEnabledState(isEnabled, to: cell)
        return cell
    }

    private func applyEnabledState(_ isEnabled: Bool, to cell: UITableViewCell) {
        cell.isUserInteractionEnabled = isEnabled
        cell.textLabel?.textColor = isEnabled ? .label : .tertiaryLabel
        cell.detailTextLabel?.textColor = isEnabled ? .secondaryLabel : .tertiaryLabel
        cell.imageView?.tintColor = isEnabled ? view.tintColor : .tertiaryLabel
    }

    private func showOptions(
        title: String,
        options: [String],
        selectedIndex: Int,
        onSelection: @escaping (Int) -> Void
    ) {
        let controller = OptionSelectionViewController(
            title: title,
            optionTitles: options,
            selectedIndex: selectedIndex,
            onSelection: onSelection
        )
        navigationController?.pushViewController(controller, animated: true)
    }

    private func showTurboSpeedOptions(title: String, currentValue: Int, onSelection: @escaping (Int) -> Void) {
        showOptions(
            title: title,
            options: (1...20).map(String.init),
            selectedIndex: currentValue - 1
        ) { onSelection($0 + 1) }
    }

    private func showVolumeOptions(title: String, currentValue: Int, onSelection: @escaping (Int) -> Void) {
        let values = Array(stride(from: 0, through: 200, by: 10))
        let selectedIndex = values.enumerated().min(by: {
            abs($0.element - currentValue) < abs($1.element - currentValue)
        })?.offset ?? 10
        showOptions(
            title: title,
            options: values.map { "\($0)%" },
            selectedIndex: selectedIndex
        ) { onSelection(values[$0]) }
    }

    @objc private func audioChanged(_ sender: UISwitch) {
        AppSettings.audioEnabled = sender.isOn
    }

    @objc private func safeVDCDefaultsChanged(_ sender: UISwitch) {
        AppSettings.safeVDCDefaultsEnabled = sender.isOn
    }

    @objc private func overscanChanged(_ sender: UISwitch) {
        AppSettings.overscanEnabled = sender.isOn
    }

    @objc private func noSpriteLimitChanged(_ sender: UISwitch) {
        AppSettings.noSpriteLimitEnabled = sender.isOn
    }

    @objc private func lowpassFilterChanged(_ sender: UISwitch) {
        AppSettings.lowpassFilterEnabled = sender.isOn
        tableView.reloadSections(IndexSet(integer: Section.video.rawValue), with: .automatic)
    }

    @objc private func smoothingChanged(_ sender: UISwitch) {
        AppSettings.smoothingEnabled = sender.isOn
    }

    @objc private func softResetChanged(_ sender: UISwitch) {
        AppSettings.softResetEnabled = sender.isOn
    }

    @objc private func turboIChanged(_ sender: UISwitch) {
        AppSettings.turboIEnabled = sender.isOn
        tableView.reloadSections(IndexSet(integer: Section.input.rawValue), with: .automatic)
    }

    @objc private func turboIIChanged(_ sender: UISwitch) {
        AppSettings.turboIIEnabled = sender.isOn
        tableView.reloadSections(IndexSet(integer: Section.input.rawValue), with: .automatic)
    }

    @objc private func hapticsChanged(_ sender: UISwitch) {
        AppSettings.hapticsEnabled = sender.isOn
    }

    @objc private func preloadCDROMChanged(_ sender: UISwitch) {
        AppSettings.preloadCDROMEnabled = sender.isOn
    }
}

extension SettingsViewController: UIDocumentPickerDelegate {
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        guard let firmware = pendingFirmware, let sourceURL = urls.first else { return }
        pendingFirmware = nil

        do {
            try FirmwareStore.importFile(at: sourceURL, as: firmware)
            tableView.reloadSections(IndexSet(integer: Section.firmware.rawValue), with: .automatic)
        } catch {
            let alert = UIAlertController(
                title: L10n("Settings::FirmwareImportFailed"),
                message: firmware.validationErrorMessage,
                preferredStyle: .alert
            )
            alert.addAction(UIAlertAction(title: L10n("Common::OK"), style: .default))
            present(alert, animated: true)
        }
    }

    func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
        pendingFirmware = nil
    }
}
