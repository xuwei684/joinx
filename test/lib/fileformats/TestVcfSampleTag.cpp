#include "fileformats/vcf/SampleTag.hpp"

#include <sstream>
#include <stdexcept>
#include <gtest/gtest.h>

using namespace Vcf;
using namespace std;

TEST(TestVcfSampleTag, parse) {
    string tests[] = {
        "ping=<x>",
        "pong=<x,y,z>,frig=\"frog\"",
        "pong=<z,y,x>",
        "frog=hello",
        "cheese=\"bucket\"",
    };
    int n = sizeof(tests)/sizeof(tests[0]);
    for (int i = 0; i < n; ++i) {
        cout << "String: '" << tests[i] << "'\n";
        SampleTag st(tests[i]);
        cout << "Got: [";
        st.toStream(cout);
        cout << "]\n---\n";
    }

    // actual tests...
    ASSERT_EQ(SampleTag("x=y").toString(), "##SAMPLE=<x=y>");
    ASSERT_EQ(SampleTag("x=<1,2,3>").toString(), "##SAMPLE=<x=<1,2,3>>");
    ASSERT_EQ(SampleTag("x=\"y\"").toString(), "##SAMPLE=<x=\"y\">");
    ASSERT_EQ(SampleTag("x=<1,\"twenty point one\",four>").toString(),
        "##SAMPLE=<x=<1,\"twenty point one\",four>>");
}
