#define LOCAL_ONLY 1
#undef LOCAL_ONLY

void localScope() {
    using namespace inner;
    static int cache = 0;
}

class Holder {
    static int cache;
};
