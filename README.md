### TCP 实现 - CS144 实验

## 项目介绍

项目的详细说明可在 [CS144](https://cs144.github.io/) 课程官网的 **Lab Assignment** 部分找到，那里有每个实验的 PDF 介绍文档。

这个项目通过分步骤实现 TCP 协议，涵盖了 TCP 字节流、流重组、TCP 接收器、TCP 发送器和 TCP 连接等模块的实现。

---

## 目录树

```
sponge
├── apps
├── build      代码构建
├── CMakeLists.txt
├── compile_commands.json -> build/compile_commands.json
├── doctests
├── etc
├── libsponge  源代码
├── README.md
├── tests
├── tun.sh
├── txrx.sh
└── writeups
```

---

## 环境搭建


推荐安装 **CS144 VirtualBox**，并使用 **Ubuntu 20.04** 作为实验环境。

详细的安装步骤可以在 [VM Installation Guide](https://stanford.edu/class/cs144/vm_howto/vm-howto-image.html) 上找到，按照步骤操作即可配置好本实验的环境。

---

## 项目框架代码拉取

在官方每个实验的 PDF 介绍中，都会有一个 **Getting started** 部分，列出了拉取项目代码的具体步骤。按照这些步骤，你可以获取到每个实验的代码。

---

## 项目运行和测试的相关命令

测试文件位于 `sponge/tests/` 文件夹下，对应的 CMake 命令位于 `sponge/etc/test.cmake` 中。

在构建时使用以下命令运行不同实验的测试：

```bash
make check_lab1
make check_lab2 
make check_lab3 
make check_lab4
```

最终，可以使用以下命令对整个项目进行性能测试：

```bash
./apps/tcp_benchmark
```

---

## 项目组成和结果展示

项目分为五个主要部分：

- **字节流（ByteStream）**：负责对传输的数据进行读写操作，并支持流量控制。
- **流重组器（StreamReassembler）**：处理 TCP 传输过程中可能出现的乱序、丢失、重复和交叉重叠等情况，确保数据按正确顺序重组。
- **TCP 接收器（TCPReceiver）**：将接收到的数据送入流重组器处理，并传入字节流。同时返回确认号（ACK）和窗口大小（Window Size），实现流量控制。
- **TCP 发送器（TCPSender）**：实现重传定时器，用于 TCP 超时重传，并对传输信息进行加工，生成 TCP 段进行发送。
- **TCP 连接（TCPConnection）**：将 TCP 接收器和发送器封装在一起，实现完整的收发数据功能。

基于以上五个部分，最终可以实现一个 TCP 协议栈。

### 压力测试

```bash
$ ./apps/tcp_benchmark 
#----------------------------------------------------------------------------------
CPU-limited throughput                : 1.00 Gbit/s
CPU-limited throughput with reordering: 0.94 Gbit/s
```

---

## 常见问题

### 1. 字节流细节

字节流内部使用一个 `queue<char> _buff` 实现，有一个容量 `capacity`，可以用于查看缓冲区是否已满，并且可以查看读写的字节数和是否到达数据末尾。当缓冲区为空且输入结束时，标记为 `eof`。

### 2. 流重组器细节

传输的字符串必须先写入字节流中，然后再由接收方读取。已写入的数据都是按正确顺序排列的。难点在于处理乱序、重叠、丢失和重复等情况。

流重组器使用自定义的 `struct segment` 结构，并重载了 `<` 操作符，以便使用 `set` 数据结构进行排序。

### 3. 项目难点

主要难点在于流重组器对写入字符串的处理，特别是如何应对乱序、重叠、丢失和重复等情况。可以重点参考 **Lab 1** 的文档，其中对流重组器的细节有详细描述。

---

## 贡献

欢迎任何形式的贡献！如果你想改进项目或修复 Bug，可以 Fork 本仓库，进行修改，然后提交 Pull Request。请确保你的代码遵循项目的编码风格并通过所有测试。

---

## 许可

本项目基于 MIT 许可协议，详情请参阅 [LICENSE](LICENSE) 文件。

---

## 作者

本项目由 **zzcodd** 实现并维护。如果你有任何问题或建议，欢迎随时联系！

[![GitHub](https://img.shields.io/badge/GitHub-zzcodd-blue?style=flat-square&logo=github)](https://github.com/zzcodd)

---

感谢你查看本项目！祝你编程愉快！🚀
