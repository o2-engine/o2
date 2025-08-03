#ifdef PLATFORM_MAC

#import <Cocoa/Cocoa.h>

// Helper functions that work with raw C types to avoid namespace conflicts
static const char* ShowOpenDialogCocoa(const char* title, const char** extensionKeys, const char** extensionValues, int extensionCount, const char* defaultPath)
{
    static char* resultPath = nullptr;
    if (resultPath) {
        free(resultPath);
        resultPath = nullptr;
    }
    
    @autoreleasepool {
        NSOpenPanel* openPanel = [NSOpenPanel openPanel];
        
        // Set title if provided
        if (title && strlen(title) > 0) {
            [openPanel setTitle:[NSString stringWithUTF8String:title]];
        }
        
        // Set initial directory
        if (defaultPath && strlen(defaultPath) > 0) {
            NSString* pathStr = [NSString stringWithUTF8String:defaultPath];
            NSURL* directoryURL = [NSURL fileURLWithPath:pathStr];
            [openPanel setDirectoryURL:directoryURL];
        }
        
        // Set allowed file types from extensions
        if (extensionCount > 0) {
            NSMutableArray* allowedTypes = [[NSMutableArray alloc] init];
            
            for (int i = 0; i < extensionCount; i++) {
                const char* extensionPattern = extensionValues[i];
                
                // Parse extension pattern like "*.txt" or "*.png;*.jpg"
                NSString* patternStr = [NSString stringWithUTF8String:extensionPattern];
                NSArray* patterns = [patternStr componentsSeparatedByString:@";"];
                
                for (NSString* pattern in patterns) {
                    NSString* cleanPattern = [pattern stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    if ([cleanPattern hasPrefix:@"*."]) {
                        NSString* fileExt = [cleanPattern substringFromIndex:2];
                        [allowedTypes addObject:fileExt];
                    }
                    else if ([cleanPattern hasPrefix:@"*"]) {
                        NSString* fileExt = [cleanPattern substringFromIndex:1];
                        [allowedTypes addObject:fileExt];
                    }
                }
            }
            
            if ([allowedTypes count] > 0) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                [openPanel setAllowedFileTypes:allowedTypes];
#pragma clang diagnostic pop
            }
        }
        
        [openPanel setCanChooseFiles:YES];
        [openPanel setCanChooseDirectories:NO];
        [openPanel setAllowsMultipleSelection:NO];
        
        NSModalResponse response = [openPanel runModal];
        
        if (response == NSModalResponseOK) {
            NSURL* selectedURL = [[openPanel URLs] firstObject];
            NSString* selectedPath = [selectedURL path];
            const char* cPath = [selectedPath UTF8String];
            resultPath = (char*)malloc(strlen(cPath) + 1);
            strcpy(resultPath, cPath);
            return resultPath;
        }
        
        return nullptr;
    }
}

static const char* ShowSaveDialogCocoa(const char* title, const char** extensionKeys, const char** extensionValues, int extensionCount, const char* defaultPath)
{
    static char* resultPath = nullptr;
    if (resultPath) {
        free(resultPath);
        resultPath = nullptr;
    }
    
    @autoreleasepool {
        NSSavePanel* savePanel = [NSSavePanel savePanel];
        
        // Set title if provided
        if (title && strlen(title) > 0) {
            [savePanel setTitle:[NSString stringWithUTF8String:title]];
        }
        
        // Set initial directory and filename
        if (defaultPath && strlen(defaultPath) > 0) {
            NSString* pathStr = [NSString stringWithUTF8String:defaultPath];
            NSURL* pathURL = [NSURL fileURLWithPath:pathStr];
            
            BOOL isDirectory;
            NSFileManager* fileManager = [NSFileManager defaultManager];
            
            if ([fileManager fileExistsAtPath:pathStr isDirectory:&isDirectory]) {
                if (isDirectory) {
                    [savePanel setDirectoryURL:pathURL];
                } else {
                    // If it's a file path, set directory to parent and filename to the file
                    NSURL* directoryURL = [pathURL URLByDeletingLastPathComponent];
                    NSString* filename = [pathURL lastPathComponent];
                    [savePanel setDirectoryURL:directoryURL];
                    [savePanel setNameFieldStringValue:filename];
                }
            } else {
                // Path doesn't exist, check if it looks like a file or directory
                if ([pathStr hasSuffix:@"/"]) {
                    // Looks like a directory
                    [savePanel setDirectoryURL:pathURL];
                } else {
                    // Treat as a file path
                    NSURL* directoryURL = [pathURL URLByDeletingLastPathComponent];
                    NSString* filename = [pathURL lastPathComponent];
                    [savePanel setDirectoryURL:directoryURL];
                    [savePanel setNameFieldStringValue:filename];
                }
            }
        }
        
        // Set allowed file types from extensions
        if (extensionCount > 0) {
            NSMutableArray* allowedTypes = [[NSMutableArray alloc] init];
            
            for (int i = 0; i < extensionCount; i++) {
                const char* extensionPattern = extensionValues[i];
                
                // Parse extension pattern like "*.txt" or "*.png;*.jpg"
                NSString* patternStr = [NSString stringWithUTF8String:extensionPattern];
                NSArray* patterns = [patternStr componentsSeparatedByString:@";"];
                
                for (NSString* pattern in patterns) {
                    NSString* cleanPattern = [pattern stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    if ([cleanPattern hasPrefix:@"*."]) {
                        NSString* fileExt = [cleanPattern substringFromIndex:2];
                        [allowedTypes addObject:fileExt];
                    }
                    else if ([cleanPattern hasPrefix:@"*"]) {
                        NSString* fileExt = [cleanPattern substringFromIndex:1];
                        [allowedTypes addObject:fileExt];
                    }
                }
            }
            
            if ([allowedTypes count] > 0) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                [savePanel setAllowedFileTypes:allowedTypes];
#pragma clang diagnostic pop
            }
        }
        
        [savePanel setCanCreateDirectories:YES];
        
        NSModalResponse response = [savePanel runModal];
        
        if (response == NSModalResponseOK) {
            NSURL* selectedURL = [savePanel URL];
            NSString* selectedPath = [selectedURL path];
            const char* cPath = [selectedPath UTF8String];
            resultPath = (char*)malloc(strlen(cPath) + 1);
            strcpy(resultPath, cPath);
            return resultPath;
        }
        
        return nullptr;
    }
}

#include "o2Editor/stdafx.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"

using namespace o2;

namespace Editor
{
    String GetOpenFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath /*= ""*/)
    {
        // Convert o2 types to C types for the helper function
        const char* titlePtr = title.IsEmpty() ? nullptr : title.Data();
        const char* defaultPathPtr = defaultPath.IsEmpty() ? nullptr : defaultPath.Data();
        
        // Convert extensions map to C arrays
        int extensionCount = extensions.Count();
        const char** extensionKeys = nullptr;
        const char** extensionValues = nullptr;
        
        if (extensionCount > 0) {
            extensionKeys = new const char*[extensionCount];
            extensionValues = new const char*[extensionCount];
            
            int i = 0;
            for (auto it = ((std::map<String, String>&)extensions).begin(); it != ((std::map<String, String>&)extensions).end(); ++it) {
                extensionKeys[i] = it->first.Data();
                extensionValues[i] = it->second.Data();
                i++;
            }
        }
        
        // Call the Cocoa helper function
        const char* result = ShowOpenDialogCocoa(titlePtr, extensionKeys, extensionValues, extensionCount, defaultPathPtr);
        
        // Clean up
        if (extensionKeys) delete[] extensionKeys;
        if (extensionValues) delete[] extensionValues;
        
        return result ? String(result) : String("");
    }

    String GetSaveFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath /*= ""*/)
    {
        // Convert o2 types to C types for the helper function
        const char* titlePtr = title.IsEmpty() ? nullptr : title.Data();
        const char* defaultPathPtr = defaultPath.IsEmpty() ? nullptr : defaultPath.Data();
        
        // Convert extensions map to C arrays
        int extensionCount = extensions.Count();
        const char** extensionKeys = nullptr;
        const char** extensionValues = nullptr;
        
        if (extensionCount > 0) {
            extensionKeys = new const char*[extensionCount];
            extensionValues = new const char*[extensionCount];
            
            int i = 0;
            for (auto it = ((std::map<String, String>&)extensions).begin(); it != ((std::map<String, String>&)extensions).end(); ++it) {
                extensionKeys[i] = it->first.Data();
                extensionValues[i] = it->second.Data();
                i++;
            }
        }
        
        // Call the Cocoa helper function
        const char* result = ShowSaveDialogCocoa(titlePtr, extensionKeys, extensionValues, extensionCount, defaultPathPtr);
        
        // Clean up
        if (extensionKeys) delete[] extensionKeys;
        if (extensionValues) delete[] extensionValues;
        
        return result ? String(result) : String("");
    }
}

#endif