// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#import <Foundation/Foundation.h>

#import <UIKit/UIKit.h>

#import "Common/Config/Config.h"

#define GSUSetInitialForBool(x, i, y, z) [GraphicsSettingsUtils SetInitialForBoolSetting:x isInverted:i forSwitch:y label:z]
#define GSUSetInitialForTransitionCell(x, y) [GraphicsSettingsUtils SetInitialForTransitionCell:x forLabel:y]
#define GSUSetInitialForTransitionCellU32(x, y) [GraphicsSettingsUtils SetInitialForTransitionCellU32:x forLabel:y]
#define GSUShowHelpWithString(x) [GraphicsSettingsUtils ShowInfoAlertForLocalizable:x onController:self]

NS_ASSUME_NONNULL_BEGIN

@interface GraphicsSettingsUtils : NSObject

+ (void)SetInitialForBoolSetting:(const Config::ConfigInfo<bool>&)setting isInverted:(bool)inverted forSwitch:(UISwitch*)sw label:(UILabel*)label;
+ (void)SetInitialForTransitionCell:(const Config::ConfigInfo<int>&)setting forLabel:(UILabel*)label;
+ (void)SetInitialForTransitionCellU32:(const Config::ConfigInfo<u32>&)setting forLabel:(UILabel*)label;
+ (void)ShowInfoAlertForLocalizable:(NSString*)localizable onController:(UIViewController*)target_controller;

@end

NS_ASSUME_NONNULL_END
