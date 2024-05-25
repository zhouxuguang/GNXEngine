

#import "WeakPtrProxy.h"

@implementation WeakPtrProxy

+ (instancetype)proxyWithTarget:(id)target
{
    // NSProxy对象不需要调用init，因为它本来就没有init方法
    WeakPtrProxy *proxy = [WeakPtrProxy alloc];
    proxy.target = target;
    return proxy;
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)sel
{
    return [self.target methodSignatureForSelector:sel];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    [invocation invokeWithTarget:self.target];
}
@end
