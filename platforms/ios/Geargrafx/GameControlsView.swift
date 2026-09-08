import UIKit

final class GameControlsView: UIView {
    var onButtonChanged: ((GeargrafxButton, Bool) -> Void)? {
        didSet {
            select.onButtonChanged = onButtonChanged
            run.onButtonChanged = onButtonChanged
            for button in actionButtons {
                button.onButtonChanged = onButtonChanged
            }
        }
    }

    var hapticsEnabled = true {
        didSet {
            dPad.hapticsEnabled = hapticsEnabled
            select.hapticsEnabled = hapticsEnabled
            run.hapticsEnabled = hapticsEnabled
            for button in actionButtons {
                button.hapticsEnabled = hapticsEnabled
            }
        }
    }

    let dPad = DirectionPadView()
    let actionContainer = UIStackView()
    let select = GameControlButton(title: "SELECT", button: .select, shape: .capsule)
    let run = GameControlButton(title: "RUN", button: .run, shape: .capsule)

    private let controllerType: GeargrafxControllerOption
    private var actionButtons = [GameControlButton]()
    private var portraitConstraints = [NSLayoutConstraint]()
    private var landscapeConstraints = [NSLayoutConstraint]()
    private var portraitBottomConstraints = [NSLayoutConstraint]()
    private var usingLandscapeConstraints = false

    init(controllerType: GeargrafxControllerOption) {
        self.controllerType = controllerType
        super.init(frame: .zero)
        configure()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func layoutSubviews() {
        let landscape = bounds.width > bounds.height
        if landscape != usingLandscapeConstraints {
            usingLandscapeConstraints = landscape
            NSLayoutConstraint.deactivate(landscape ? portraitConstraints : landscapeConstraints)
            NSLayoutConstraint.activate(landscape ? landscapeConstraints : portraitConstraints)
        }

        super.layoutSubviews()
    }

    func positionPortraitControls(after screenBottom: CGFloat) -> Bool {
        guard UIDevice.current.userInterfaceIdiom == .pad,
              bounds.height > bounds.width,
              dPad.bounds.height > 0.0,
              let primaryConstraint = portraitBottomConstraints.first else { return false }

        let minimumGap: CGFloat = 44.0
        let minimumBottomInset: CGFloat = 24.0
        let safeFrame = safeAreaLayoutGuide.layoutFrame
        let bottomInset = max(
            safeFrame.maxY - screenBottom - minimumGap - dPad.bounds.height,
            minimumBottomInset
        )
        let constant = -bottomInset
        guard abs(primaryConstraint.constant - constant) > 0.5 else { return false }

        for constraint in portraitBottomConstraints {
            constraint.constant = constant
        }
        return true
    }

    private func configure() {
        isMultipleTouchEnabled = true
        backgroundColor = .clear
        dPad.onDirectionChanged = { [weak self] direction, pressed in
            guard let self else { return }
            self.onButtonChanged?(self.emulatorButton(for: direction), pressed)
        }

        dPad.translatesAutoresizingMaskIntoConstraints = false
        actionContainer.translatesAutoresizingMaskIntoConstraints = false
        select.translatesAutoresizingMaskIntoConstraints = false
        run.translatesAutoresizingMaskIntoConstraints = false

        configureActionButtons()

        addSubview(dPad)
        addSubview(actionContainer)
        addSubview(select)
        addSubview(run)

        let isPad = UIDevice.current.userInterfaceIdiom == .pad
        let dPadSize: CGFloat = isPad ? 176.0 : 132.0
        let actionSize: CGFloat

        switch controllerType {
        case .standard:
            actionSize = isPad ? 88.0 : 72.0
        case .avenuePad3:
            actionSize = isPad ? 76.0 : 62.0
        case .avenuePad6:
            actionSize = isPad ? 64.0 : 52.0
        }

        let actionSpacing: CGFloat = isPad ? 12.0 : 9.0
        let columns = min(controllerType.actionCount, 3)
        let rows = (controllerType.actionCount + 2) / 3
        let actionWidth = (CGFloat(columns) * actionSize) + (CGFloat(columns - 1) * actionSpacing)
        let actionHeight = (CGFloat(rows) * actionSize) + (CGFloat(rows - 1) * actionSpacing)
        let primaryOffset: CGFloat = isPad ? 220.0 : 120.0
        let menuWidth: CGFloat = isPad ? 104.0 : 80.0
        let portraitBottomInset: CGFloat = isPad ? 160.0 : 24.0
        let portraitPrimaryConstraint = isPad
            ? dPad.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -portraitBottomInset)
            : dPad.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor, constant: primaryOffset)
        let portraitSelectConstraint = select.bottomAnchor.constraint(
            equalTo: safeAreaLayoutGuide.bottomAnchor,
            constant: -portraitBottomInset
        )

        if isPad {
            portraitBottomConstraints = [portraitPrimaryConstraint, portraitSelectConstraint]
        }

        for button in actionButtons {
            NSLayoutConstraint.activate([
                button.widthAnchor.constraint(equalToConstant: actionSize),
                button.heightAnchor.constraint(equalTo: button.widthAnchor)
            ])
        }

        actionContainer.spacing = actionSpacing
        for row in actionContainer.arrangedSubviews {
            (row as? UIStackView)?.spacing = actionSpacing
        }

        NSLayoutConstraint.activate([
            dPad.widthAnchor.constraint(equalToConstant: dPadSize),
            dPad.heightAnchor.constraint(equalTo: dPad.widthAnchor),

            actionContainer.widthAnchor.constraint(equalToConstant: actionWidth),
            actionContainer.heightAnchor.constraint(equalToConstant: actionHeight),

            select.widthAnchor.constraint(equalToConstant: menuWidth),
            select.heightAnchor.constraint(equalToConstant: 44.0),
            run.widthAnchor.constraint(equalTo: select.widthAnchor),
            run.heightAnchor.constraint(equalTo: select.heightAnchor)
        ])

        portraitConstraints = [
            dPad.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 20.0),
            actionContainer.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -20.0),
            portraitPrimaryConstraint,
            actionContainer.centerYAnchor.constraint(equalTo: dPad.centerYAnchor),
            select.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.centerXAnchor, constant: -12.0),
            run.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.centerXAnchor, constant: 12.0),
            portraitSelectConstraint,
            run.bottomAnchor.constraint(equalTo: select.bottomAnchor)
        ]

        landscapeConstraints = [
            dPad.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 8.0),
            actionContainer.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -8.0),
            dPad.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor),
            actionContainer.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor),
            select.centerXAnchor.constraint(equalTo: dPad.centerXAnchor),
            run.centerXAnchor.constraint(equalTo: actionContainer.centerXAnchor),
            select.bottomAnchor.constraint(
                equalTo: safeAreaLayoutGuide.bottomAnchor,
                constant: isPad ? -24.0 : -8.0
            ),
            run.bottomAnchor.constraint(equalTo: select.bottomAnchor)
        ]

        NSLayoutConstraint.activate(portraitConstraints)
    }

    private func emulatorButton(for direction: DirectionPadDirection) -> GeargrafxButton {
        switch direction {
        case .up: return .up
        case .down: return .down
        case .left: return .left
        case .right: return .right
        }
    }

    private func configureActionButtons() {
        let definitions: [(String, GeargrafxButton)]

        switch controllerType {
        case .standard:
            definitions = [("II", .II), ("I", .I)]
        case .avenuePad3:
            definitions = [("III", .III), ("II", .II), ("I", .I)]
        case .avenuePad6:
            definitions = [
                ("IV", .IV), ("V", .V), ("VI", .VI),
                ("III", .III), ("II", .II), ("I", .I)
            ]
        }

        actionButtons = definitions.map { title, button in
            GameControlButton(title: title, button: button, shape: .circle)
        }

        let rows = stride(from: 0, to: actionButtons.count, by: 3).map { startIndex -> UIStackView in
            let endIndex = min(startIndex + 3, actionButtons.count)
            let row = UIStackView(arrangedSubviews: Array(actionButtons[startIndex..<endIndex]))
            row.axis = .horizontal
            row.distribution = .fillEqually
            return row
        }

        actionContainer.axis = .vertical
        actionContainer.distribution = .fillEqually
        for row in rows {
            actionContainer.addArrangedSubview(row)
        }
    }
}

final class GameControlButton: UIButton {
    enum Shape {
        case circle
        case capsule
    }

    var onButtonChanged: ((GeargrafxButton, Bool) -> Void)?
    var hapticsEnabled = true

    private let emulatorButton: GeargrafxButton
    private let shape: Shape
    private var pressed = false
    private let feedback = UIImpactFeedbackGenerator(style: .light)

    init(title: String, button: GeargrafxButton, shape: Shape) {
        self.emulatorButton = button
        self.shape = shape
        super.init(frame: .zero)

        setTitle(title, for: .normal)
        setTitleColor(.label, for: .normal)
        titleLabel?.font = shape == .circle
            ? .systemFont(ofSize: title.count > 2 ? 14.0 : 20.0, weight: .bold)
            : .systemFont(ofSize: 11.0, weight: .semibold)
        backgroundColor = UIColor.secondarySystemFill.withAlphaComponent(0.92)
        layer.borderColor = UIColor.separator.withAlphaComponent(0.65).cgColor
        layer.borderWidth = 1.0
        accessibilityLabel = title

        addTarget(self, action: #selector(press), for: [.touchDown, .touchDragEnter])
        addTarget(self, action: #selector(releaseButton), for: [.touchUpInside, .touchUpOutside, .touchCancel, .touchDragExit])
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        layer.cornerRadius = shape == .circle ? bounds.width * 0.5 : bounds.height * 0.5
    }

    @objc private func press() {
        guard !pressed else { return }
        pressed = true
        if hapticsEnabled {
            feedback.prepare()
            feedback.impactOccurred(intensity: 0.55)
        }
        backgroundColor = tintColor.withAlphaComponent(0.28)
        transform = CGAffineTransform(scaleX: 0.94, y: 0.94)
        onButtonChanged?(emulatorButton, true)
    }

    @objc private func releaseButton() {
        guard pressed else { return }
        pressed = false
        backgroundColor = UIColor.secondarySystemFill.withAlphaComponent(0.92)
        transform = .identity
        onButtonChanged?(emulatorButton, false)
    }
}
