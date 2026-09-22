#import <Cocoa/Cocoa.h>
#include <FL/Enumerations.H>
#include "macos_about.hpp"

void ep128emuShowMacOSAboutPanel()
{
  @autoreleasepool {
    NSString *creditsText = [NSString stringWithFormat:
        @"macOS ARM64 build 2026.09\n\nGUI with FLTK %d.%d.%d",
        FL_MAJOR_VERSION, FL_MINOR_VERSION, FL_PATCH_VERSION];
    NSMutableParagraphStyle *paragraphStyle =
        [[[NSMutableParagraphStyle alloc] init] autorelease];
    [paragraphStyle setAlignment:NSTextAlignmentCenter];
    NSAttributedString *credits = [[NSAttributedString alloc]
        initWithString:creditsText
        attributes:[NSDictionary dictionaryWithObject:paragraphStyle
                                                   forKey:NSParagraphStyleAttributeName]];
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
        credits, @"Credits",
        @"2.0.11.2", @"ApplicationVersion",
        nil];
    [NSApp orderFrontStandardAboutPanelWithOptions:options];
#if !__has_feature(objc_arc)
    [credits release];
#endif
  }
}
