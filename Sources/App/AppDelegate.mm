#import "AppDelegate.h"

#import "GameView.h"

#include <cmath>

@implementation SIAppDelegate {
    NSWindow* _window;
    SIGameView* _gameView;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    [self buildMainMenu];
    [self createWindow];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    (void)sender;
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    [_gameView saveHighScore];
}

- (void)windowWillClose:(NSNotification*)notification {
    (void)notification;
    [_gameView saveHighScore];
    [_gameView shutdown];
}

- (void)createWindow {
    const CGFloat designWidth = 224.0;
    const CGFloat designHeight = 256.0;
    const NSSize initialSize = NSMakeSize(760.0, std::floor(760.0 * designHeight / designWidth));

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0.0, 0.0, initialSize.width, initialSize.height)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.title = @"SPACE INVADERS  —  METAL ARCADE";
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = NSColor.blackColor;
    window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    window.releasedWhenClosed = NO;
    window.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;
    window.contentMinSize = NSMakeSize(448.0, 512.0);
    window.contentAspectRatio = NSMakeSize(designWidth, designHeight);
    window.delegate = self;

    NSError* error = nil;
    SIGameView* gameView = [[SIGameView alloc]
        initWithFrame:NSMakeRect(0.0, 0.0, initialSize.width, initialSize.height)
                error:&error];
    if (gameView == nil) {
        NSAlert* alert = [[NSAlert alloc] init];
        alert.alertStyle = NSAlertStyleCritical;
        alert.messageText = @"SPACE INVADERS could not start";
        NSString* reason = error != nil ? error.localizedDescription : @"unknown error";
        alert.informativeText =
            [NSString stringWithFormat:@"Metal initialization failed: %@", reason];
        [alert addButtonWithTitle:@"Quit"];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    _window = window;
    _gameView = gameView;
    window.contentView = gameView;
    [window center];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:gameView];
}

- (void)buildMainMenu {
    NSMenu* mainMenu = [[NSMenu alloc] init];

    NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:appMenuItem];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"SPACE INVADERS"];
    appMenuItem.submenu = appMenu;
    [appMenu addItemWithTitle:@"About SPACE INVADERS"
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* hide = [appMenu addItemWithTitle:@"Hide SPACE INVADERS"
                                          action:@selector(hide:)
                                   keyEquivalent:@"h"];
    hide.target = NSApp;
    NSMenuItem* hideOthers = [appMenu addItemWithTitle:@"Hide Others"
                                                action:@selector(hideOtherApplications:)
                                         keyEquivalent:@"h"];
    hideOthers.target = NSApp;
    hideOthers.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
    NSMenuItem* showAll = [appMenu addItemWithTitle:@"Show All"
                                             action:@selector(unhideAllApplications:)
                                      keyEquivalent:@""];
    showAll.target = NSApp;
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit SPACE INVADERS"
                       action:@selector(terminate:)
                keyEquivalent:@"q"];

    NSMenuItem* gameMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:gameMenuItem];
    NSMenu* gameMenu = [[NSMenu alloc] initWithTitle:@"Game"];
    gameMenuItem.submenu = gameMenu;

    NSMenuItem* newGame = [gameMenu addItemWithTitle:@"New Game"
                                              action:@selector(newGame:)
                                       keyEquivalent:@"n"];
    newGame.target = self;
    NSMenuItem* pause = [gameMenu addItemWithTitle:@"Pause / Resume"
                                            action:@selector(togglePause:)
                                     keyEquivalent:@"p"];
    pause.target = self;
    NSMenuItem* mute = [gameMenu addItemWithTitle:@"Toggle Sound"
                                           action:@selector(toggleSound:)
                                    keyEquivalent:@"m"];
    mute.target = self;

    NSMenuItem* viewMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:viewMenuItem];
    NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    viewMenuItem.submenu = viewMenu;
    NSMenuItem* fullScreen = [viewMenu addItemWithTitle:@"Toggle Full Screen"
                                                 action:@selector(toggleFullScreen:)
                                          keyEquivalent:@"f"];
    fullScreen.target = self;
    fullScreen.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagControl;

    NSApp.mainMenu = mainMenu;
}

- (void)newGame:(id)sender {
    (void)sender;
    [_gameView newGame];
    [_window makeFirstResponder:_gameView];
}

- (void)togglePause:(id)sender {
    (void)sender;
    [_gameView togglePause];
    [_window makeFirstResponder:_gameView];
}

- (void)toggleSound:(id)sender {
    (void)sender;
    [_gameView toggleMute];
    [_window makeFirstResponder:_gameView];
}

- (void)toggleFullScreen:(id)sender {
    [_window toggleFullScreen:sender];
}

@end
