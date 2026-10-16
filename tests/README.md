# 自动化白盒与端到端测试

在 `Individual Coursework` 目录运行。脚本每次重新编译当前源码，不使用目录里已有的 exe/o，也不修改业务源码。仅依赖 Python 3 标准库，另需 GCC、OpenSSL 开发头文件/库和 OpenSSL 命令行工具。

通用环境（依赖均已在搜索路径中）：

```sh
python tests/whitebox.py
```

当前 Windows 电脑可用的完整命令（PowerShell 单行）：

```powershell
python tests/whitebox.py --cc "E:/application/Dev-Cpp/msys64/ucrt64/bin/gcc.exe" --openssl-prefix "E:/application/Dev-Cpp/OpenSSL-3.0.22-ucrt64" --openssl-libdir "E:/application/Dev-Cpp/OpenSSL-3.0.22-ucrt64/lib64" --openssl "D:/anaconda3/Library/bin/openssl.exe" --large --keep
```

这些安装路径只出现在说明中，脚本没有绑定本机路径。默认寻找 `gcc`、`openssl`；`--openssl-prefix` 默认查找其 `lib` 子目录，不存在时使用 `lib64`，也可以明确指定 `--openssl-libdir`。

## 覆盖内容

- 直接调用 C 模块：AES-256 已知答案、HMAC-SHA256 已知答案、CBC 0–16 字节及逐字节填充、非法长度和空指针、认证标签每个字节被修改、KDF 参数检查、输入模块参数检查。
- 文件长度 0、1、15、16、17、31、32、33、4095、4096、4097 的往返验证，包含全部字节值。
- 使用 Python `hashlib` 独立派生密钥和计算 HMAC；用 OpenSSL CLI 独立解密程序输出，并构造输入供程序解密。避免只依赖自身往返验证。
- 文件头每类字段、密文、标签篡改；截断、追加、固定种子的畸形输入；正确 HMAC 但错误填充，以及无密文的认证文件。
- 错口令、空口令、EOF、200/201 字节口令、错误命令行参数。
- 无后缀、多重后缀（项目只记录最后一个后缀）、62/63 字节后缀、输出后缀不匹配。
- 已存在输出、相同输入输出路径、缺失输入与输出目录；拒绝后不得创建输出或修改输入。
- 多次加密检查 salt/IV 不重复。这只是基本检查，不是随机数质量证明。
- `--large` 增加超过加密/解密大小上限的测试，可能占用数百 MiB 内存和磁盘空间。

## 结果与选项

- 控制台逐项显示 `PASS` / `FAIL`。
- 默认 JSON 报告是 `tests/last-report.json`，`--report PATH` 可指定其他位置。
- 全部通过时脚本退出码为 0；存在失败、编译错误或超时则为 1。
- CLI 的退出码与功能分别检查。如果程序失败返回 0，功能拒绝检查仍会继续，但最后的退出码检查必定 FAIL，整个脚本也返回 1。
- C 模块逐项结果与模块进程状态都记录，因此报告包含单项检查以及套件级检查。
- 编译产物和测试文件存放于项目上级的 `output/whitebox/run-*` 新目录；默认自动清理，`--keep` 保留现场和编译日志。仅使用脚本生成的测试口令和测试文件。
- `--timeout 120` 调整每个子进程的超时秒数，默认 60。
- `--sanitize` 启用 AddressSanitizer/UndefinedBehaviorSanitizer；要求 GCC 及运行时支持，推荐在相应 Linux 环境运行。缺少运行时会报告失败，不会静默跳过。

本测试针对当前 v1 文件格式、PBKDF2 600000 次及当前模块返回码。修改协议后，应同步维护预期值。它不声称达到所有分支覆盖，也未自动模拟内存分配失败、磁盘写满、文件竞争、终端回显、权限隔离和侧信道。认证顺序可通过专门的插桩测试进一步核实；这里只检查输入输出行为及直接模块行为。
