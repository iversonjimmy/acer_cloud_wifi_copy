#import <UIKit/UIKit.h>

/*
 The KeychainItemWrapper class is an abstraction layer for the iPhone Keychain communication. It is merely a 
 simple wrapper to provide a distinct barrier between all the idiosyncracies involved with the Keychain
 CF/NS container objects.
 */
@interface KeychainItemWrapper : NSObject

// Designated initializer.
- (id)initWithAccessGroup:(NSString *) accessGroup;
- (id)getObjectWithIdentifier: (NSString *)identifier status:(OSStatus *) status;
- (bool)setObject:(id)inObject forIdentifier:(NSString *)identifier status:(OSStatus *) status;

// Initializes and resets the default generic keychain item data.
- (bool)resetKeychainItem:(NSString *)identifier status:(OSStatus *) status;

@end