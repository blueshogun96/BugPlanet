//
//  main.m
//  BugPlanet
//
//  Created by admin on 10/14/13.
//  Copyright (c) 2013 Shogun3D. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "AppDelegate.h"

int main(int argc, char *argv[])
{
    /* Cheaply do pre-initialization */
    extern void pre_init();
    pre_init();
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
