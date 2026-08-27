#include "mac_dialogs.h"

#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>

std::string mac_show_open_panel() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setAllowedFileTypes:@[@"sks"]];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = panel.URLs.firstObject;
            return std::string([[url path] UTF8String]);
        }
        return "";
    }
}

std::string mac_show_save_panel() {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setCanCreateDirectories:YES];
        [panel setAllowedFileTypes:@[@"sks"]];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = panel.URL;
            return std::string([[url path] UTF8String]);
        }
        return "";
    }
}
#endif
