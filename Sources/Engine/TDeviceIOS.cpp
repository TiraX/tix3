/*
 TiX Engine v2.0 Copyright (C) 2018
 By ZhaoShuai tirax.cn@gmail.com
 */

#include "stdafx.h"

#include "TDeviceIOS.h"
#import "TViewController.h"
#import "FMetalView.h"
#import <sys/utsname.h>

#ifdef TI_PLATFORM_IOS

namespace tix
{
	TDeviceIOS::TDeviceIOS(int32 w, int32 h)
		: TDevice(w, h)
		, Window(nil)
		, ViewController(nil)
	{
		Create();
		
		// change dir to app base dir
		NSString* path = [[NSBundle mainBundle] resourcePath];
		path = [path stringByAppendingString: @"/iOS/"];
		chdir([path UTF8String]);

		AbsolutePath = [path UTF8String];
		if (AbsolutePath.at(AbsolutePath.size() - 1) != '/')
		{
			// Make path format correct.
			TStringReplace(AbsolutePath, "\\", "/");
			if (AbsolutePath.at(AbsolutePath.size() - 1) != '/')
			{
				AbsolutePath += '/';
			}
		}

		
		FindDeviceType();
	}
	
	TDeviceIOS::~TDeviceIOS()
	{
		Window = nil;
	}
	
	void TDeviceIOS::Resize(int32 w, int32 h)
	{
		TDevice::Resize(w, h);
	}
	
	//! runs the device. Returns false if device wants to be deleted
	bool TDeviceIOS::Run()
	{
		return true;
	}
	
	void TDeviceIOS::FindDeviceType()
	{
		//struct utsname systemInfo;
		//uname(&systemInfo);
		
		//NSString* device = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
	}
	
	int32 TDeviceIOS::GetPreferredLanguage()
	{
		//float ios_version = [[UIDevice currentDevice].systemVersion floatValue];
		//NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
		//NSArray* languages = [NSLocale preferredLanguages];//[defs objectForKey:@"AppleLanguages"];
		//NSString* preferredLang = [languages objectAtIndex:0];
		
		//return TI_LANG_DEFAULT;
		return 0;
	}
	
	void TDeviceIOS::Create()
	{
		FMetalView * _View = nil;
		Window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, Width, Height)];
		// Override point for customization after application launch.
		_View = [FMetalView viewWithFrame: [Window bounds]];
		
		ViewController = [[TViewController alloc] initWithNibName:nil bundle:nil];
		ViewController.edgesForExtendedLayout = UIRectEdgeNone;
		ViewController.view = _View;
		
		// Set RootViewController to window
		[Window setRootViewController:ViewController];
		[_View setMultipleTouchEnabled:YES];
		[Window makeKeyAndVisible];
	}
}

#endif //TI_PLATFORM_IOS
