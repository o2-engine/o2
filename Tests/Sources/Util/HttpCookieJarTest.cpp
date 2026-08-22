#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <time.h>

#include "o2/Network/Http/HttpCookieJar.h"

using namespace o2;

TEST(HttpCookieJar, StoresAndSendsBack)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "session=abc; Path=/");
    EXPECT_EQ(jar->GetCookieHeader(url), String("session=abc"));
}

TEST(HttpCookieJar, MultipleCookiesJoined)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "a=1; Path=/");
    jar->SetFromHeader(url, "b=2; Path=/");

    String header = jar->GetCookieHeader(url);
    EXPECT_TRUE(header.find("a=1") != std::string::npos);
    EXPECT_TRUE(header.find("b=2") != std::string::npos);
    EXPECT_TRUE(header.find("; ") != std::string::npos);
}

TEST(HttpCookieJar, ReplacesSameNameDomainPath)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "a=old; Path=/");
    jar->SetFromHeader(url, "a=new; Path=/");

    EXPECT_EQ(jar->GetCookies().Count(), 1);
    EXPECT_EQ(jar->GetCookieHeader(url), String("a=new"));
}

TEST(HttpCookieJar, HostOnlyDoesNotMatchSubdomain)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "host=1; Path=/");

    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://sub.example.com/")), String());
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/page")), String("host=1"));
}

TEST(HttpCookieJar, DomainCookieMatchesSubdomains)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "shared=1; Path=/; Domain=example.com");

    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://sub.example.com/")), String("shared=1"));
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/")), String("shared=1"));
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://notexample.com/")), String());
}

TEST(HttpCookieJar, PathMatching)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "scoped=1; Path=/game");

    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/game")), String("scoped=1"));
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/game/level")), String("scoped=1"));
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/other")), String());
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/gamer")), String());
}

TEST(HttpCookieJar, SecureCookieOnlyOverHttps)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("https://example.com/");

    jar->SetFromHeader(url, "token=s3cret; Path=/; Secure");

    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("https://example.com/")), String("token=s3cret"));
    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/")), String());
}

TEST(HttpCookieJar, NegativeMaxAgeRemovesCookie)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "a=1; Path=/");
    EXPECT_EQ(jar->GetCookies().Count(), 1);

    jar->SetFromHeader(url, "a=1; Path=/; Max-Age=-1");
    EXPECT_EQ(jar->GetCookies().Count(), 0);
}

TEST(HttpCookieJar, ExpiredCookieNotSent)
{
    auto jar = mmake<HttpCookieJar>();

    HttpCookie expired;
    expired.name = "old";
    expired.value = "1";
    expired.domain = "example.com";
    expired.path = "/";
    expired.expiresAt = (Int64)time(nullptr) - 10;
    jar->Add(expired);

    EXPECT_EQ(jar->GetCookieHeader(Url::Parse("http://example.com/")), String());
    EXPECT_EQ(jar->GetCookies().Count(), 0);
}

TEST(HttpCookieJar, MaxAgeKeepsCookieAlive)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "keep=1; Path=/; Max-Age=3600");
    EXPECT_EQ(jar->GetCookieHeader(url), String("keep=1"));

    auto& cookie = jar->GetCookies()[0];
    EXPECT_GT(cookie.expiresAt, (Int64)time(nullptr));
}

TEST(HttpCookieJar, ExpiresDateParsed)
{
    auto jar = mmake<HttpCookieJar>();
    Url url = Url::Parse("http://example.com/");

    jar->SetFromHeader(url, "future=1; Path=/; Expires=Wed, 21 Oct 2100 07:28:00 GMT");
    EXPECT_EQ(jar->GetCookies().Count(), 1);
    EXPECT_GT(jar->GetCookies()[0].expiresAt, (Int64)time(nullptr));

    jar->SetFromHeader(url, "past=1; Path=/; Expires=Wed, 21 Oct 2015 07:28:00 GMT");
    EXPECT_EQ(jar->GetCookieHeader(url), String("future=1"));
}
