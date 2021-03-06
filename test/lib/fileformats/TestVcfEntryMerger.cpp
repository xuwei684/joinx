#include "fileformats/vcf/EntryMerger.hpp"
#include "fileformats/vcf/AlleleMerger.hpp"
#include "fileformats/vcf/Builder.hpp"
#include "fileformats/vcf/MergeStrategy.hpp"
#include "fileformats/vcf/Entry.hpp"
#include "fileformats/vcf/Header.hpp"

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <gtest/gtest.h>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace Vcf;

namespace {
    void push_back(vector<Entry>& v, const Entry& e) {
        v.push_back(e);
    }

    string headerText[] = {
        "##fileformat=VCFv4.1\n"
        "##fileDate=20090805\n"
        "##source=myImputationProgramV3.1\n"
        "##reference=file:///seq/references/1000GenomesPilot-NCBI36.fasta\n"
        "##contig=<ID=20,length=62435964,assembly=B36,md5=f126cdf8a6e0c7f379d618ff66beb2da,species=\"Homo sapiens\",taxonomy=x>\n"
        "##phasing=partial\n"
        "##FILTER=<ID=q10,Description=\"Quality below 10\">\n"
        "##FILTER=<ID=s50,Description=\"Less than 50% of samples have data\">\n"
        "##INFO=<ID=VC,Number=.,Type=String,Description=\"Variant caller\">\n"
        "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n"
        "##FORMAT=<ID=GQ,Number=1,Type=Integer,Description=\"Genotype Quality\">\n"
        "##FORMAT=<ID=DP,Number=1,Type=Integer,Description=\"Read Depth\">\n"
        "##FORMAT=<ID=HQ,Number=2,Type=Integer,Description=\"Haplotype Quality\">\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tNA1\tNA2\n"
        ,
        "##fileformat=VCFv4.1\n"
        "##fileDate=20090805\n"
        "##source=myImputationProgramV3.1\n"
        "##reference=file:///seq/references/1000GenomesPilot-NCBI36.fasta\n"
        "##contig=<ID=20,length=62435964,assembly=B36,md5=f126cdf8a6e0c7f379d618ff66beb2da,species=\"Homo sapiens\",taxonomy=x>\n"
        "##phasing=partial\n"
        "##FILTER=<ID=q10,Description=\"Quality below 10\">\n"
        "##FILTER=<ID=s50,Description=\"Less than 50% of samples have data\">\n"
        "##INFO=<ID=VC,Number=.,Type=String,Description=\"Variant caller\">\n"
        "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n"
        "##FORMAT=<ID=GQ,Number=1,Type=Integer,Description=\"Genotype Quality\">\n"
        "##FORMAT=<ID=DP,Number=1,Type=Integer,Description=\"Read Depth\">\n"
        "##FORMAT=<ID=HQ,Number=2,Type=Integer,Description=\"Haplotype Quality\">\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tNA3\tNA4\n"
        ,
        "##fileformat=VCFv4.1\n"
        "##fileDate=20090805\n"
        "##source=myImputationProgramV3.1\n"
        "##reference=file:///seq/references/1000GenomesPilot-NCBI36.fasta\n"
        "##contig=<ID=20,length=62435964,assembly=B36,md5=f126cdf8a6e0c7f379d618ff66beb2da,species=\"Homo sapiens\",taxonomy=x>\n"
        "##phasing=partial\n"
        "##FILTER=<ID=q10,Description=\"Quality below 10\">\n"
        "##FILTER=<ID=s50,Description=\"Less than 50% of samples have data\">\n"
        "##INFO=<ID=VC,Number=.,Type=String,Description=\"Variant caller\">\n"
        "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n"
        "##FORMAT=<ID=GQ,Number=1,Type=Integer,Description=\"Genotype Quality\">\n"
        "##FORMAT=<ID=DP,Number=1,Type=Integer,Description=\"Read Depth\">\n"
        "##FORMAT=<ID=HQ,Number=2,Type=Integer,Description=\"Haplotype Quality\">\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tNA5\tNA6\n"
    };

    string snvEntryText[] = {
        "20\t14370\tid1\tG\tA\t29\tPASS\tVC=Samtools\tGT:GQ:DP:HQ\t0|1:48:1:51,51\t1|0:48:8:51,51",
        "20\t14370\tid1;id2\tG\tC\t.\tPASS\tVC=Samtools\tGT:GQ:DP:HQ\t0|1:48:1:51,51\t1/1:43:5:.,.",
        "20\t14370\tid3\tG\tC\t31\tPASS\tVC=Varscan,Samtools\tGT:GQ:DP:HQ\t.\t1/0:44:6:50,40"
    };

    string indelEntryText[] = {
        "20\t14370\tid1\tTAC\tT\t29\tPASS\tVC=Samtools\tGT:GQ:DP:HQ\t0|1:48:1:51,51\t1|0:48:8:51,51",
        "20\t14370\tid1\tTACAG\tT\t.\tPASS\tVC=Samtools"
    };
}

class TestVcfEntryMerger : public ::testing::Test {
public:
    void SetUp() {
        unsigned nHeaders = sizeof(headerText)/sizeof(headerText[0]);
        for (unsigned i = 0; i < nHeaders; ++i) {
            stringstream ss(headerText[i]);
            _headers.push_back(Header::fromStream(ss));
        }

        unsigned nSnvs = sizeof(snvEntryText)/sizeof(snvEntryText[0]);
        assert(nSnvs <= nHeaders);

       for (unsigned i = 0; i < nSnvs; ++i) {
            _mergedHeader.merge(_headers[i]);
            stringstream ss(snvEntryText[i]);
            Entry e;
            Entry::parseLine(&_headers[i], snvEntryText[i], e);
            _snvs.push_back(e);
        }

        unsigned nIndels = sizeof(indelEntryText)/sizeof(indelEntryText[0]);
        assert(nIndels <= nHeaders);

       for (unsigned i = 0; i < nIndels; ++i) {
            stringstream ss(indelEntryText[i]);
            Entry e;
            Entry::parseLine(&_headers[i], indelEntryText[i], e);
            _indels.push_back(e);
        }

        _defaultMs.reset(new MergeStrategy(&_mergedHeader));
    }

    Entry makeEntry(string chrom, int64_t pos, string const& ref, string const& alt) {
        stringstream ss;
        ss << chrom << "\t" << pos << "\t.\t" << ref << "\t" << alt << "\t.\t.\t.";
        return Entry(&_mergedHeader, ss.str());
    }

    Header _mergedHeader;
    vector<Entry> _snvs;
    vector<Entry> _indels;
    vector<Header> _headers;
    boost::scoped_ptr<MergeStrategy> _defaultMs;
};

TEST_F(TestVcfEntryMerger, mergeDifferentSampleFields) {
    string t1="20\t14370\tid1\tT\tG\t.\tPASS\tVC=Samtools\tGT:DP\t1/1:30";
    string t2="20\t14370\tid1\tT\tC\t29\tPASS\tVC=Samtools\tGT\t0/1";
    Entry entries[2];
    Entry::parseLine(&_headers[0], t1, entries[0]);
    Entry::parseLine(&_headers[1], t2, entries[1]);
    EntryMerger merger(*_defaultMs, &_mergedHeader, entries, entries + 2);
    Entry entry(std::move(merger));
    std::stringstream ss;
    // There was a bug about printing null CustomValue fields in sample
    // data that happened in cases like this.
    EXPECT_NO_THROW(ss << entry);
}

TEST_F(TestVcfEntryMerger, merge) {
    // We want to concatenate variant caller names
    _defaultMs->setMerger("VC", "uniq-concat");

    EntryMerger merger(*_defaultMs, &_mergedHeader, &*_snvs.begin(), &*_snvs.end());
    ASSERT_EQ("20", merger.chrom());
    ASSERT_EQ(14370u, merger.pos());
    ASSERT_EQ(3u, merger.identifiers().size());
    ASSERT_EQ("G", merger.ref());
    ASSERT_EQ(Entry::MISSING_QUALITY, merger.qual());

    Entry mergedEntry(std::move(merger));

    // check the simple fields: chrom, pos, etc.
    ASSERT_EQ("20", mergedEntry.chrom());
    ASSERT_EQ(14370u, mergedEntry.pos());

    // make sure identifiers are merged properly without duplicates
    ASSERT_EQ(3u, mergedEntry.identifiers().size());
    auto iter = mergedEntry.identifiers().begin();
    ASSERT_EQ("id1", *iter++);
    ASSERT_EQ("id2", *iter++);
    ASSERT_EQ("id3", *iter++);
    ASSERT_TRUE(iter == mergedEntry.identifiers().end());

    // make sure reference allele doesn't change
    ASSERT_EQ("G", mergedEntry.ref());

    // for now, we are assigning combined quality scores of '.' to merged entries
    // unless there is only 1 present
    ASSERT_EQ(Entry::MISSING_QUALITY, mergedEntry.qual());

    // check that genotype allele references were updated
    // sample 1 (from entry 1)
    const CustomValue* v = mergedEntry.sampleData().get(0, "GT");
    ASSERT_TRUE(v);
    ASSERT_EQ("0|1", v->toString());

    // sample 2 (from entry 1)
    v = mergedEntry.sampleData().get(1, "GT");
    ASSERT_TRUE(v);
    ASSERT_EQ("1|0", v->toString());

    // sample 3 (from entry 2)
    v = mergedEntry.sampleData().get(2, "GT");
    ASSERT_TRUE(v);
    ASSERT_EQ("0|2", v->toString());

    // sample 4 (from entry 2)
    v = mergedEntry.sampleData().get(3, "GT");
    ASSERT_TRUE(v);
    ASSERT_EQ("2/2", v->toString());

    // sample 5 (from entry 3)
    v = mergedEntry.sampleData().get(4, "GT");
    ASSERT_FALSE(v);

    // sample 6 (from entry 3)
    v = mergedEntry.sampleData().get(5, "GT");
    ASSERT_TRUE(v);
    ASSERT_EQ("2/0", v->toString());

    // check that the VC field was merged
    v = mergedEntry.info("VC");
    ASSERT_TRUE(v);
    ASSERT_EQ("Samtools,Varscan", v->toString());
}

TEST_F(TestVcfEntryMerger, mergeWrongPos) {
    EntryMerger merger(*_defaultMs, &_mergedHeader, &*_snvs.begin(), &*_snvs.end());
    Entry wrongPos(&_headers[2], "20\t14371\tid1\tG\tA\t29\t.\t.\t");
    Entry e[] = { _snvs[0], wrongPos };
    ASSERT_THROW(EntryMerger(*_defaultMs, &_mergedHeader, e, e+2), runtime_error);

    Entry wrongChrom(&_headers[2], "21\t14370\tid1\tG\tA\t29\t.\t.\t");
    e[1] = wrongChrom;
    EntryMerger merger2(*_defaultMs, &_mergedHeader, e, e+2);
    ASSERT_FALSE(merger2.merged());
}

// Test merging when only 1 entry has a valid quality. The score should be preserved
TEST_F(TestVcfEntryMerger, singleQual) {
    EntryMerger merger(*_defaultMs, &_mergedHeader, &*_snvs.begin(), &*(_snvs.begin()+1));
    ASSERT_FALSE(merger.merged());
    ASSERT_THROW(Entry(std::move(merger)), runtime_error);
}

TEST_F(TestVcfEntryMerger, mergeAlleles) {
    EntryMerger merger(*_defaultMs, &_mergedHeader, &*_indels.begin(), &*_indels.end());
    Entry e(std::move(merger));
    ASSERT_EQ(2u, e.alt().size());
    ASSERT_EQ("TAG", e.alt()[0]);
    ASSERT_EQ("T", e.alt()[1]);
}

TEST_F(TestVcfEntryMerger, stripFilters) {
    string t1="20\t14370\tid1\tT\tG\t.\tq10\tVC=Samtools\tDP\t1\t3";
    string t2="20\t14370\tid1\tT\tC\t29\ts50\tVC=Samtools\tGT:GQ:DP:HQ\t0|1:48:1:51,51\t1|0:48:8:51,51";
    Entry entries[2];
    Entry::parseLine(&_headers[0], t1, entries[0]);
    Entry::parseLine(&_headers[1], t2, entries[1]);
    EntryMerger merger(*_defaultMs, &_mergedHeader, entries, entries+2);
    Entry merged(std::move(merger));
    ASSERT_EQ(2u, merged.failedFilters().size());

    MergeStrategy ms2(*_defaultMs);
    ms2.clearFilters(true);
    EntryMerger merger2(ms2, &_mergedHeader, entries, entries+2);
    Entry e(std::move(merger2));
    ASSERT_TRUE(e.failedFilters().empty());
    ASSERT_EQ(Entry::MISSING_QUALITY, merged.qual());
}

TEST_F(TestVcfEntryMerger, GTfieldAlwaysFirst) {
    string t1="20\t14370\tid1\tT\tG\t.\tPASS\tVC=Samtools\tDP\t1\t3";
    string t2="20\t14370\tid1\tT\tC\t29\tPASS\tVC=Samtools\tGT:GQ:DP:HQ\t0|1:48:1:51,51\t1|0:48:8:51,51";
    Entry entries[2];
    Entry::parseLine(&_headers[0], t1, entries[0]);
    Entry::parseLine(&_headers[1], t2, entries[1]);
    EntryMerger merger(*_defaultMs, &_mergedHeader, entries, entries+2);
    Entry merged(std::move(merger));
    ASSERT_EQ(4u, merged.sampleData().format().size());
    ASSERT_EQ("GT", merged.sampleData().format()[0]->id());
    ASSERT_EQ(Entry::MISSING_QUALITY, merged.qual());
}

// Let's make sure the builder correctly reheaders Entry objects that are not merged.
// That is, that the samples show up in the right output column.
TEST_F(TestVcfEntryMerger, Builder) {
    vector<Entry> v;
    Builder builder(*_defaultMs, &_mergedHeader, boost::bind(&push_back, ref(v), _1));
    string t1="20\t14370\tid1\tT\tG\t.\tPASS\tVC=Samtools\tDP\t1";
    string t2="21\t14370\tid1\tT\tC\t29\tPASS\tVC=Samtools\tDP\t2";
    Entry entries[2];
    Entry::parseLine(&_headers[0], t1, entries[0]);
    Entry::parseLine(&_headers[1], t2, entries[1]);
    builder(*entries);
    builder(*(entries+1));
    builder.flush();

    ASSERT_EQ(2u, v.size());
    ASSERT_EQ(&_mergedHeader, &v[0].header());
    ASSERT_EQ(&_mergedHeader, &v[1].header());
    ASSERT_EQ(1u, v[0].sampleData().samplesWithData());
    ASSERT_EQ(1u, v[1].sampleData().samplesWithData());
    // Our test headers specified 3 samples each, so entry 1 should have
    // a sample in the first position, entry 2 should have one in the third.
    // FIXME the test entries seem to be supplying 6 samples not 3
    ASSERT_EQ(
        "20\t14370\tid1\tT\tG\t.\tPASS\tVC=Samtools\tDP\t1\t.\t.\t.\t.\t.",
        v[0].toString()
        );
    ASSERT_EQ(
        "21\t14370\tid1\tT\tC\t29\tPASS\tVC=Samtools\tDP\t.\t.\t2\t.\t.\t.",
        v[1].toString()
        );
}

TEST_F(TestVcfEntryMerger, insertion) {
    vector<Entry> ents;

    ents.push_back(makeEntry("1", 39, "T", "TCG"));
    ents.push_back(makeEntry("1", 40, "C", "CGC"));
    ents.push_back(makeEntry("1", 41, "G", "GCG"));
    ents.push_back(makeEntry("1", 41, "G", "C"));
    ents.push_back(makeEntry("1", 41, "GGG", "C"));

    for (auto i = ents.begin(); i != ents.end(); ++i) {
        cout << i->start() << "-" << i->stop() << ": " << *i << "\n";
        if (i > ents.begin()) {
            cout << ", can merge with prev: " << _defaultMs->canMerge(*i, *(i-1)) << "\n";
        }
    }
}

TEST_F(TestVcfEntryMerger, nullAlt) {
    vector<Entry> v;
    Builder builder(*_defaultMs, &_mergedHeader, boost::bind(&push_back, ref(v), _1));
    string t1="20\t14370\tid1\tT\tG\t.\tPASS\tVC=Samtools\tDP\t1";
    string t2="20\t14370\tid1\tT\t.\t29\tPASS\tVC=Samtools\tDP\t2";
    Entry entries[2];
    Entry::parseLine(&_headers[0], t1, entries[0]);
    Entry::parseLine(&_headers[1], t2, entries[1]);
    builder(*entries);
    builder(*(entries+1));
    builder.flush();


    ASSERT_EQ(1u, v.size());
    ASSERT_EQ(&_mergedHeader, &v[0].header());
}
