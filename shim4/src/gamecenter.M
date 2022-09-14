#ifdef IOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#import <GameKit/GameKit.h>

#include "shim4/main.h"
#include "shim4/gamecenter.h"

#include "shim4/internal/gfx.h"

using namespace noo;

static int is_authenticated;
static bool reset_complete;
static volatile bool modalViewShowing;
static NSMutableDictionary *achievementsDictionary;
static GKLocalPlayer *localPlayer;

#ifdef IOS
@interface GameCenterDelegate : NSObject<GKGameCenterControllerDelegate>
{
}
- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController *)viewController;
@end

@implementation GameCenterDelegate
- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController *)viewController
{
    UIViewController *root = [[[UIApplication sharedApplication] keyWindow] rootViewController];
    [root dismissViewControllerAnimated:YES completion:nil];
    modalViewShowing = false;
}
@end
#else
@interface GameCenterVC : NSObject <GKAchievementViewControllerDelegate> {
}
- (void) showAchievements;
- (void)achievementViewControllerDidFinish:(GKAchievementViewController *)viewController;
@end

#define NOTIF_NAME @"GKDialogControllerWillDisappear"

@implementation GameCenterVC

- (void) showAchievements
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(shut_r_down:) name:NOTIF_NAME object:nil];

	GKAchievementViewController *achievements = [[GKAchievementViewController alloc] init];

	if (achievements != nil)
	{
		achievements.achievementDelegate = self;
		GKDialogController *sdc = [GKDialogController sharedDialogController];
		SDL_SysWMinfo info;
		SDL_GetWindowWMInfo(gfx::internal::gfx_context.window, &info);
		sdc.parentWindow = info.info.cocoa.window;
		[sdc presentViewController: achievements];
	}

	[achievements release];
}

- (void)shut_r_down:(NSNotification *)notification
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NOTIF_NAME object:nil];

	modalViewShowing = false;
}

- (void)achievementViewControllerDidFinish:(GKGameCenterViewController *)achievementViewController
{
	GKDialogController *sdc = [GKDialogController sharedDialogController];
	[sdc dismiss: achievementViewController];
	modalViewShowing = false;
}

@end
#endif

namespace noo {

namespace util {

void static_init_gamecenter()
{
	is_authenticated = -1;
	reset_complete = false;
	modalViewShowing = false;
}

static bool is_gamecenter_available()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	// Check for presence of GKLocalPlayer class.
	BOOL localPlayerClassAvailable = (NSClassFromString(@"GKLocalPlayer")) != nil;

	BOOL osVersionSupported = FALSE;

#ifdef IOS
	// The device must be running iOS 4.1 or later.
	NSString *reqSysVer = @"4.1";
	NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
	osVersionSupported = ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending);
#else
	OSErr err;
	SInt32 systemVersion;
	if ((err = Gestalt(gestaltSystemVersion, &systemVersion)) == noErr) {
		if (systemVersion >= 0x1080) {
			osVersionSupported = TRUE;
		}
	}
#endif

	[pool drain];

	return (localPlayerClassAvailable && osVersionSupported);
}

static void reportAchievementIdentifier(NSString* identifier, bool notification)
{
	if (!is_gamecenter_available() || !is_authenticated)
		return;

	if ([achievementsDictionary objectForKey:identifier] != nil) {
		return;
	}

	float percent = 100;

	GKAchievement *achievement = [[[GKAchievement alloc] initWithIdentifier: identifier] autorelease];
	if (achievement)
	{
		achievement.showsCompletionBanner = notification ? YES : NO;
		[achievementsDictionary setObject:achievement forKey:identifier];
		achievement.percentComplete = percent;
		[GKAchievement reportAchievements:@[achievement] withCompletionHandler:^(NSError *error)
		{
			if (error != nil)
			{
			}
		}];
	}
}

static void resetAchievements(void)
{
	if (!is_gamecenter_available() || !is_authenticated)
		return;

	// Clear all locally saved achievement objects.
	achievementsDictionary = [[NSMutableDictionary alloc] init];

	// Clear all progress saved on Game Center
	[GKAchievement resetAchievementsWithCompletionHandler:^(NSError *error)
	{
		if (error != nil) {
		// handle errors
	}
	reset_complete = true;
	}];
}

void init_gamecenter()
{
	if (!is_gamecenter_available()) {
		is_authenticated = 0;
		return;
	}

	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	localPlayer = [GKLocalPlayer localPlayer];

#ifdef IOS
	[localPlayer setAuthenticateHandler:^(UIViewController *viewController, NSError *error) {
		if (viewController != nil) {
			//al_iphone_set_statusbar_orientation(ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT);
			UIViewController *root = [[[UIApplication sharedApplication] keyWindow] rootViewController];
			[root presentViewController:viewController animated: YES completion:nil];
		}
		else if (localPlayer.isAuthenticated) {
			is_authenticated = true;
		}
		else {
			is_authenticated = false;
		}
	}];
#else
	[localPlayer authenticateWithCompletionHandler:^(NSError *error) {
		if (localPlayer.isAuthenticated)
		{
			// Perform additional tasks for the authenticated player.
			is_authenticated = 1;
		}
		else {
			printf("Game Center authentication error: code %ld\n", (long)[error code]);
			is_authenticated = 0;
		}
	}];
#endif

#if 0
/*
	// FIXME:
	while (is_authenticated == false) {
		SDL_PumpEvents();
		// wait...
	}
	resetAchievements();
*/
#endif

	[pool drain];
}

bool achieve_gamecenter(char *id)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *str = [NSString stringWithUTF8String:id];

	reportAchievementIdentifier(str, true);

	[pool release];

	return true;
}

bool show_achievements_gamecenter()
{
	// ***** NOTE *****
	// don't use autorelease pools here, causes a crash (double free)

	modalViewShowing = true;
#ifdef IOS
	GKGameCenterViewController *vc = [[GKGameCenterViewController alloc] init];
#ifndef TVOS
	vc.viewState = GKGameCenterViewControllerStateAchievements;
#endif
	vc.gameCenterDelegate = [[GameCenterDelegate alloc] init];
	UIViewController *root = [[[UIApplication sharedApplication] keyWindow] rootViewController];
	[root presentViewController:vc animated:YES completion:nil];

	while (modalViewShowing) {
		SDL_PumpEvents();
		SDL_Delay(10);
	}

	/*
	SDL_SysWMinfo info;
	SDL_GetWindowWMInfo(gfx::internal::gfx_context.window, &info);

	UIView *view = info.info.uikit.window.rootViewController.view;
	[info.info.uikit.window bringSubviewToFront:view];
	*/
#else
	// FIXME!

	/*
	   ScreenDescriptor *sd = config.getWantedGraphicsMode();
	   bool was_fullscreen = sd->fullscreen;
	   if (sd->fullscreen) {
	   toggle_fullscreen();
	   set_target_backbuffer();
	   m_clear(black);
	   m_flip_display();
	   }
	 */

	modalViewShowing = true;
	GameCenterVC *vc = [[GameCenterVC alloc] init];
	[vc performSelectorOnMainThread: @selector(showAchievements) withObject:nil waitUntilDone:FALSE];
	while (modalViewShowing) {
		SDL_PumpEvents();
		SDL_Delay(10);
	}

	/*
	   SDL_Delay(1000);

	   if (was_fullscreen) {
	   toggle_fullscreen();
	   }
	 */
#endif

	return true;
}

} // End namespace util

} // End namespace noo
