//
//  ViewController.h
//  platform_ios
//
//  Created by Jimmy Horng on 12/6/28.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "KeychainItemWrapper.h"

@interface ViewController : UIViewController
{
}

@property (nonatomic, retain) KeychainItemWrapper *confKeychainItemWrapper;
@property (nonatomic, retain) KeychainItemWrapper *crdUser;
@property (nonatomic, retain) KeychainItemWrapper *crdIAS;

@property (nonatomic, retain) KeychainItemWrapper *ansSessionKey;
@property (nonatomic, retain) KeychainItemWrapper *ansLoginBlob;

@property (nonatomic, retain) KeychainItemWrapper *devCrdId;
@property (nonatomic, retain) KeychainItemWrapper *devCrdClear;
@property (nonatomic, retain) KeychainItemWrapper *devCrdSecret;
@property (nonatomic, retain) KeychainItemWrapper *devCrdToken;
@property (nonatomic, retain) KeychainItemWrapper *devIsLinked;

@property (nonatomic, retain) KeychainItemWrapper *userCred;
@property (nonatomic, retain) KeychainItemWrapper *devCred;

@property (nonatomic, retain) IBOutlet UITextField *domainTextField;
@property (nonatomic, retain) IBOutlet UITextField *userGroupTextField;
@property (nonatomic, retain) IBOutlet UIButton *resetButton;

- (IBAction)saveAction:(id)sender;
- (IBAction)resetAction:(id)sender;
- (IBAction)resetCredAction:(id)sender;

@end
