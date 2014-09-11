//
//  ViewController.m
//  OGLTest3
//
//  Created by admin on 2/8/13.
//  Copyright (c) 2013 Shogun3D. All rights reserved.
//

#import "ViewController.h"
#import "GLView.h"

void __gluMakeIdentityf(GLfloat m[16])
{
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
    GLfloat m[4][4];
    GLfloat sine, cotangent, deltaZ;
    GLfloat radians = fovy / 2 * 3.14 / 180;
    
    deltaZ = zFar;
    deltaZ -= zNear;

    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
    {
        return;
    }
    cotangent = cos(radians) / sine;
    
    __gluMakeIdentityf(&m[0][0]);
    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;
    glMultMatrixf(&m[0][0]);
}

@interface ViewController ()

@end

@implementation ViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
    
    CGRect rect = [[UIScreen mainScreen] bounds];
    
    // 1. Create an Xcode Project
    
    // 2. Create a Context
    EAGLContext *context = [[[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1] autorelease];
    [EAGLContext setCurrentContext:context];
    
    // 3. Create a View
    GLView *glView = [[[GLView alloc] initWithFrame:CGRectMake(0, 0, 320, rect.size.height)] autorelease];
    [self.view addSubview:glView];
   
    // 4. Create a Renderbuffer
    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)glView.layer];
    
    // 5. Create a Framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, 320, rect.size.height );
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
    
    GLuint depthBuffer;
    glGenRenderbuffers( 1, &depthBuffer );
    glBindRenderbuffer( GL_RENDERBUFFER, depthBuffer );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 320, rect.size.height );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
    
    glBindRenderbuffer( GL_RENDERBUFFER, renderbuffer );
    
    GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if( status != GL_FRAMEBUFFER_COMPLETE )
    {
        NSLog( @"Error completing FBO! %x", status );
    }
    
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
//    glClearDepthf( 1.0f );
    
    glDisable( GL_CULL_FACE );
    glDisable( GL_LIGHT0 );
    
    glViewport( 0, 0, 320, rect.size.height );
    /*glMatrixMode( GL_PROJECTION );
    gluPerspective( 45.0f, 320.0f/480.0f, 0.1f, 1000.0f );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();*/
    
    // Set up OpenGL projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0, rect.size.width, rect.size.height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND_SRC);
    glEnableClientState(GL_VERTEX_ARRAY);
//    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

@end
