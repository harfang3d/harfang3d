// GS Framework - Copyright 2001-2015 Emmanuel Julien. All Rights Reserved.

#import <Cocoa/Cocoa.h>

char cocoa_keyboard[256];
int cocoa_keyboard_flags;

void cocoaInit()
{
	NSLog(@"Cocoa init\n");

	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	NSLog(@"Cocoa is multithreaded: %d", [NSThread isMultiThreaded]);

	for (int i = 0; i < 256; ++i)
		cocoa_keyboard[i] = 0;
	cocoa_keyboard_flags = 0;
};

NSWindow *cocoaNewWindow(int width, int height, int bpp, int visibility)
{
	NSRect frame = NSMakeRect(0, 0, width, height);
	NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
				styleMask:NSTitledWindowMask | NSResizableWindowMask
				backing:NSBackingStoreBuffered
				defer:NO];

	[window setBackgroundColor:[NSColor blueColor]];

	[window makeKeyAndOrderFront:nil];
	[NSApp activateIgnoringOtherApps:YES];

	NSLog(@"cocoaNewWindow() -> %d", window);
	return window;
}

void ProcessPastEvents()
{
	while (1) {
		NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
			untilDate:[NSDate distantPast]
			inMode:NSDefaultRunLoopMode
			dequeue:YES];

		if (event == nil)
			break;

		switch (event.type) {
			case NSKeyDown:
				if (event.keyCode < 256)
					cocoa_keyboard[event.keyCode] = 1;
				break;
			case NSKeyUp:
				if (event.keyCode < 256)
					cocoa_keyboard[event.keyCode] = 0;
				break;
			case NSFlagsChanged:
				cocoa_keyboard_flags = [event modifierFlags];
				break;

			case NSMouseMoved:
				break;
			case NSLeftMouseDown:
				break;
			case NSLeftMouseUp:
				break;
			case NSRightMouseDown:
				break;
			case NSRightMouseUp:
				break;
			case NSScrollWheel:
				break;
		}

		[NSApp sendEvent:event];
	}
}

void cocoaUpdateWindow(NSWindow *window) { ProcessPastEvents(); }

void cocoaSetWindowTitle(NSWindow *window, const char *title) { [window setTitle:[NSString stringWithUTF8String:title]]; }

void cocoaDestroyWindow(NSWindow *window)
{
	NSLog(@"cocoaDestroyWindow() -> %d", window);

	[window close];
	[window release];
}

NSView *cocoaGetWindowView(NSWindow *window) { return [window contentView]; }

void cocoaGetViewSize(NSView *view, int *w, int *h)
{
	*w = (int)view.frame.size.width;
	*h = (int)view.frame.size.height;
}

bool CocoaWindowHasFocus(NSWindow *window) { return [window isKeyWindow]; }

void cocoaSetViewSize(NSWindow *window, NSView *view, int w, int h)
{
	NSSize size;
	size.width = w;
	size.height = h + window.frame.size.height - view.frame.size.height;

	NSRect frame = [window frame];
	frame.size = size;
	[window setFrame:frame display:YES animate:NO];
}

int *cocoaGetMonitors(int *count)
{
	NSMutableArray *rects = [NSMutableArray array];
	NSArray *screenArray = [NSScreen screens];
	unsigned screenCount = [screenArray count];
	unsigned index = 0;

	for (index; index < screenCount; index++) {
		NSScreen *screen = [screenArray objectAtIndex:index];
		NSRect screenRect = [screen visibleFrame];
		[rects addObject:[NSNumber numberWithInt:screenRect.origin.x]];
		[rects addObject:[NSNumber numberWithInt:screenRect.origin.y]];
		[rects addObject:[NSNumber numberWithInt:screenRect.size.width]];
		[rects addObject:[NSNumber numberWithInt:screenRect.size.height]];
	}

	*count = [rects count];
	int *cArray = malloc(sizeof(int) * *count);

	for (int i = 0; i < *count; ++i) {
		cArray[i] = [[rects objectAtIndex:i] intValue];
	}

	return cArray;
}

void cocoaDeleteArrayMonitors(int *array) { free(array); }

void cocoaGetWindowPos(NSWindow *window, int *x, int *y)
{
	*x = (int)window.frame.origin.x;
	*y = (int)window.frame.origin.y + window.frame.size.height;
}

void cocoaSetWindowPos(NSWindow *window, int x, int y)
{
	NSPoint pos;
	pos.x = x;
	pos.y = y;
	[window setFrameTopLeftPoint:pos];
}