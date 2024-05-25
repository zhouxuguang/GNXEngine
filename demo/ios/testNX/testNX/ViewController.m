//
//  ViewController.m
//  testNX
//
//  Created by zhouxuguang on 2021/5/3.
//

#import "ViewController.h"
#import "EAGLView.h"
#import "MetalView.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
//    EAGLView* glView = [[EAGLView alloc] initWithFrame:self.view.frame];
//    [self.view addSubview:glView];
    MetalView* metalView = [[MetalView alloc] initWithFrame:self.view.frame];
    [self.view addSubview:metalView];
    
}


@end
