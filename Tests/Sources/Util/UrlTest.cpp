#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Network/Url.h"

using namespace o2;

TEST(Url, ParsesFullUrl)
{
    Url url = Url::Parse("http://example.com:8080/path/to/page?query=1");
    EXPECT_TRUE(url.isValid);
    EXPECT_EQ(url.scheme, String("http"));
    EXPECT_EQ(url.host, String("example.com"));
    EXPECT_EQ(url.port, 8080);
    EXPECT_EQ(url.path, String("/path/to/page?query=1"));
}

TEST(Url, DefaultPorts)
{
    EXPECT_EQ(Url::Parse("http://example.com/").port, 80);
    EXPECT_EQ(Url::Parse("https://example.com/").port, 443);
}

TEST(Url, DefaultPath)
{
    Url url = Url::Parse("http://example.com");
    EXPECT_TRUE(url.isValid);
    EXPECT_EQ(url.path, String("/"));
}

TEST(Url, HostAndSchemeAreLowercased)
{
    Url url = Url::Parse("HTTP://Example.COM/Path");
    EXPECT_TRUE(url.isValid);
    EXPECT_EQ(url.scheme, String("http"));
    EXPECT_EQ(url.host, String("example.com"));
    EXPECT_EQ(url.path, String("/Path"));
}

TEST(Url, QueryWithoutPath)
{
    Url url = Url::Parse("http://example.com?a=1");
    EXPECT_TRUE(url.isValid);
    EXPECT_EQ(url.path, String("/?a=1"));
}

TEST(Url, InvalidUrls)
{
    EXPECT_FALSE(Url::Parse("").isValid);
    EXPECT_FALSE(Url::Parse("example.com").isValid);
    EXPECT_FALSE(Url::Parse("ftp://example.com/").isValid);
    EXPECT_FALSE(Url::Parse("http://").isValid);
    EXPECT_FALSE(Url::Parse("http://host:notaport/").isValid);
    EXPECT_FALSE(Url::Parse("http://host:0/").isValid);
    EXPECT_FALSE(Url::Parse("http://host:99999/").isValid);
}

TEST(Url, ToStringOmitsDefaultPort)
{
    EXPECT_EQ(Url::Parse("http://example.com:80/x").ToString(), String("http://example.com/x"));
    EXPECT_EQ(Url::Parse("http://example.com:8080/x").ToString(), String("http://example.com:8080/x"));
}

TEST(Url, ResolveRedirect)
{
    Url base = Url::Parse("http://example.com:8080/dir/page?q=1");

    Url absolute = base.ResolveRedirect("https://other.org/target");
    EXPECT_TRUE(absolute.isValid);
    EXPECT_EQ(absolute.host, String("other.org"));
    EXPECT_EQ(absolute.scheme, String("https"));

    Url absolutePath = base.ResolveRedirect("/moved");
    EXPECT_EQ(absolutePath.host, String("example.com"));
    EXPECT_EQ(absolutePath.port, 8080);
    EXPECT_EQ(absolutePath.path, String("/moved"));

    Url relative = base.ResolveRedirect("sibling");
    EXPECT_EQ(relative.path, String("/dir/sibling"));
}
