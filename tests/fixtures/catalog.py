CASES = {
    "static_function": {
        "a.cpp": "static int helper() { return 1; }\nint a() { return helper(); }\n",
        "b.cpp": "static int helper() { return 2; }\nint b() { return helper(); }\n",
    },
    "anonymous_type": {
        "a.cpp": "namespace { struct Local { int a; }; }\nint a() { return sizeof(Local); }\n",
        "b.cpp": "namespace { struct Local { double b; }; }\nint b() { return sizeof(Local); }\n",
    },
    "macro_leak": {
        "a.cpp": "#define DOCTOR_FLAG 1\nint a() { return DOCTOR_FLAG; }\n",
        "b.cpp": "#if DOCTOR_FLAG\n#error macro_leak\n#endif\nint b() { return 2; }\n",
    },
    "header_definition": {
        "bad.h": "struct HeaderValue { int value; };\n",
        "a.cpp": '#include "bad.h"\nint a() { return sizeof(HeaderValue); }\n',
        "b.cpp": '#include "bad.h"\nint b() { return sizeof(HeaderValue); }\n',
    },
    "global_function": {
        "a.cpp": "int duplicated() { return 1; }\nint a() { return duplicated(); }\n",
        "b.cpp": "int duplicated() { return 2; }\nint b() { return duplicated(); }\n",
    },
    "global_variable": {
        "a.cpp": "int duplicated_value = 1;\nint a() { return duplicated_value; }\n",
        "b.cpp": "int duplicated_value = 2;\nint b() { return duplicated_value; }\n",
    },
    "conflicting_alias": {
        "a.cpp": "using LocalValue = int;\nint a() { return sizeof(LocalValue); }\n",
        "b.cpp": "using LocalValue = double;\nint b() { return sizeof(LocalValue); }\n",
    },
    "enum_value": {
        "a.cpp": "enum { local_value = 1 };\nint a() { return local_value; }\n",
        "b.cpp": "enum { local_value = 2 };\nint b() { return local_value; }\n",
    },
}
