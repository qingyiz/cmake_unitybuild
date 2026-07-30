#include <string>

#define TEMPORARY_DEMO_VALUE 3

int safePatternsValue() {
    using namespace std;
    static int localCache = TEMPORARY_DEMO_VALUE;
    return localCache;
}

#undef TEMPORARY_DEMO_VALUE

class DemoHolder {
    static int classCache;
};

// using namespace ignored_comment;
const char* ignoredText = "static int ignoredString;";
