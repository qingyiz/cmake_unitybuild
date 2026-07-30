# Unity Risk Demo

这是 Unity Build Doctor 的手工验收工程。普通构建可以通过；打开 Unity Build
后，同一批次中的两个 `fileCache` 会产生重定义错误。

## 在界面中验证源码扫描

1. 打开 `dist/UnityBuildDoctor.app`。
2. 项目目录选择本目录 `examples/unity_risk_demo`。
3. 分析模式选择“源码快速扫描（无需构建）”。
4. 点击“开始源码扫描”。

预期三个检查项都显示 `Risk Found`：

- 宏定义检查：两个文件的活动宏，以及 `UNITY_DEMO_LIMIT` 替换内容冲突；
- `using namespace` 检查：`using_namespace.cpp` 的文件作用域声明；
- 文件级 `static` 检查：`static_a.cpp` 与 `static_b.cpp` 的 `fileCache`。

`safe_patterns.cpp` 用于验证排除逻辑：已经 `#undef` 的宏、函数局部
`using namespace`、函数局部 `static`、类静态成员、注释和字符串不应被报告。

## 用 CMake 对照验证

普通构建应成功：

```bash
cmake -S . -B build -G Ninja -DCMAKE_UNITY_BUILD=OFF
cmake --build build
./build/unity_risk_demo
```

Unity Build 应在编译阶段报告 `fileCache` 重定义：

```bash
cmake -S . -B build-unity -G Ninja -DCMAKE_UNITY_BUILD=ON
cmake --build build-unity
```

`build` 与 `build-unity` 会被源码扫描器自动跳过。
