//
//  ViewController.m
//  platform_ios
//
//  Created by Jimmy Horng on 12/6/28.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#import "ViewController.h"
#import "SystemInfo.h"

@interface ViewController ()

@end

@implementation ViewController

@synthesize confKeychainItemWrapper=_confKeychainItemWrapper;
@synthesize domainTextField=_domainTextField;
@synthesize userGroupTextField=_userGroupTextField;
@synthesize resetButton=_resetButton;
@synthesize crdUser, crdIAS, ansSessionKey, ansLoginBlob, devCrdId, devCrdClear, devCrdSecret, devCrdToken, devIsLinked;
@synthesize userCred, devCred;

NSString *actoolGroup;
NSString *credentialsGroup;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    actoolGroup = [NSString stringWithFormat:@"%@.actool", [SystemInfo bundleSeedID]];
    credentialsGroup = [NSString stringWithFormat:@"%@.credentials", [SystemInfo bundleSeedID]];
    
    self.confKeychainItemWrapper = [[KeychainItemWrapper alloc] initWithIdentifier:@"ccdconf" accessGroup:actoolGroup];
    
    NSData *ccdconfData = (NSData *)[self.confKeychainItemWrapper objectForKey:(__bridge id)kSecAttrService];
    
    if (!([ccdconfData length] > 0)) {
        [self.resetButton setEnabled:NO];
        [self.resetButton setHidden:YES];
    } else {
        NSArray *confLines = [[[NSString alloc] initWithData:ccdconfData encoding:NSASCIIStringEncoding] componentsSeparatedByString:@"\n"];
        for (NSString *line in confLines) {
            if ([line rangeOfString:@"infraDomain"].location != NSNotFound)
            {
                NSArray *domainKeyValue = [line componentsSeparatedByString:@" = "];
                NSString *domainString = [domainKeyValue objectAtIndex:1];
                if (![domainString isEqual: @"${DOMAIN}"]) {
                    [self.domainTextField setText:domainString];
                }
            }
            if ([line rangeOfString:@"userGroup"].location != NSNotFound)
            {
                NSArray *userGroupKeyValue = [line componentsSeparatedByString:@" = "];
                NSString *userGroupString = [userGroupKeyValue objectAtIndex:1];
                if (![userGroupString isEqual: @"${GROUP}"]) {
                    [self.userGroupTextField setText:userGroupString];
                }
            }
        }
        
    }
    NSLog(@"--------%@",[[NSString alloc] initWithData:ccdconfData encoding:NSASCIIStringEncoding]);
    
    //--------- Log credentials
    // User
    self.crdUser = [[KeychainItemWrapper alloc] initWithIdentifier:@"username" accessGroup:credentialsGroup];
    NSString *stringData = (NSString *)[self.crdUser objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====user_name===%@", stringData);
    
    self.crdIAS = [[KeychainItemWrapper alloc] initWithIdentifier:@"user_iasoutput" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.crdIAS objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====user_iasoutput===%@", stringData);
    
    //ANS
    self.ansSessionKey = [[KeychainItemWrapper alloc] initWithIdentifier:@"ans_session_key" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.ansSessionKey objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====ans_session_key===%@", stringData);
    
    self.ansLoginBlob = [[KeychainItemWrapper alloc] initWithIdentifier:@"ans_login_blob" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.ansLoginBlob objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====ans_login_blob===%@", stringData);
    
    //Device
    self.devCrdId = [[KeychainItemWrapper alloc] initWithIdentifier:@"device_id" accessGroup:credentialsGroup];
    NSData *devCrdIdData = [self.devCrdId objectForKey:(__bridge id)kSecAttrService];
    if (devCrdIdData) {
        NSLog(@"=====device_id===%llu", *((u_int64_t*)[devCrdIdData bytes]));
    } else {
        NSLog(@"=====device_id===%@", devCrdIdData);
    }
    
    self.devCrdClear = [[KeychainItemWrapper alloc] initWithIdentifier:@"device_creds_clear" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.devCrdClear objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====device_creds_clear===%@", stringData);
    
    self.devCrdSecret = [[KeychainItemWrapper alloc] initWithIdentifier:@"device_creds_secret" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.devCrdSecret objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====device_creds_secret===%@", stringData);
    
    self.devCrdToken = [[KeychainItemWrapper alloc] initWithIdentifier:@"device_renewal_token" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.devCrdToken objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====device_renewal_token===%@", stringData);
    
    self.devIsLinked = [[KeychainItemWrapper alloc] initWithIdentifier:@"is_device_linked" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.devIsLinked objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====is_device_linked===%@", stringData);
    //--------- Log end
    
    self.userCred = [[KeychainItemWrapper alloc] initWithIdentifier:@"user_credential" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.userCred objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====user_credential===%@", stringData);
    
    self.devCred = [[KeychainItemWrapper alloc] initWithIdentifier:@"device_credential" accessGroup:credentialsGroup];
    stringData = (NSString *)[self.devCred objectForKey:(__bridge id)kSecAttrService];
    NSLog(@"=====device_credential===%@", stringData);
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    self.resetButton = nil;
    self.domainTextField = nil;
    self.userGroupTextField = nil;
    self.confKeychainItemWrapper = nil;
    
    self.crdUser = nil;
    self.crdIAS = nil;
    self.ansSessionKey = nil;
    self.ansLoginBlob = nil;
    self.devCrdId = nil;
    self.devCrdClear = nil;
    self.devCrdSecret = nil;
    self.devCrdToken = nil;
    self.devIsLinked = nil;
    
    self.userCred = nil;
    self.devCred = nil;
}

- (IBAction)saveAction:(id)sender
{
    // If domain text field is empty, assign a default value. ("cloud.acer.com")
    if ([self.domainTextField.text length] == 0) {
        [self.domainTextField setText:@"cloud.acer.com"];
    }

    NSString *templateFilePath = [[NSBundle mainBundle] pathForResource:@"ccd.conf.tmpl" ofType:nil];
    
    NSError *stringFileError = nil;
    NSString *confContent = [[NSString alloc] initWithContentsOfFile:templateFilePath encoding:NSASCIIStringEncoding error:&stringFileError];
    NSString *modifiedConfContent = [confContent stringByReplacingOccurrencesOfString:@"${DOMAIN}" withString:self.domainTextField.text];
    modifiedConfContent = [modifiedConfContent stringByReplacingOccurrencesOfString:@"${GROUP}" withString:self.userGroupTextField.text];
    NSLog(@"%@",modifiedConfContent);
    
    NSData *confData = [modifiedConfContent dataUsingEncoding:NSUTF8StringEncoding];
    [self.confKeychainItemWrapper setObject:confData forKey:(__bridge id)kSecAttrService];
    
    [self.resetButton setEnabled:YES];
    [self.resetButton setHidden:NO];
}

- (IBAction)resetAction:(id)sender
{
    [self.domainTextField setText:@""];
    [self.userGroupTextField setText:@""];
    [self.confKeychainItemWrapper resetKeychainItem];
    [self.resetButton setEnabled:NO];
    [self.resetButton setHidden:YES];
}

- (IBAction)resetCredAction:(id)sender
{
    // Clear user credentials
    [self.crdUser resetKeychainItem];
    [self.crdIAS resetKeychainItem];
    // Clear ANS credentials
    [self.ansSessionKey resetKeychainItem];
    [self.ansLoginBlob resetKeychainItem];
    // Clear device credentials
    [self.devCrdId resetKeychainItem];
    [self.devCrdClear resetKeychainItem];
    [self.devCrdSecret resetKeychainItem];
    [self.devCrdToken resetKeychainItem];
    [self.devIsLinked resetKeychainItem];
    
    [self.userCred resetKeychainItem];
    [self.devCred resetKeychainItem];
}

@end
