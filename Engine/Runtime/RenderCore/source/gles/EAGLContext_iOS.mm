//
//  EAGLContext_iOS.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/25.
//

#import "EAGLContext_iOS.h"
#include "gl3stub.h"
#include "BaseLib/BaseLib.h"

NAMESPACE_RENDERCORE_BEGIN

GLContextImplPtr createEAGLContext(void *viewHandle)
{
    return std::make_shared<EAGLContext_iOS>(viewHandle);
}

EAGLContext_iOS::EAGLContext_iOS(void *viewHandle) : GLContextImpl(viewHandle)
{
    if (m_GLContext == nil)
    {
        m_GLContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        
        if (m_GLContext == nil)
        {
            m_GLContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
            m_isGLES3 = false;
        }
    }
    
    assert(m_GLContext != nil);
    //创建第二个共享的上下文
    if (m_GLContext)
    {
        EAGLContext* secondContext = [[EAGLContext alloc] initWithAPI:[m_GLContext API] sharegroup: [m_GLContext sharegroup]];
        
//        NSInvocationOperation *operation = [[NSInvocationOperation alloc]initWithTarget:this
//                                                                                   selector:@selector(downloadImage:)
//                                                                                     object:kURL];
//        NSOperationQueue *queue = [[NSOperationQueue alloc]init];
        
        [EAGLContext setCurrentContext:secondContext];
        
        dispatch_queue_t serialDiapatchQueue = dispatch_queue_create("com.gnxengine.gles.resource", DISPATCH_QUEUE_SERIAL);
        
        static const char *queueKey1 = "queueKey1";
        dispatch_queue_set_specific(serialDiapatchQueue, queueKey1, &queueKey1, nullptr);
//        dispatch_queue_t dispatchgetglobalqueue=dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
//        dispatch_set_target_queue(serialDiapatchQueue, dispatchgetglobalqueue);
        dispatch_async(serialDiapatchQueue, ^{
            NSLog(@"我优先级低，先让让");
            
            if (dispatch_queue_get_specific(serialDiapatchQueue, queueKey1) == &queueKey1)
            {
                printf("");
            }
            
            EAGLContext* context = [EAGLContext currentContext];
            [EAGLContext setCurrentContext:secondContext];
            context = [EAGLContext currentContext];
            
            const char* str =(const char*) glGetString(GL_VERSION);
            
            NSLog(@"我优先级低，先让让");
        });
        
        dispatch_async(serialDiapatchQueue, ^{
            NSLog(@"我优先级低，先让让");
            
            EAGLContext* context = [EAGLContext currentContext];
//            [EAGLContext setCurrentContext:secondContext];
//            context = [EAGLContext currentContext];
            NSLog(@"我优先级低，先让让");
        });
        
        //使用线程池
        baselib::ThreadPool *pool = new baselib::ThreadPool(1);
        pool->Start();
        {
            class mywork : public baselib::TaskRunner
            {
                
            private:
                void Run()
                {
                    EAGLContext* context = [EAGLContext currentContext];
                    if (!context)
                    {
                        [EAGLContext setCurrentContext:m_context];
                        context = [EAGLContext currentContext];
                    }
                    
                    const char* str =(const char*) glGetString(GL_VERSION);
                    
                    NSLog(@"我优先级低，先让让");
                }
                
                EAGLContext* m_context;
                
            public:
                void setContext(EAGLContext* context)
                {
                    m_context = context;
                }
            };
            
            std::shared_ptr<mywork> task = std::make_shared<mywork>();
            task->setContext(secondContext);
            pool->Execute(task);
            
            std::shared_ptr<mywork> task1 = std::make_shared<mywork>();
            task1->setContext(secondContext);
            pool->Execute(task1);
        }
    }
    
    m_EAGLLayer = (__bridge CAEAGLLayer *)viewHandle;
    assert(m_EAGLLayer != nil);
    //m_EAGLLayer.contentsFormat = kEAGLColorFormatSRGBA8;
    //[m_EAGLLayer.drawableProperties setValue:kEAGLColorFormatSRGBA8 forKey:kEAGLDrawablePropertyColorFormat];
    
    deleteFramebuffer();
    createFramebuffer();
}

void EAGLContext_iOS::createFramebuffer()
{
    if (m_GLContext && !m_defaultFramebuffer)
    {
        [EAGLContext setCurrentContext:m_GLContext];
        
//        GLboolean sRGB = GL_FALSE;
//        glGetBooleanv(EGL_FRAMEBUFFER_SRG, &sRGB);
//        if (sRGB)
//        {
//            glEnable(GL_FRAMEBUFFER_SRGB_EXT);
//        }
//        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
        
        glGenFramebuffers(1, &m_defaultFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFramebuffer);

        glGenRenderbuffers(1, &m_colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
        
        //renderbufferStorage 必须在主线程调用，画面才能显示出来，这个需要研究
        __block BOOL bResult;
        dispatch_sync(dispatch_get_main_queue(), ^{
            bResult = [m_GLContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:m_EAGLLayer];
        });
//        BOOL _bResult = [m_GLContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:m_EAGLLayer];
//        assert(_bResult = YES);
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRenderbuffer);
        
        GLint bufferWidth;
        GLint bufferHeight;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &bufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &bufferHeight);
        m_framebufferWidth = bufferWidth;
        m_framebufferHeight = bufferHeight;

        
        // The following is MSAA settings
        glGenFramebuffers(1, &m_msaaFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFramebuffer);
        
        glGenRenderbuffers(1, &m_msaaRenderbuffer);
        glGenRenderbuffers(1, &m_msaaDepthbuffer);
        
        if (m_isGLES3)
        {
            // 2 samples for color
            glBindRenderbuffer(GL_RENDERBUFFER, m_msaaRenderbuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_RGBA8, bufferWidth, bufferHeight);
            //glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 2, GL_RGBA8_OES, bufferWidth, bufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaaRenderbuffer);
            
            // 3 samples for depth
            glBindRenderbuffer(GL_RENDERBUFFER, m_msaaDepthbuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_DEPTH24_STENCIL8, bufferWidth, bufferHeight);
            //glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 2, GL_DEPTH24_STENCIL8_OES, bufferWidth, bufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthbuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthbuffer);
        }
        else
        {
            // 2 samples for color
            glBindRenderbuffer(GL_RENDERBUFFER, m_msaaRenderbuffer);
            glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 2, GL_RGBA8_OES, bufferWidth, bufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaaRenderbuffer);
            
            // 3 samples for depth
            glBindRenderbuffer(GL_RENDERBUFFER, m_msaaDepthbuffer);
            glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 2, GL_DEPTH24_STENCIL8_OES, bufferWidth, bufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthbuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthbuffer);
        }
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
            assert(false);
        }
    
    }
}

void EAGLContext_iOS::deleteFramebuffer()
{
    if (m_GLContext)
    {
        [EAGLContext setCurrentContext:m_GLContext];
        
        if (m_defaultFramebuffer)
        {
            glDeleteFramebuffers(1, &m_defaultFramebuffer);
            m_defaultFramebuffer = 0;
        }
        
        if (m_colorRenderbuffer)
        {
            glDeleteRenderbuffers(1, &m_colorRenderbuffer);
            m_colorRenderbuffer = 0;
        }
        
        if(m_depthRenderbuffer)
        {
            glDeleteRenderbuffers(1, &m_depthRenderbuffer);
            m_depthRenderbuffer = 0;
        }
        
        if (m_msaaFramebuffer)
        {
            glDeleteFramebuffers(1, &m_msaaFramebuffer);
            m_msaaFramebuffer = 0;
        }
        
        if (m_msaaRenderbuffer)
        {
            glDeleteRenderbuffers(1, &m_msaaRenderbuffer);
            m_msaaRenderbuffer = 0;
        }
        
        if (m_msaaDepthbuffer)
        {
            glDeleteRenderbuffers(1, &m_msaaDepthbuffer);
            m_msaaDepthbuffer = 0;
        }
    }
}

void EAGLContext_iOS::setFramebuffer()
{
    if (m_GLContext)
    {
        [EAGLContext setCurrentContext:m_GLContext];

        if (!m_defaultFramebuffer)
        {
            createFramebuffer();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msaaRenderbuffer);

        //glViewport(0, 0, (GLint)framebufferWidth, (GLint)framebufferHeight);
    }
}

bool EAGLContext_iOS::presentFramebuffer()
{
    BOOL success = FALSE;

    if (m_GLContext)
    {
        [EAGLContext setCurrentContext:m_GLContext];
        
        // Discard the depth buffer and stencil buffer from the read fbo. It is no more necessary.
        glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 2, (GLenum[]){GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT});
        
        if (m_isGLES3)
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_defaultFramebuffer);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFramebuffer);

            glBlitFramebuffer(0, 0, m_framebufferWidth, m_framebufferHeight, 0, 0, m_framebufferWidth, m_framebufferHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            //glResolveMultisampleFramebufferAPPLE();
        }
        else
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, m_defaultFramebuffer);
            glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, m_msaaFramebuffer);

            glResolveMultisampleFramebufferAPPLE();
        }

        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
        success = [m_GLContext presentRenderbuffer:GL_RENDERBUFFER];
    }
    return success;
}

void EAGLContext_iOS::setupDisplayLink()
{
    //m_displayLink = [CADisplayLink displayLinkWithTarget:this selector:@selector(updateTextColor)];
    //self.displayLink.paused = YES;
}


NAMESPACE_RENDERCORE_END
