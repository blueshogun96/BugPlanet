//
//  GLView.m
//  OGLTest3
//
//  Created by admin on 2/8/13.
//  Copyright (c) 2013 Shogun3D. All rights reserved.
//

#import "GLView.h"

extern int (*_init_func)();
extern void (*_display_func)(void);
extern void (*_touch_func)(int, int, int, int);
extern void (*_move_func)(int, int);
extern void (*_exit_func)();

@implementation GLView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialize rendering loop
        displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
        [displayLink setFrameInterval:1];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [self setMultipleTouchEnabled:YES];
        _init_func();
    }
    return self;
}

-(void) drawView:(id)sender
{
    _display_func();
    
    [[EAGLContext currentContext] presentRenderbuffer:GL_RENDERBUFFER];
}

#pragma mark -
#pragma mark Touch-handling methods

int touch_addr = 0;
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSMutableSet *currentTouches = [[[event touchesForView:self] mutableCopy] autorelease];
    [currentTouches minusSet:touches];
    int count = [[touches allObjects] count];
	
	// New touches are not yet included in the current touches for the view
	lastMovementPosition = [[touches anyObject] locationInView:self];
    _touch_func( count, 1, lastMovementPosition.x+160.0f, lastMovementPosition.y );
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
{
	CGPoint currentMovementPosition = [[touches anyObject] locationInView:self];
//	[renderer renderByRotatingAroundX:(lastMovementPosition.x - currentMovementPosition.x) rotatingAroundY:(lastMovementPosition.y - currentMovementPosition.y)];
	lastMovementPosition = currentMovementPosition;
    _move_func( lastMovementPosition.x+160.0f, lastMovementPosition.y );
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSMutableSet *remainingTouches = [[[event touchesForView:self] mutableCopy] autorelease];
    [remainingTouches minusSet:touches];
    
	lastMovementPosition = [[remainingTouches anyObject] locationInView:self];
    _touch_func( 0, 0, lastMovementPosition.x+160.0f, lastMovementPosition.y );
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	// Handle touches canceled the same as as a touches ended event
    [self touchesEnded:touches withEvent:event];
}


@end
