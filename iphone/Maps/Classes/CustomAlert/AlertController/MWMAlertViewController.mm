//
//  MWMAlertViewController.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlertViewController.h"
#import "MWMDownloadTransitMapAlert.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController () <UIGestureRecognizerDelegate>
@property (nonatomic, weak, readwrite) UIViewController *ownerViewController;
@property (nonatomic, weak) IBOutlet UITapGestureRecognizer *tap;
@end

@implementation MWMAlertViewController

- (instancetype)initWithViewController:(UIViewController *)viewController {
  self = [super initWithNibName:kAlertControllerNibIdentifier bundle:nil];
  if (self) {
    self.ownerViewController = viewController;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.frame = UIApplication.sharedApplication.keyWindow.bounds;
  
  // Need only for iOS 5.
  if ([[[UIDevice currentDevice] systemVersion] integerValue] < 6) {
    self.tap.delegate = self;
  }
}

#pragma mark - Actions

- (void)presentDownloaderAlertWithCountrieIndex:(const storage::TIndex&)index {
  MWMAlert *alert = [MWMAlert downloaderAlertWithCountrieIndex:index];
  [self displayAlert:alert];
}

- (void)presentAlert:(routing::IRouter::ResultCode)type {
  MWMAlert *alert = [MWMAlert alert:type];
  [self displayAlert:alert];
}

- (void)displayAlert:(MWMAlert *)alert {
  alert.alertController = self;
  [self.ownerViewController addChildViewController:self];
  self.view.center = self.ownerViewController.view.center;
  [self.ownerViewController.view addSubview:self.view];
  [self.view addSubview:alert];
  alert.center = self.view.center;
}

- (void)closeAlert {
  self.ownerViewController.view.userInteractionEnabled = YES;
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}

#pragma mark - Gesture intercepter

- (IBAction)backgroundTap:(UITapGestureRecognizer *)sender {
  return;
}

- (IBAction)backgroundSwipe:(UISwipeGestureRecognizer *)sender {
  return;
}

- (IBAction)backgroundPinch:(id)sender {
  return;
}

- (IBAction)backgroundPan:(id)sender {
  return;
}

#pragma mark - Gesture Recognizer Delegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch {
  if ([touch.view isKindOfClass:[UIControl class]]) {
    [(UIControl *)touch.view sendActionsForControlEvents:UIControlEventTouchUpInside];
  }
  return YES;
}

@end
