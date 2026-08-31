/*
 * Geargrafx - PC Engine / TurboGrafx-16 Emulator
 * Copyright (C) 2024 Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, GeargrafxButton)
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

@interface GeargrafxEmulator : NSObject

@property (nonatomic, readonly, getter=isLoaded) BOOL loaded;
@property (nonatomic, readonly, getter=isPaused) BOOL paused;
@property (nonatomic, getter=isMuted) BOOL muted;
@property (nonatomic, readonly) const uint16_t* frameBuffer;
@property (nonatomic, readonly) NSInteger frameWidth;
@property (nonatomic, readonly) NSInteger frameHeight;
@property (nonatomic, readonly) NSInteger frameWidthScale;
@property (nonatomic, readonly) double framesPerSecond;

+ (nullable NSString*)romCRCInArchiveAtURL:(NSURL*)url NS_SWIFT_NAME(romCRC(inArchiveAt:));
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
            huc6280AEnabled:(BOOL)huc6280AEnabled
                  psgVolume:(NSInteger)psgVolume
                cdromVolume:(NSInteger)cdromVolume
                adpcmVolume:(NSInteger)adpcmVolume
                  cdromType:(NSInteger)cdromType
                  cdromBIOS:(NSInteger)cdromBIOS
               preloadCDROM:(BOOL)preloadCDROM
              saveStateSlot:(NSInteger)saveStateSlot
          firmwareDirectory:(NSURL*)firmwareDirectory
    NS_SWIFT_NAME(configure(console:palette:overscan:scanlineMode:scanlineStart:scanlineEnd:noSpriteLimit:safeVDCDefaults:lowpassFilter:lowpassIntensity:lowpassCutoff:controllerType:avenuePad3MainButton:softResetEnabled:turboIEnabled:turboISpeed:turboIIEnabled:turboIISpeed:huc6280AEnabled:psgVolume:cdromVolume:adpcmVolume:cdromType:cdromBIOS:preloadCDROM:saveStateSlot:firmwareDirectory:));
- (BOOL)loadROMAtURL:(NSURL*)url error:(NSError* _Nullable* _Nullable)error NS_SWIFT_NAME(loadROM(at:));
- (void)runFrame;
- (void)setButton:(GeargrafxButton)button pressed:(BOOL)pressed;
- (void)releaseAllButtons;
- (void)pause;
- (void)resume;
- (void)reset;
- (void)saveRAM;
- (void)saveState;
- (void)loadState;
- (void)startAudio;
- (void)stopAudio;

@end

NS_ASSUME_NONNULL_END
