#import <Foundation/NSArray.h>
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SCPreferences.h>
#import <SystemConfiguration/SCNetworkConfiguration.h>
#include <sys/syslimits.h>
#include <sys/stat.h>
#include <mach-o/dyld.h>
#include "common.h"

int setUid()
{
  char exeFullPath [PATH_MAX];
  uint32_t size = PATH_MAX;
  if (_NSGetExecutablePath(exeFullPath, &size) != 0)
  {
    printf("Path longer than %d, should not occur!!!!!", size);
    return SYSCALL_FAILED;
  }
  if (chown(exeFullPath, 0, 0) != 0) // root:wheel
  {
    puts("Error chown");
    return NO_PERMISSION;
  }
  if (chmod(exeFullPath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | S_ISUID) != 0)
  {
    puts("Error chmod");
    return NO_PERMISSION;
  }
  return RET_NO_ERROR;
}

int toggleProxy(bool turnOn, const char* proxyHost, const char* proxyPort)
{
  NSString* nsProxyHost = [[NSString alloc] initWithCString: proxyHost encoding:NSUTF8StringEncoding];
  NSNumber* nsProxyPort = [[NSNumber alloc] initWithLong: [[[NSString alloc] initWithCString: proxyPort encoding:NSUTF8StringEncoding] integerValue]];
  NSString* nsOldProxyHost;
  NSNumber* nsOldProxyPort;
  int ret = RET_NO_ERROR;
  Boolean success;

  SCNetworkSetRef networkSetRef;
  CFArrayRef networkServicesArrayRef;
  SCNetworkServiceRef networkServiceRef;
  SCNetworkProtocolRef proxyProtocolRef;
  NSDictionary *oldPreferences;
  NSMutableDictionary *newPreferences;

  // Get System Preferences Lock
  SCPreferencesRef prefsRef = SCPreferencesCreate(NULL, CFSTR("org.getlantern.lantern"), NULL);

  if(prefsRef==NULL) {
    NSLog(@"Fail to obtain Preferences Ref");
    ret = NO_PERMISSION;
    goto freePrefsRef;
  }

  success = SCPreferencesLock(prefsRef, true);
  if (!success) {
    NSLog(@"Fail to obtain PreferencesLock");
    ret = NO_PERMISSION;
    goto freePrefsRef;
  }

  // Get available network services
  networkSetRef = SCNetworkSetCopyCurrent(prefsRef);
  if(networkSetRef == NULL) {
    NSLog(@"Fail to get available network services");
    ret = SYSCALL_FAILED;
    goto freeNetworkSetRef;
  }

  //Look up interface entry
  networkServicesArrayRef = SCNetworkSetCopyServices(networkSetRef);
  networkServiceRef = NULL;
  for (long i = 0; i < CFArrayGetCount(networkServicesArrayRef); i++) {
    networkServiceRef = CFArrayGetValueAtIndex(networkServicesArrayRef, i);

    // Get proxy protocol
    proxyProtocolRef = SCNetworkServiceCopyProtocol(networkServiceRef, kSCNetworkProtocolTypeProxies);
    if(proxyProtocolRef == NULL) {
      NSLog(@"Couldn't acquire copy of proxyProtocol");
      ret = SYSCALL_FAILED;
      goto freeProxyProtocolRef;
    }

    oldPreferences = (__bridge NSDictionary*)SCNetworkProtocolGetConfiguration(proxyProtocolRef);
    newPreferences = [NSMutableDictionary dictionaryWithDictionary: oldPreferences];

    if(turnOn == true) {
      [newPreferences setValue: nsProxyHost forKey:(NSString*)kSCPropNetProxiesHTTPProxy];
      [newPreferences setValue: nsProxyHost forKey:(NSString*)kSCPropNetProxiesHTTPSProxy];
      [newPreferences setValue: nsProxyPort forKey:(NSString*)kSCPropNetProxiesHTTPPort];
      [newPreferences setValue: nsProxyPort forKey:(NSString*)kSCPropNetProxiesHTTPSPort];
      [newPreferences setValue:[NSNumber numberWithInt:1] forKey:(NSString*)kSCPropNetProxiesHTTPEnable];
      [newPreferences setValue:[NSNumber numberWithInt:1] forKey:(NSString*)kSCPropNetProxiesHTTPSEnable];
    } else {
      nsOldProxyHost = [newPreferences valueForKey:(NSString*)kSCPropNetProxiesHTTPProxy];
      nsOldProxyPort = [newPreferences valueForKey:(NSString*)kSCPropNetProxiesHTTPPort];
      if ([nsProxyHost isEqualToString:nsOldProxyHost] && [nsProxyPort intValue] == [nsOldProxyPort intValue]) {
        [newPreferences setValue:[NSNumber numberWithInt:0] forKey:(NSString*)kSCPropNetProxiesHTTPEnable];
        [newPreferences setValue: @"" forKey:(NSString*)kSCPropNetProxiesHTTPProxy];
        [newPreferences setValue: @"" forKey:(NSString*)kSCPropNetProxiesHTTPPort];
      }
      nsOldProxyHost = [newPreferences valueForKey:(NSString*)kSCPropNetProxiesHTTPSProxy];
      nsOldProxyPort = [newPreferences valueForKey:(NSString*)kSCPropNetProxiesHTTPSPort];
      if ([nsProxyHost isEqualToString:nsOldProxyHost] && [nsProxyPort intValue] == [nsOldProxyPort intValue]) {
        [newPreferences setValue:[NSNumber numberWithInt:0] forKey:(NSString*)kSCPropNetProxiesHTTPSEnable];
        [newPreferences setValue: @"" forKey:(NSString*)kSCPropNetProxiesHTTPSProxy];
        [newPreferences setValue: @"" forKey:(NSString*)kSCPropNetProxiesHTTPSPort];
      }
    }

    success = SCNetworkProtocolSetConfiguration(proxyProtocolRef, (__bridge CFDictionaryRef)newPreferences);
    if(!success) {
      NSLog(@"Failed to set Protocol Configuration");
      ret = SYSCALL_FAILED;
      goto freeProxyProtocolRef;
    }

freeProxyProtocolRef:
    CFRelease(proxyProtocolRef);
  }

  success = SCPreferencesCommitChanges(prefsRef);
  if(!success) {
    NSLog(@"Failed to Commit Changes");
    ret = SYSCALL_FAILED;
    goto freeNetworkServicesArrayRef;
  }

  success = SCPreferencesApplyChanges(prefsRef);
  if(!success) {
    NSLog(@"Failed to Apply Changes");
    ret = SYSCALL_FAILED;
    goto freeNetworkServicesArrayRef;
  }
  success = true;

  //Free Resources
freeNetworkServicesArrayRef:
  CFRelease(networkServicesArrayRef);
freeNetworkSetRef:
  CFRelease(networkSetRef);
freePrefsRef:
  SCPreferencesUnlock(prefsRef);
  CFRelease(prefsRef);

  return ret;
}
