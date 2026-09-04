/*
 * Geargrafx - PC Engine / TurboGrafx-16 Emulator
 * Copyright (C) 2024 Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#import "GeargrafxEmulator.h"

#import <AVFAudio/AVFAudio.h>

#include <algorithm>
#include <string.h>
#include <strings.h>

#include "IOSAudioQueue.h"

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#undef MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#undef MIN
#undef MAX
#include "../../../src/geargrafx.h"

bool g_mcp_stdio_mode = false;

static NSString* const GeargrafxEmulatorErrorDomain = @"me.ignaciosanchez.geargrafx.emulator";
static const int kMaxFrameWidth = 1120;
static const int kMaxFrameHeight = 242;

static bool IsROMArchiveEntry(const char* filename)
{
    const char* extension = strrchr(filename, '.');
    if (!extension)
        return false;

    return (strcasecmp(extension, ".pce") == 0) ||
           (strcasecmp(extension, ".sgx") == 0) ||
           (strcasecmp(extension, ".hes") == 0) ||
           (strcasecmp(extension, ".rom") == 0) ||
           (strcasecmp(extension, ".cue") == 0);
}

static NSString* ROMCRCInArchive(NSURL* url)
{
    if (!url.isFileURL)
        return nil;

    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_reader_init_file(&archive, url.fileSystemRepresentation, 0))
        return nil;

    NSString* result = nil;
    mz_uint fileCount = mz_zip_reader_get_num_files(&archive);
    for (mz_uint index = 0; index < fileCount; index++)
    {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&archive, index, &fileStat))
            break;
        if (!IsROMArchiveEntry(fileStat.m_filename))
            continue;

        size_t size = 0;
        void* data = mz_zip_reader_extract_to_heap(&archive, index, &size, 0);
        if (!data)
            break;

        mz_ulong checksum = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)data, size);
        free(data);
        result = [NSString stringWithFormat:@"%08X", (unsigned int)checksum];
        break;
    }

    mz_zip_reader_end(&archive);
    return result;
}

static void InputPump()
{
}

static GG_Console_Type ConsoleForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GG_CONSOLE_PCE;
        case 2:
            return GG_CONSOLE_SGX;
        case 3:
            return GG_CONSOLE_TG16;
        default:
            return GG_CONSOLE_AUTO;
    }
}

static GG_PSG_Revision PSGRevisionForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GG_PSG_REVISION_HUC6280;
        case 2:
            return GG_PSG_REVISION_HUC6280A;
        default:
            return GG_PSG_REVISION_AUTO;
    }
}

static GG_Controller_Type ControllerForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GG_CONTROLLER_AVENUE_PAD_3;
        case 2:
            return GG_CONTROLLER_AVENUE_PAD_6;
        default:
            return GG_CONTROLLER_STANDARD;
    }
}

static GG_Keys AvenuePad3ButtonForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GG_KEY_SELECT;
        case 2:
            return GG_KEY_RUN;
        default:
            return GG_KEY_NONE;
    }
}

static GG_CDROM_Type CDROMTypeForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GG_CDROM_STANDARD;
        case 2:
            return GG_CDROM_SUPER_CDROM;
        case 3:
            return GG_CDROM_ARCADE_CARD;
        default:
            return GG_CDROM_AUTO;
    }
}

static GG_Keys KeyForButton(GeargrafxButton button)
{
    switch (button)
    {
        case GeargrafxButtonUp: return GG_KEY_UP;
        case GeargrafxButtonDown: return GG_KEY_DOWN;
        case GeargrafxButtonLeft: return GG_KEY_LEFT;
        case GeargrafxButtonRight: return GG_KEY_RIGHT;
        case GeargrafxButtonI: return GG_KEY_I;
        case GeargrafxButtonII: return GG_KEY_II;
        case GeargrafxButtonIII: return GG_KEY_III;
        case GeargrafxButtonIV: return GG_KEY_IV;
        case GeargrafxButtonV: return GG_KEY_V;
        case GeargrafxButtonVI: return GG_KEY_VI;
        case GeargrafxButtonSelect: return GG_KEY_SELECT;
        case GeargrafxButtonRun: return GG_KEY_RUN;
    }

    return GG_KEY_NONE;
}

static BOOL IsCDROMURL(NSURL* url)
{
    NSString* extension = url.pathExtension.lowercaseString;
    return [extension isEqualToString:@"cue"] || [extension isEqualToString:@"chd"];
}

@interface GeargrafxEmulator ()
{
    GeargrafxCore* m_core;
    u16* m_frameBuffer;
    s16* m_audioBuffer;
    IOSAudioQueue m_audioQueue;
    uint32_t m_pressedButtons;
    BOOL m_loaded;
    BOOL m_muted;
    BOOL m_overscan;
    BOOL m_noSpriteLimit;
    BOOL m_safeVDCDefaults;
    BOOL m_lowpassFilter;
    BOOL m_softResetEnabled;
    BOOL m_turboIEnabled;
    BOOL m_turboIIEnabled;
    BOOL m_preloadCDROM;
    NSInteger m_console;
    NSInteger m_palette;
    NSInteger m_scanlineMode;
    NSInteger m_scanlineStart;
    NSInteger m_scanlineEnd;
    NSInteger m_lowpassIntensity;
    NSInteger m_lowpassCutoff;
    NSInteger m_controllerType;
    NSInteger m_avenuePad3MainButton;
    NSInteger m_turboISpeed;
    NSInteger m_turboIISpeed;
    NSInteger m_psgRevision;
    NSInteger m_psgVolume;
    NSInteger m_cdromVolume;
    NSInteger m_adpcmVolume;
    NSInteger m_cdromType;
    NSInteger m_cdromBIOS;
    NSInteger m_saveStateSlot;
    NSInteger m_frameWidth;
    NSInteger m_frameHeight;
    NSInteger m_frameWidthScale;
    double m_framesPerSecond;
    NSURL* m_firmwareDirectory;
    AVAudioEngine* m_audioEngine;
    AVAudioSourceNode* m_audioSourceNode;
}

- (void)applyConfiguration;
- (BOOL)loadFirmwareForURL:(NSURL*)url error:(NSError**)error;
- (void)updateRuntimeInfo;
- (void)configureAudio;
- (void)audioEngineConfigurationChanged:(NSNotification*)notification;
- (void)clearAudio;
- (void)enqueueAudioSamples:(const s16*)samples count:(int)count;
- (OSStatus)renderAudioFrames:(AVAudioFrameCount)frameCount outputData:(AudioBufferList*)outputData silence:(BOOL*)isSilence;

@end

@implementation GeargrafxEmulator

+ (NSString*)romCRCInArchiveAtURL:(NSURL*)url
{
    return ROMCRCInArchive(url);
}

- (instancetype)init
{
    self = [super init];

    if (self)
    {
        m_core = new GeargrafxCore();
        m_core->Init(InputPump, GG_PIXEL_RGB565);
        NSURL* temporaryDirectory = NSFileManager.defaultManager.temporaryDirectory;
        m_core->GetMedia()->SetTempPath(temporaryDirectory.fileSystemRepresentation);

        m_frameBuffer = new u16[kMaxFrameWidth * kMaxFrameHeight]();
        m_audioBuffer = new s16[GG_AUDIO_BUFFER_SIZE]();
        m_audioQueue.Configure(GG_AUDIO_QUEUE_SIZE, 3);
        m_pressedButtons = 0;
        m_loaded = NO;
        m_muted = NO;
        m_overscan = NO;
        m_noSpriteLimit = NO;
        m_safeVDCDefaults = NO;
        m_lowpassFilter = NO;
        m_softResetEnabled = YES;
        m_turboIEnabled = NO;
        m_turboIIEnabled = NO;
        m_preloadCDROM = NO;
        m_console = 0;
        m_palette = 0;
        m_scanlineMode = 0;
        m_scanlineStart = 11;
        m_scanlineEnd = 234;
        m_lowpassIntensity = 100;
        m_lowpassCutoff = 50;
        m_controllerType = 0;
        m_avenuePad3MainButton = 0;
        m_turboISpeed = 4;
        m_turboIISpeed = 4;
        m_psgRevision = 0;
        m_psgVolume = 100;
        m_cdromVolume = 100;
        m_adpcmVolume = 100;
        m_cdromType = 0;
        m_cdromBIOS = 0;
        m_saveStateSlot = 1;
        m_frameWidth = 256;
        m_frameHeight = 224;
        m_frameWidthScale = 1;
        m_framesPerSecond = 60.0;

        [self applyConfiguration];
        [self configureAudio];
    }

    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [self stopAudio];

    if (m_loaded)
    {
        m_core->SaveRam();
    }

    SafeDeleteArray(m_audioBuffer);
    SafeDeleteArray(m_frameBuffer);
    SafeDelete(m_core);
}

- (void)configureWithConsole:(NSInteger)console
                    palette:(NSInteger)palette
                   overscan:(BOOL)overscan
               scanlineMode:(NSInteger)scanlineMode
              scanlineStart:(NSInteger)scanlineStart
                scanlineEnd:(NSInteger)scanlineEnd
             noSpriteLimit:(BOOL)noSpriteLimit
            safeVDCDefaults:(BOOL)safeVDCDefaults
             lowpassFilter:(BOOL)lowpassFilter
          lowpassIntensity:(NSInteger)lowpassIntensity
             lowpassCutoff:(NSInteger)lowpassCutoff
             controllerType:(NSInteger)controllerType
        avenuePad3MainButton:(NSInteger)avenuePad3MainButton
           softResetEnabled:(BOOL)softResetEnabled
              turboIEnabled:(BOOL)turboIEnabled
                turboISpeed:(NSInteger)turboISpeed
             turboIIEnabled:(BOOL)turboIIEnabled
               turboIISpeed:(NSInteger)turboIISpeed
                psgRevision:(NSInteger)psgRevision
                  psgVolume:(NSInteger)psgVolume
                cdromVolume:(NSInteger)cdromVolume
                adpcmVolume:(NSInteger)adpcmVolume
                  cdromType:(NSInteger)cdromType
                  cdromBIOS:(NSInteger)cdromBIOS
               preloadCDROM:(BOOL)preloadCDROM
             saveStateSlot:(NSInteger)saveStateSlot
          firmwareDirectory:(NSURL*)firmwareDirectory
{
    m_console = console;
    m_palette = palette;
    m_overscan = overscan;
    m_scanlineMode = scanlineMode;
    m_scanlineStart = scanlineStart;
    m_scanlineEnd = scanlineEnd;
    m_noSpriteLimit = noSpriteLimit;
    m_safeVDCDefaults = safeVDCDefaults;
    m_lowpassFilter = lowpassFilter;
    m_lowpassIntensity = std::min(std::max(lowpassIntensity, (NSInteger)0), (NSInteger)100);
    m_lowpassCutoff = std::min(std::max(lowpassCutoff, (NSInteger)30), (NSInteger)70);
    m_controllerType = controllerType;
    m_avenuePad3MainButton = avenuePad3MainButton;
    m_softResetEnabled = softResetEnabled;
    m_turboIEnabled = turboIEnabled;
    m_turboISpeed = std::min(std::max(turboISpeed, (NSInteger)1), (NSInteger)20);
    m_turboIIEnabled = turboIIEnabled;
    m_turboIISpeed = std::min(std::max(turboIISpeed, (NSInteger)1), (NSInteger)20);
    m_psgRevision = psgRevision;
    m_psgVolume = std::min(std::max(psgVolume, (NSInteger)0), (NSInteger)200);
    m_cdromVolume = std::min(std::max(cdromVolume, (NSInteger)0), (NSInteger)200);
    m_adpcmVolume = std::min(std::max(adpcmVolume, (NSInteger)0), (NSInteger)200);
    m_cdromType = cdromType;
    m_cdromBIOS = cdromBIOS;
    m_preloadCDROM = preloadCDROM;
    m_firmwareDirectory = firmwareDirectory;

    if (saveStateSlot < 1)
    {
        m_saveStateSlot = 1;
    }
    else if (saveStateSlot > 5)
    {
        m_saveStateSlot = 5;
    }
    else
    {
        m_saveStateSlot = saveStateSlot;
    }

    [self applyConfiguration];
}

- (void)applyConfiguration
{
    m_core->GetMedia()->SetConsoleType(ConsoleForOption(m_console));
    m_core->GetMedia()->SetCDROMType(CDROMTypeForOption(m_cdromType));
    m_core->GetMedia()->ForceBackupRAM(true);
    m_core->GetMedia()->PreloadCdRom(m_preloadCDROM);
    m_core->GetMedia()->ForceGameExpress(m_cdromBIOS == 3);
    m_core->GetHuC6260()->SetPalette((int)m_palette);
    m_core->GetHuC6260()->SetOverscan(m_overscan);

    if (m_scanlineMode == 1)
    {
        m_core->GetHuC6260()->SetScanlineStart(2);
        m_core->GetHuC6260()->SetScanlineEnd(241);
    }
    else if (m_scanlineMode == 2)
    {
        m_core->GetHuC6260()->SetScanlineStart(0);
        m_core->GetHuC6260()->SetScanlineEnd(241);
    }
    else if (m_scanlineMode == 3)
    {
        m_core->GetHuC6260()->SetScanlineStart((int)m_scanlineStart);
        m_core->GetHuC6260()->SetScanlineEnd((int)m_scanlineEnd);
    }
    else
    {
        m_core->GetHuC6260()->SetScanlineStart(11);
        m_core->GetHuC6260()->SetScanlineEnd(234);
    }

    m_core->GetHuC6270_1()->SetNoSpriteLimit(m_noSpriteLimit);
    m_core->GetHuC6270_2()->SetNoSpriteLimit(m_noSpriteLimit);
    m_core->GetHuC6270_1()->SetSafeDefaults(m_safeVDCDefaults);
    m_core->GetHuC6270_2()->SetSafeDefaults(m_safeVDCDefaults);
    m_core->GetHuC6260()->SetLowPassFilter(m_lowpassFilter, (float)m_lowpassIntensity / 100.0f,
        (float)m_lowpassCutoff / 10.0f, false, true, true);
    m_core->GetInput()->SetControllerType(GG_CONTROLLER_1, ControllerForOption(m_controllerType));
    m_core->GetInput()->SetAvenuePad3Button(GG_CONTROLLER_1,
        AvenuePad3ButtonForOption(m_avenuePad3MainButton));
    m_core->GetInput()->EnableTurbo(GG_CONTROLLER_1, GG_KEY_I, m_turboIEnabled);
    m_core->GetInput()->SetTurboSpeed(GG_CONTROLLER_1, GG_KEY_I, (u8)m_turboISpeed);
    m_core->GetInput()->EnableTurbo(GG_CONTROLLER_1, GG_KEY_II, m_turboIIEnabled);
    m_core->GetInput()->SetTurboSpeed(GG_CONTROLLER_1, GG_KEY_II, (u8)m_turboIISpeed);
    m_core->SetPSGRevision(PSGRevisionForOption(m_psgRevision));
    m_core->GetAudio()->SetPSGVolume((float)m_psgVolume / 100.0f);
    m_core->GetAudio()->SetCDROMVolume((float)m_cdromVolume / 100.0f);
    m_core->GetAudio()->SetADPCMVolume((float)m_adpcmVolume / 100.0f);
    m_core->EnableMB128(GG_MB128_AUTO);
}

- (BOOL)loadFirmwareForURL:(NSURL*)url error:(NSError**)error
{
    if (!IsCDROMURL(url))
    {
        return YES;
    }

    if (!m_firmwareDirectory)
    {
        if (error)
        {
            *error = [NSError errorWithDomain:GeargrafxEmulatorErrorDomain
                                         code:2
                                     userInfo:@{NSLocalizedDescriptionKey:
                                         @"No firmware directory is available for CD-ROM games."}];
        }

        return NO;
    }

    m_core->UnloadBios(true);
    m_core->UnloadBios(false);

    NSString* systemCardName = @"syscard3.pce";
    if (m_cdromBIOS == 1)
    {
        systemCardName = @"syscard2.pce";
    }
    else if (m_cdromBIOS == 2)
    {
        systemCardName = @"syscard1.pce";
    }

    NSURL* systemCardURL = [m_firmwareDirectory URLByAppendingPathComponent:systemCardName];
    NSURL* gameExpressURL = [m_firmwareDirectory URLByAppendingPathComponent:@"gexpress.pce"];
    BOOL systemCardLoaded = [NSFileManager.defaultManager fileExistsAtPath:systemCardURL.path] &&
        m_core->LoadBios(systemCardURL.fileSystemRepresentation, true);
    BOOL gameExpressLoaded = [NSFileManager.defaultManager fileExistsAtPath:gameExpressURL.path] &&
        m_core->LoadBios(gameExpressURL.fileSystemRepresentation, false);
    BOOL firmwareReady = (m_cdromBIOS == 3) ? gameExpressLoaded : systemCardLoaded;

    if (!firmwareReady && error)
    {
        NSString* missingName = (m_cdromBIOS == 3) ? @"gexpress.pce" : systemCardName;
        NSString* message = [NSString stringWithFormat:
            @"CD-ROM firmware %@ is not installed or is invalid. Import it in Settings.", missingName];
        *error = [NSError errorWithDomain:GeargrafxEmulatorErrorDomain
                                     code:2
                                 userInfo:@{NSLocalizedDescriptionKey: message}];
    }

    return firmwareReady;
}

- (void)updateRuntimeInfo
{
    GG_Runtime_Info runtimeInfo;

    if (m_core->GetRuntimeInfo(runtimeInfo))
    {
        m_frameWidth = runtimeInfo.screen_width;
        m_frameHeight = runtimeInfo.screen_height;
        m_frameWidthScale = runtimeInfo.width_scale;
        m_framesPerSecond = runtimeInfo.fps;
    }
}

- (BOOL)loadROMAtURL:(NSURL*)url error:(NSError**)error
{
    if (!url.isFileURL)
    {
        if (error)
        {
            *error = [NSError errorWithDomain:GeargrafxEmulatorErrorDomain
                                         code:1
                                     userInfo:@{NSLocalizedDescriptionKey: @"The selected item is not a local ROM file."}];
        }

        return NO;
    }

    if (m_loaded)
    {
        m_core->SaveRam();
    }

    [self releaseAllButtons];
    [self clearAudio];

    [self applyConfiguration];
    if (![self loadFirmwareForURL:url error:error])
    {
        m_loaded = NO;
        return NO;
    }

    BOOL loaded = m_core->LoadMedia(url.fileSystemRepresentation);

    if (!loaded)
    {
        m_loaded = NO;

        if (error)
        {
            *error = [NSError errorWithDomain:GeargrafxEmulatorErrorDomain
                                         code:3
                                     userInfo:@{NSLocalizedDescriptionKey: @"Geargrafx could not load this game."}];
        }

        return NO;
    }

    if (IsCDROMURL(url) && !m_core->GetMedia()->IsBiosReady())
    {
        m_loaded = NO;

        if (error)
        {
            *error = [NSError errorWithDomain:GeargrafxEmulatorErrorDomain
                                         code:4
                                     userInfo:@{NSLocalizedDescriptionKey:
                                         @"The installed CD-ROM firmware is not compatible with this game."}];
        }

        return NO;
    }

    m_core->LoadRam();
    [self applyConfiguration];
    m_core->Pause(false);
    m_loaded = YES;
    memset(m_frameBuffer, 0, kMaxFrameWidth * kMaxFrameHeight * sizeof(u16));
    [self updateRuntimeInfo];

    return YES;
}

- (void)runFrame
{
    if (!m_loaded || m_core->IsPaused())
    {
        return;
    }

    int sampleCount = 0;
    m_core->RunToVBlank(reinterpret_cast<u8*>(m_frameBuffer), m_audioBuffer, &sampleCount);
    [self updateRuntimeInfo];

    if (!m_muted && (sampleCount > 0))
    {
        [self enqueueAudioSamples:m_audioBuffer count:sampleCount];
    }
}

- (void)setButton:(GeargrafxButton)button pressed:(BOOL)pressed
{
    if (!m_loaded)
    {
        return;
    }

    uint32_t buttonMask = 1U << (uint32_t)button;
    BOOL wasPressed = (m_pressedButtons & buttonMask) != 0;

    if (pressed == wasPressed)
    {
        return;
    }

    if (pressed && !m_softResetEnabled)
    {
        uint32_t selectMask = 1U << (uint32_t)GeargrafxButtonSelect;
        uint32_t runMask = 1U << (uint32_t)GeargrafxButtonRun;

        if (((button == GeargrafxButtonSelect) && ((m_pressedButtons & runMask) != 0)) ||
            ((button == GeargrafxButtonRun) && ((m_pressedButtons & selectMask) != 0)))
        {
            return;
        }
    }

    GG_Keys key = KeyForButton(button);

    if (pressed)
    {
        m_pressedButtons |= buttonMask;
        m_core->KeyPressed(GG_CONTROLLER_1, key);
    }
    else
    {
        m_pressedButtons &= ~buttonMask;
        m_core->KeyReleased(GG_CONTROLLER_1, key);
    }
}

- (void)releaseAllButtons
{
    if (!m_core)
    {
        return;
    }

    static const GeargrafxButton buttons[] =
    {
        GeargrafxButtonUp,
        GeargrafxButtonDown,
        GeargrafxButtonLeft,
        GeargrafxButtonRight,
        GeargrafxButtonI,
        GeargrafxButtonII,
        GeargrafxButtonIII,
        GeargrafxButtonIV,
        GeargrafxButtonV,
        GeargrafxButtonVI,
        GeargrafxButtonSelect,
        GeargrafxButtonRun
    };

    for (GeargrafxButton button : buttons)
    {
        if ((m_pressedButtons & (1U << (uint32_t)button)) != 0)
        {
            [self setButton:button pressed:NO];
        }
    }
}

- (void)pause
{
    if (m_loaded)
    {
        [self releaseAllButtons];
        m_core->Pause(true);
    }
}

- (void)resume
{
    if (m_loaded)
    {
        m_core->Pause(false);
    }
}

- (void)reset
{
    if (!m_loaded)
    {
        return;
    }

    [self releaseAllButtons];
    m_core->SaveRam();
    [self applyConfiguration];
    m_core->ResetMedia(true);
    [self applyConfiguration];
    [self updateRuntimeInfo];
    [self clearAudio];
}

- (void)saveRAM
{
    if (m_loaded)
    {
        m_core->SaveRam();
    }
}

- (void)saveState
{
    if (m_loaded)
    {
        m_core->SaveState(NULL, (int)m_saveStateSlot);
    }
}

- (void)loadState
{
    if (m_loaded)
    {
        [self releaseAllButtons];
        m_core->LoadState(NULL, (int)m_saveStateSlot);
        [self clearAudio];
    }
}

- (BOOL)isLoaded
{
    return m_loaded;
}

- (BOOL)isPaused
{
    return !m_loaded || m_core->IsPaused();
}

- (BOOL)isMuted
{
    return m_muted;
}

- (void)setMuted:(BOOL)muted
{
    m_muted = muted;

    if (muted)
    {
        [self clearAudio];
    }
}

- (const uint16_t*)frameBuffer
{
    return m_frameBuffer;
}

- (NSInteger)frameWidth
{
    return m_frameWidth;
}

- (NSInteger)frameHeight
{
    return m_frameHeight;
}

- (NSInteger)frameWidthScale
{
    return m_frameWidthScale;
}

- (double)framesPerSecond
{
    return m_framesPerSecond;
}

- (void)configureAudio
{
    m_audioEngine = [[AVAudioEngine alloc] init];
    AVAudioFormat* format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:GG_AUDIO_SAMPLE_RATE channels:2];
    __weak GeargrafxEmulator* weakSelf = self;

    m_audioSourceNode = [[AVAudioSourceNode alloc] initWithFormat:format
                                                     renderBlock:^OSStatus(BOOL* isSilence,
                                                                         const AudioTimeStamp* timestamp,
                                                                         AVAudioFrameCount frameCount,
                                                                         AudioBufferList* outputData)
    {
        UNUSED(timestamp);
        GeargrafxEmulator* strongSelf = weakSelf;

        if (!strongSelf)
        {
            *isSilence = YES;

            for (UInt32 bufferIndex = 0; bufferIndex < outputData->mNumberBuffers; ++bufferIndex)
            {
                AudioBuffer* buffer = &outputData->mBuffers[bufferIndex];
                memset(buffer->mData, 0, buffer->mDataByteSize);
            }

            return noErr;
        }

        return [strongSelf renderAudioFrames:frameCount outputData:outputData silence:isSilence];
    }];

    [m_audioEngine attachNode:m_audioSourceNode];
    [m_audioEngine connect:m_audioSourceNode to:m_audioEngine.mainMixerNode format:format];
    [m_audioEngine prepare];

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(audioEngineConfigurationChanged:)
                                               name:AVAudioEngineConfigurationChangeNotification
                                             object:m_audioEngine];
}

- (void)startAudio
{
    if (m_audioEngine.isRunning)
    {
        return;
    }

    AVAudioSession* session = AVAudioSession.sharedInstance;
    NSError* error = nil;
    [session setCategory:AVAudioSessionCategoryAmbient
                    mode:AVAudioSessionModeDefault
                 options:AVAudioSessionCategoryOptionMixWithOthers
                   error:&error];

    if (!error)
    {
        NSError* preferenceError = nil;
        [session setPreferredSampleRate:GG_AUDIO_SAMPLE_RATE error:&preferenceError];
        preferenceError = nil;
        [session setPreferredIOBufferDuration:512.0 / GG_AUDIO_SAMPLE_RATE error:&preferenceError];
        [session setActive:YES error:&error];
    }

    [self clearAudio];

    if (!error)
    {
        [m_audioEngine startAndReturnError:&error];
    }

    if (error)
    {
        NSLog(@"Unable to start Geargrafx audio: %@", error.localizedDescription);
    }
}

- (void)stopAudio
{
    if (m_audioEngine.isRunning)
    {
        [m_audioEngine pause];
    }

    [self clearAudio];

    NSError* error = nil;
    [AVAudioSession.sharedInstance setActive:NO
                                 withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                       error:&error];

    if (error)
    {
        NSLog(@"Unable to stop Geargrafx audio: %@", error.localizedDescription);
    }
}

- (void)audioEngineConfigurationChanged:(NSNotification*)notification
{
    UNUSED(notification);
    [self clearAudio];

    __weak GeargrafxEmulator* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        GeargrafxEmulator* strongSelf = weakSelf;

        if (!strongSelf || !strongSelf->m_loaded ||
            strongSelf->m_core->IsPaused() || strongSelf->m_audioEngine.isRunning)
        {
            return;
        }

        [strongSelf->m_audioEngine prepare];

        NSError* error = nil;
        [strongSelf->m_audioEngine startAndReturnError:&error];

        if (error)
        {
            NSLog(@"Unable to restart Geargrafx audio: %@", error.localizedDescription);
        }
    });
}

- (void)clearAudio
{
    m_audioQueue.Reset();
}

- (void)enqueueAudioSamples:(const s16*)samples count:(int)count
{
    if (count > 0)
        m_audioQueue.Write(samples, (uint32_t)count);
}

- (OSStatus)renderAudioFrames:(AVAudioFrameCount)frameCount outputData:(AudioBufferList*)outputData silence:(BOOL*)isSilence
{
    bool audible = false;

    if (outputData->mNumberBuffers >= 2)
    {
        float* left = (float*)outputData->mBuffers[0].mData;
        float* right = (float*)outputData->mBuffers[1].mData;
        audible = m_audioQueue.Render(left, right, (uint32_t)frameCount);
    }
    else if (outputData->mNumberBuffers == 1)
    {
        float* output = (float*)outputData->mBuffers[0].mData;
        audible = m_audioQueue.RenderInterleaved(output, (uint32_t)frameCount);
    }

    *isSilence = !audible;
    return noErr;
}

@end
