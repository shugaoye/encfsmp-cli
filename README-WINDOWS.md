# EncFS MP CLI - Windows 构建指南

## 方式一：使用 GitHub Actions 自动构建（推荐）

### 步骤

1. **Fork 本仓库** 到你的 GitHub 账号

2. **推送代码触发构建**
   ```bash
   git add .
   git commit -m "触发 Windows 构建"
   git push origin main
   ```

3. **下载构建产物**
   - 进入你的 GitHub 仓库页面
   - 点击 **Actions** 标签
   - 点击最新的 workflow 运行
   - 在 Artifacts 部分下载 **encfsmp-cli-windows-x64.zip**

4. **解压并使用**
   - 解压 zip 文件
   - 打开命令提示符 (cmd) 或 PowerShell
   - 运行 `encfsmp_cli.exe --help`

---

## 方式二：在 Windows 本地构建

### 前置要求

1. **安装 MSYS2**
   - 下载地址：https://www.msys2.org/
   - 安装完成后，打开 **MSYS2 MinGW 64-bit** 终端

2. **安装编译工具链**
   ```bash
   pacman -S mingw-w64-x86_64-toolchain
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-boost
   pacman -S mingw-w64-x86_64-openssl
   ```

3. **克隆源代码**
   ```bash
   git clone <本仓库地址>
   cd encfsmp-cli
   ```

### 编译

**方法 A：使用批处理脚本（简单）**
```bash
# 在 MSYS2 MinGW 64-bit 终端中运行
./build-windows.bat
```

**方法 B：手动编译**
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j$(nproc)
```

### 输出

构建完成后：
- `dist/encfsmp-cli-win-x64.zip` - 完整打包（推荐）
- `package/bin/encfsmp_cli.exe` - 可执行文件

---

## 使用方法

### 基本命令

```cmd
# 创建加密卷
encfsmp_cli.exe create C:\vaults\myvault -p mypassword

# 添加文件
encfsmp_cli.exe put C:\vaults\myvault C:\docs\secret.txt secret.txt -p mypassword

# 查看内容
encfsmp_cli.exe list C:\vaults\myvault -p mypassword

# 提取文件
encfsmp_cli.exe get C:\vaults\myvault secret.txt C:\temp\restored.txt -p mypassword

# 终端查看（不写入磁盘）
encfsmp_cli.exe cat C:\vaults\myvault secret.txt -p mypassword
```

### 完整命令列表

| 命令 | 说明 |
|------|------|
| `create <dir> [-p password]` | 创建新加密卷 |
| `info <dir> [-p password]` | 显示卷信息 |
| `list <dir> [path] [-p pwd]` | 列出文件 |
| `cat <dir> <path> [-p pwd]` | 解密输出到终端 |
| `put <dir> <src> <dst> [-p pwd]` | 写入/加密文件 |
| `get <dir> <src> <dst> [-p pwd]` | 读取/解密文件 |
| `mkdir <dir> <path> [-p pwd]` | 创建目录 |
| `rm <dir> <path> [-p pwd]` | 删除文件/目录 |

---

## 注意事项

### 路径格式

- Windows 路径使用反斜杠 `\` 或正斜杠 `/` 都可以
- 示例：
  ```cmd
  encfsmp_cli.exe create C:\vault\myvault -p pwd   (OK)
  encfsmp_cli.exe create C:/vault/myvault -p pwd    (也 OK)
  ```

### 依赖

- 静态链接版本，不需要额外安装运行时库
- 如遇问题，确保使用的是 **MSYS2 MinGW 64-bit** 终端

### 密码安全

- `-p` 参数会在命令行中暴露密码
- 生产环境建议不使用 `-p`，程序会安全地提示输入密码
- 示例：
  ```cmd
  encfsmp_cli.exe create vault
  # 然后按提示输入密码
  ```

---

## 故障排除

### 问题：提示 "不是内部或外部命令"

**解决方案**：
1. 使用完整路径运行
2. 或将 `encfsmp_cli.exe` 所在目录添加到 PATH

```cmd
set PATH=%PATH%;C:\path\to\encfsmp-cli\package\bin
```

### 问题：MSYS2 编译报错

**解决方案**：
1. 确保打开的是 **MSYS2 MinGW 64-bit** 终端，不是 MSYS2 MSYS
2. 确保所有包安装到 mingw64 系统
   ```bash
   pacman -Ss mingw-w64-x86_64-boost | head -20
   ```

### 问题：运行时缺少 DLL

**解决方案**：
1. 下载完整 zip 包
2. 确保解压到干净目录，不与其他软件混用

---

## 技术细节

- **编译器**: MinGW-w64 GCC
- **加密**: AES-256 (OpenSSL)
- **文件名加密**: nameio/block (IV chaining)
- **链接方式**: 静态链接 (无外部依赖)
