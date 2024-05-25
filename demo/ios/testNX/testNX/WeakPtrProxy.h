

#import <Foundation/Foundation.h>

@interface WeakPtrProxy : NSProxy

+ (instancetype)proxyWithTarget:(id)target;
@property (weak, nonatomic) id target;

@end
