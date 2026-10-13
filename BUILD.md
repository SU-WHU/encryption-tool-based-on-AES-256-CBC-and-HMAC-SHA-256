# 命令行构建

在本目录打开终端。使用 `Makefile`，不要指定 IDE 自动生成的 `Makefile.win`。

## 环境要求

- GNU Make（命令可能叫 `make` 或 `mingw32-make`）。Microsoft `nmake` 不适用。
- 支持 C11 的 GCC。项目是 C 语言，不需要使用 g++。
- 与 GCC 架构、ABI 和运行库兼容的 OpenSSL 开发包：头文件和 libcrypto 链接库。只有 openssl.exe 不够。当前代码已在 MinGW-w64 UCRT64 + OpenSSL 3.0 上验证。
- gcc 和 make 所在目录应在 PATH 中，GCC 随附的辅助程序和 DLL 也应可被找到。

## OpenSSL 位于默认搜索路径

```powershell
mingw32-make -j4
```

若命令名是 `make`，将本文所有 `mingw32-make` 替换为 `make`。
Makefile 使用 GCC 默认搜索路径和 `-lcrypto`，不要求安装 pkg-config。若 GCC 找不到 OpenSSL，请按下文指定其路径。

## OpenSSL 位于自定义目录

假设目录中有 `include/openssl` 和 `lib`：

```powershell
mingw32-make -j4 "OPENSSL_PREFIX=D:/Libraries/OpenSSL"
```

如果库文件在 `lib64`，则额外指定库目录：

```powershell
mingw32-make -j4 "OPENSSL_PREFIX=D:/Libraries/OpenSSL" "OPENSSL_LIBDIR=D:/Libraries/OpenSSL/lib64"
```

路径有空格时，仍把完整的 `变量=值` 放在一对引号中；变量中的目录建议使用 `/`。
当前电脑的可用配置示例（仅说明文档包含本机路径，Makefile 没有写死它）：

```powershell
$env:PATH = "E:\application\Dev-Cpp\msys64\ucrt64\bin;" + $env:PATH
mingw32-make -j4 "OPENSSL_PREFIX=E:/application/Dev-Cpp/OpenSSL-3.0.22-ucrt64" "OPENSSL_LIBDIR=E:/application/Dev-Cpp/OpenSSL-3.0.22-ucrt64/lib64"
```

可以通过命令行指定其他编译器，例如 `"CC=D:/Tools/mingw64/bin/gcc.exe"`。同时确保该工具链的 bin 目录位于 PATH。
特殊 OpenSSL 包可以设置 `OPENSSL_CFLAGS`、`OPENSSL_LIBS`、`SYSTEM_LIBS` 或 `LDLIBS`；静态库所需的额外依赖应遵循该包的构建说明。默认未要求完全静态链接。

## 运行

```powershell
.\coursework.exe encrypt "test.txt" "test.enc"
.\coursework.exe decrypt "test.enc" "restored.txt"
```

两次输入相同口令。输出文件必须不存在；解密输出后缀必须与原始输入后缀一致。
如果链接到 OpenSSL DLL，需要将匹配的 DLL 所在目录加入 PATH，或将所需 DLL 随程序放置；不要从不同工具链或版本混用 DLL。

## 清理与迁移

```powershell
mingw32-make clean
```

清理只删除本 Makefile 生成的 `.portable.o`、`.portable.d` 和 `coursework.exe`，不删除源文件、IDE 的 `.o` 或用户数据。
修改头文件后会自动重编译相关模块。切换编译器、OpenSSL 路径、架构或构建参数时，先 clean 再构建。
迁移时携带全部 `.c`、`.h`、`Makefile` 和本说明；在目标电脑重新编译。

Dev-C++ 的自动构建不会因此自动改用本 Makefile；若希望 IDE 也使用它，需要在项目设置中选用自定义 Makefile。
