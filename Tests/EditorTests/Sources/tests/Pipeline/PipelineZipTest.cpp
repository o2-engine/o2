#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Pipeline/PipelineZip.h"

using namespace o2;
using namespace Editor;

TEST(PipelineZip, WritesAndReadsStoredAndDeflatedEntries)
{
    Vector<PipelineZip::Entry> entries = {
        { "pipeline.json", "{\"a\":1}" },
        { "results/n1.txt", String(std::string(5000, 'x')) },
        { "results/bin.dat", String(std::string("\x00\x01\x02\xff", 4)) },
        { "empty.txt", "" }
    };

    for (bool compress : { false, true })
    {
        String bytes = PipelineZip::Write(entries, compress);
        EXPECT_TRUE(PipelineZip::IsZip(bytes));

        Vector<PipelineZip::Entry> read;
        String error;
        ASSERT_TRUE(PipelineZip::Read(bytes, read, error)) << error;
        ASSERT_EQ(read.Count(), entries.Count());
        for (int i = 0; i < entries.Count(); i++)
        {
            EXPECT_EQ(read[i].name, entries[i].name);
            EXPECT_EQ(read[i].data, entries[i].data) << entries[i].name;
        }
    }

    EXPECT_LT(PipelineZip::Write(entries, true).Length(), PipelineZip::Write(entries, false).Length());
}

TEST(PipelineZip, RejectsBrokenInput)
{
    Vector<PipelineZip::Entry> read;
    String error;
    EXPECT_FALSE(PipelineZip::IsZip("nope"));
    EXPECT_FALSE(PipelineZip::Read("not a zip archive, just a text long enough to be scanned", read, error));
    EXPECT_FALSE(error.IsEmpty());

    String bytes = PipelineZip::Write({ { "a.txt", "hello" } }, false);
    String truncated = bytes.SubStr(0, bytes.Length() - 10);
    EXPECT_FALSE(PipelineZip::Read(truncated, read, error));
}
