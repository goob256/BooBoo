#include "shim4/macosx.h"
#include "shim4/util.h"

using namespace noo;

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

namespace noo {

namespace gfx {

void macosx_centre_window(void *window)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *win = (NSWindow *)window;
	[win center];
	[pool release];
}

util::Size<int> macosx_get_desktop_resolution()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSRect r = [[[NSScreen screens] objectAtIndex:shim::adapter] frame];
	util::Size<int> size(r.size.width, r.size.height);
	[pool release];
	return size;
}

void macosx_set_background_colour(void *window, SDL_Colour colour)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *nswindow = (NSWindow *)window;

	[nswindow setBackgroundColor:[NSColor colorWithDeviceRed:(colour.r/255.0f) green:(colour.g/255.0f) blue:(colour.b/255.0f) alpha:(colour.a/255.0f)]];
	[pool release];
}

} // End namespace gfx

namespace gui {

int macosx_popup(std::string caption, std::string text, gui::Popup_Type type)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSAlert *alert = [[NSAlert alloc] init];

	if (type == gui::OK) {
		[alert addButtonWithTitle:@"OK"];
	}
	else {
		[alert addButtonWithTitle:@"Yes"];
		[alert addButtonWithTitle:@"No"];
	}
	[alert setMessageText:[NSString stringWithUTF8String:caption.c_str()]];
	[alert setInformativeText:[NSString stringWithUTF8String:text.c_str()]];
	[alert setAlertStyle:NSInformationalAlertStyle];

	NSInteger result = [alert runModal];

	[pool release];

	if (type == gui::YESNO) {
		if (result == NSAlertFirstButtonReturn) {
			return 0;
		}
		else {
			return 1;
		}
	}

	return 0;
}

} // End namespace gui

namespace util {

void macosx_log(const char *s)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *str = [NSString stringWithUTF8String:s];
	NSLog(@"%@", str);
	[pool release];
}

std::string macosx_get_standard_path(Path_Type type)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	std::string path;
	NSString *s = 0;

	if (type == DOCUMENTS) {
		s = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
	}
	else if (type == APPDATA) {
		s = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex:0];
	}
	else if (type == HOME) {
		s = NSHomeDirectory();
	}

	if (s) {
		path = [s UTF8String];
	}
	
	[pool release];

	return path;
}

void macosx_open_with_system(std::string filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *str = [NSString stringWithUTF8String:filename.c_str()];
	[[NSWorkspace sharedWorkspace] openFile:str];
	[pool release];
}

std::string macosx_get_system_language()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *str = [[NSLocale preferredLanguages] objectAtIndex:0];

	std::string result;
    
	if ([str hasPrefix:@"de"]) {
		result = "german";
	}
	else if ([str hasPrefix:@"fr"]) {
		result = "french";
	}
	else if ([str hasPrefix:@"nl"]) {
		result = "dutch";
	}
	else if ([str hasPrefix:@"el"]) {
		result = "greek";
	}
	else if ([str hasPrefix:@"it"]) {
		result = "italian";
	}
	else if ([str hasPrefix:@"pl"]) {
		result = "polish";
	}
	else if ([str hasPrefix:@"pt"]) {
		result = "portuguese";
	}
	else if ([str hasPrefix:@"ru"]) {
		result = "russian";
	}
	else if ([str hasPrefix:@"es"]) {
		result = "spanish";
	}
	else if ([str hasPrefix:@"ko"]) {
		result = "korean";
	}
	else {
		result = "english";
	}

	[pool release];

	return result;
}

} // End namespace util

} // End namespace noo
