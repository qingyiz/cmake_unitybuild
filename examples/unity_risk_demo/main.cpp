#include <iostream>

int macroValueA();
int macroValueB();
int safePatternsValue();
int staticValueA();
int staticValueB();
int usingNamespaceValue();

int main() {
    std::cout
        << "macro=" << macroValueA() + macroValueB()
        << ", static=" << staticValueA() + staticValueB()
        << ", using=" << usingNamespaceValue()
        << ", safe=" << safePatternsValue()
        << '\n';
    return 0;
}
