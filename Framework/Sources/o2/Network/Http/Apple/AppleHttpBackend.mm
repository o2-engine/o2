#include "o2/stdafx.h"

#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)

#include "o2/Network/Http/HttpBackend.h"

#import <Foundation/Foundation.h>

// Session delegate that blocks automatic redirect following: redirects are handled by the
// engine's client layer so cookies and caching behave the same on every platform
@interface O2HttpSessionDelegate: NSObject<NSURLSessionTaskDelegate>
@end

@implementation O2HttpSessionDelegate

- (void)URLSession:(NSURLSession*)session
              task:(NSURLSessionTask*)task
willPerformHTTPRedirection:(NSHTTPURLResponse*)response
        newRequest:(NSURLRequest*)request
 completionHandler:(void (^)(NSURLRequest*))completionHandler
{
    completionHandler(nil);
}

@end

namespace o2
{
    // Splits the comma-joined Set-Cookie value from allHeaderFields back into separate cookie
    // strings. A comma starts a new cookie only when it is followed by a token and '=' before
    // any ';' — commas inside Expires dates do not match that shape
    static void SplitJoinedSetCookie(const String& joined, Vector<String>& outCookies)
    {
        size_t begin = 0;
        for (size_t i = 0; i < joined.size(); i++)
        {
            if (joined[i] != ',')
                continue;

            size_t probe = i + 1;
            while (probe < joined.size() && joined[probe] == ' ')
                probe++;

            size_t tokenEnd = probe;
            while (tokenEnd < joined.size() && joined[tokenEnd] != '=' && joined[tokenEnd] != ';' &&
                   joined[tokenEnd] != ',' && joined[tokenEnd] != ' ')
            {
                tokenEnd++;
            }

            if (tokenEnd > probe && tokenEnd < joined.size() && joined[tokenEnd] == '=')
            {
                outCookies.Add(joined.substr(begin, i - begin));
                begin = probe;
                i = probe;
            }
        }

        outCookies.Add(joined.substr(begin));
    }

    // NSURLSession-based backend for macOS and iOS. Cookies, cache and redirects are disabled at
    // the session, the engine's client layer owns them
    class AppleHttpBackend: public IHttpBackend
    {
    public:
        AppleHttpBackend()
        {
            NSURLSessionConfiguration* configuration = [NSURLSessionConfiguration ephemeralSessionConfiguration];
            configuration.URLCache = nil;
            configuration.HTTPCookieStorage = nil;
            configuration.HTTPShouldSetCookies = NO;
            configuration.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;

            mDelegate = [[O2HttpSessionDelegate alloc] init];
            mSession = [NSURLSession sessionWithConfiguration:configuration
                                                     delegate:(O2HttpSessionDelegate*)mDelegate
                                                delegateQueue:nil];
            [(NSURLSession*)mSession retain];
        }

        ~AppleHttpBackend() override
        {
            [(NSURLSession*)mSession invalidateAndCancel];
            [(NSURLSession*)mSession release];
            [(O2HttpSessionDelegate*)mDelegate release];
        }

        void Perform(const SharedRef<HttpTransfer>& transfer) override
        {
            NSString* urlString = [NSString stringWithUTF8String:transfer->url.Data()];
            NSURL* url = [NSURL URLWithString:urlString];
            if (!url)
            {
                transfer->error = HttpError::InvalidUrl;
                transfer->done.Store(1);
                return;
            }

            NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
            request.HTTPMethod = [NSString stringWithUTF8String:transfer->method.Data()];
            request.timeoutInterval = (NSTimeInterval)transfer->timeout;
            request.HTTPShouldHandleCookies = NO;

            for (auto& line : transfer->headerLines)
            {
                size_t colon = line.find(':');
                if (colon == std::string::npos)
                    continue;

                String name = line.substr(0, colon);
                size_t valueBegin = line.find_first_not_of(" \t", colon + 1);
                String value = valueBegin == std::string::npos ? String() : (String)line.substr(valueBegin);

                [request setValue:[NSString stringWithUTF8String:value.Data()]
                    forHTTPHeaderField:[NSString stringWithUTF8String:name.Data()]];
            }

            if (!transfer->body.IsEmpty())
                request.HTTPBody = [NSData dataWithBytes:transfer->body.data() length:transfer->body.size()];

            // The completion handler runs on a session queue; the holder keeps the transfer alive
            // and is released exactly once, when the handler fires
            auto holder = new SharedRef<HttpTransfer>(transfer);

            NSURLSessionDataTask* task = [(NSURLSession*)mSession dataTaskWithRequest:request
                completionHandler:^(NSData* data, NSURLResponse* response, NSError* error)
            {
                HttpTransfer* result = holder->Get();

                if (error)
                {
                    switch (error.code)
                    {
                        case NSURLErrorTimedOut:
                            result->error = HttpError::Timeout;
                            break;

                        case NSURLErrorCannotFindHost:
                        case NSURLErrorDNSLookupFailed:
                            result->error = HttpError::ResolveFailed;
                            break;

                        case NSURLErrorCannotConnectToHost:
                        case NSURLErrorNetworkConnectionLost:
                        case NSURLErrorNotConnectedToInternet:
                            result->error = HttpError::ConnectionFailed;
                            break;

                        default:
                            result->error = HttpError::Internal;
                            break;
                    }
                }
                else
                {
                    NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
                    result->status = (int)httpResponse.statusCode;

                    for (NSString* key in httpResponse.allHeaderFields)
                    {
                        NSString* value = httpResponse.allHeaderFields[key];
                        String name = [key UTF8String];
                        String valueString = [value UTF8String];

                        if ([key caseInsensitiveCompare:@"Set-Cookie"] == NSOrderedSame)
                        {
                            Vector<String> cookies;
                            SplitJoinedSetCookie(valueString, cookies);
                            for (auto& cookie : cookies)
                                result->responseHeaderLines.Add("Set-Cookie: " + cookie);
                        }
                        else
                            result->responseHeaderLines.Add(name + ": " + valueString);
                    }

                    if (data)
                        result->responseBody.assign((const char*)data.bytes, (size_t)data.length);
                }

                result->done.Store(1);
                delete holder;
            }];

            [task resume];
        }

    protected:
        void* mSession = nullptr;  // Retained NSURLSession
        void* mDelegate = nullptr; // Retained redirect-blocking delegate
    };

    IHttpBackend* CreateAppleHttpBackend()
    {
        return mnew AppleHttpBackend();
    }
}

#endif // PLATFORM_MAC || PLATFORM_IOS
