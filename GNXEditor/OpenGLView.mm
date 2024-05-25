//
//  OpenGLView.mm
//  GNXEditor
//
//  Created by zhouxuguang on 2023/4/10.
//

#import "OpenGLView.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

@interface OpenGLView()
{
    NSOpenGLPixelFormat *pixelFormat;
    NSOpenGLContext *openGLContext;
    
    GLint virtualScreen;
    BOOL enableMultisample;
    CVDisplayLinkRef displayLink;
}

@end

@implementation OpenGLView

- (NSOpenGLContext*)openGLContext
{
    return openGLContext;
}

- (NSOpenGLPixelFormat*)pixelFormat
{
    return pixelFormat;
}

- (void)lockFocus
{
    [super lockFocus];

    if ([[self openGLContext] view] != self)
    {
        // Unlike NSOpenGLView, NSView does not export a -prepareOpenGL method to override.
        // We need to call it explicitly.
        [self prepareOpenGL];

        [[self openGLContext] setView:self];
    }
}

- (id)initWithFrame:(NSRect)frame
{
    // Any Macintosh system configuration that includes more GPUs than displays will have both online and offline GPUs.
    // Online GPUs are those that are connected to a display while offline GPUs are those that have no such output
    // hardware attached. In these configurations you may wish to take advantage of the offline hardware, or to be able
    // to start rendering on this hardware should a display be connected at a future date without having to reconfigure
    // and reupload all of your OpenGL content.
    //
    // To enable the usage of offline renderers, add NSOpenGLPFAAllowOfflineRenderers when using NSOpenGL or
    // kCGLPFAAllowOfflineRenderers when using CGL to the attribute list that you use to create your pixel format.
    NSOpenGLPixelFormatAttribute attribs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAllowOfflineRenderers, // lets OpenGL know this context is offline renderer aware
        NSOpenGLPFAMultisample, 1,
        NSOpenGLPFASampleBuffers, 1,
        NSOpenGLPFASamples, 4,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core, // Core Profile is the future
        0
    };

    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    if(!pf)
    {
        NSLog(@"Failed to create pixel format.");
        return nil;
    }

    self = [super initWithFrame:frame];
    if (self)
    {
        openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];

        enableMultisample = YES;
    }

    //[self lockFocus];

    return self;
}

- (void)initGL
{
    [[self openGLContext] makeCurrentContext];

    // Synchronize buffer swaps with vertical refresh rate
    GLint one = 1;
    [[self openGLContext] setValues:&one forParameter:NSOpenGLCPSwapInterval];

    if(enableMultisample)
        glEnable(GL_MULTISAMPLE);
}

// When displays are reconfigured this callback will be called. You can take this opportunity to do further
// processing or pass the notification on to an object for further handling.
void MyDisplayReconfigurationCallBack(CGDirectDisplayID display,
                                      CGDisplayChangeSummaryFlags flags,
                                      void *__nullable userInfo)
{
    if (flags & kCGDisplaySetModeFlag)
    {
        // In this example we've passed 'self' for the userInfo pointer,
        // so we can cast it to an appropriate object type and forward the message onwards.
        [((__bridge OpenGLView *)userInfo) update];

        // Display has been reconfigured.
        // Adapt to any changes in capabilities
        // (such as max texture size, extensions and hardware capabilities such as the amount of VRAM).
    }
}

- (void)prepareOpenGL
{
    // Make the OpenGL context current and do some one-time initialization.
    [self initGL];

    // Create the CVDisplayLink for driving the rendering loop
    [self setupDisplayLink];

    // This is an NSView subclass, not an NSOpenGLView.
    // We need to register the following notifications to be able to detect renderer changes.

    // Add an observer to NSViewGlobalFrameDidChangeNotification, which is posted
    // whenever an NSView that has an attached NSSurface changes size or changes screens
    // (thus potentially changing graphics hardware drivers).
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(surfaceNeedsUpdate:)
                                                 name:NSViewGlobalFrameDidChangeNotification
                                               object:self];

    // Also register for changes to the display configuation using Quartz Display Services
    CGDisplayRegisterReconfigurationCallback(MyDisplayReconfigurationCallBack, (__bridge void*)self);
}

- (void)update
{
    // Call -[NSOpenGLContext update] to let it handle screen selection after resize/move.
    [[self openGLContext] update];

    // A virtual screen change is detected
    if(virtualScreen != [[self openGLContext] currentVirtualScreen])
    {
        // Find the current renderer and update the UI.
        //[self gpuChanged];

        // Add your additional handling here
        // Adapt to any changes in capabilities
        // (such as max texture size, extensions and hardware capabilities such as the amount of VRAM).
    }
}

- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime
{
    [self drawView];

    return kCVReturnSuccess;
}

// This is the renderer output callback function
static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    CVReturn result = [(__bridge OpenGLView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

-(void)setupDisplayLink
{
    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, (__bridge void*)self);

    // Set the display link for the current renderer
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

    // Activate the display link
    CVDisplayLinkStart(displayLink);
}

- (BOOL)isOpaque
{
    return YES;
}

- (void)drawView
{
    [[self openGLContext] makeCurrentContext];

    // We draw on a secondary thread through the display link
    // Add a mutex around to avoid the threads from accessing the context simultaneously
    CGLLockContext([[self openGLContext] CGLContextObj]);

    glViewport(0,0,[self bounds].size.width,[self bounds].size.height);

    glClearColor(0.675f,0.675f,0.675f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    //if (!renderer) //first time drawing
    {
        // Create a BoingRenderer object which handles the rendering of a Boing ball.
        //renderer = [[BoingRenderer alloc] init];

        // Delegate to the BoingRenderer object to create an Orthographic projection camera
        //[renderer makeOrthographicForWidth:self.bounds.size.width height:self.bounds.size.height];

        // Update the text fields with the initial renderer info
        //[self gpuChanged];
    }

    [[self openGLContext] flushBuffer];

    CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void)dealloc
{
    // Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been released
    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);

    // Remove the registrations
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:NSViewGlobalFrameDidChangeNotification
                                                  object:self];

    //CGDisplayRemoveReconfigurationCallback(MyDisplayReconfigurationCallBack, self);

    //[super dealloc];
}

@end
