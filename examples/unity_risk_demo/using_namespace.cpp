namespace demo_api {

int answer() {
    return 42;
}

}  // namespace demo_api

using namespace demo_api;

int usingNamespaceValue() {
    return answer();
}
