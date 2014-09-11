//
//  GLView.h
//  OGLTest3
//
//  Created by admin on 2/8/13.
//  Copyright (c) 2013 Shogun3D. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CADisplayLink.h>

@interface GLView : UIView
{
    CADisplayLink* displayLink;
    CGPoint lastMovementPosition;
}

-(void) drawView:(id)sender;

@end
