# TCP 实现 - CS144 实验

## 项目介绍

这是我在CS144课程中从零开始实现的一个简化版TCP协议。这个项目的重点在于构建一个最小化的TCP协议栈，处理基本的TCP功能，包括连接建立、数据传输和连接终止。本文档将详细介绍项目的结构、主要功能，以及如何编译和测试这个实现。

---

## 目录结构

```
/project_root
│
├── include/               # 头文件
│   ├── tcp_sender.hh      # TCP发送方接口定义
│   ├── tcp_receiver.hh    # TCP接收方接口定义
│   ├── tcp_connection.hh  # TCP连接接口定义
│   └── ...                # 其他头文件
│
├── src/                   # 源代码文件
│   ├── tcp_sender.cc      # TCP发送方实现
│   ├── tcp_receiver.cc    # TCP接收方实现
│   ├── tcp_connection.cc  # TCP连接实现
│   └── ...                # 其他源代码文件
│
├── tests/                 # 单元测试
│   ├── tcp_sender_test.cc # TCP发送方测试
│   ├── tcp_receiver_test.cc # TCP接收方测试
│   ├── tcp_connection_test.cc # TCP连接测试
│   └── ...                # 其他测试文件
│
└── Makefile               # 编译配置文件
```

---

## 环境搭建

推荐使用 **CS144 VirtualBox**，并选择 **Ubuntu 20.04** 作为实验环境。

详细的安装步骤可以在 [VM Installation Guide](https://stanford.edu/class/cs144/vm_howto/vm-howto-image.html) 中找到。按照指南操作即可完成实验环境的搭建。

---

## 获取项目代码

在CS144官方提供的每个实验PDF中都有 **Getting started** 部分，详细列出了获取项目代码的步骤。按照这些步骤操作，你可以拉取到每个实验的框架代码。

---

## 项目编译与测试

测试文件位于 `sponge/tests/` 目录下，对应的 CMake 命令在 `sponge/etc/test.cmake` 中定义。

你可以使用以下命令来构建并运行不同实验的测试：

```bash
make check_lab1
make check_lab2 
make check_lab3 
make check_lab4
```

在完成所有实验后，还可以使用以下命令对整个项目进行性能测试：

```bash
./apps/tcp_benchmark
```

---

## 项目组成与实现展示

项目分为以下五个主要模块：

- **字节流（ByteStream）**：管理数据的读写操作，支持流量控制。
- **流重组器（StreamReassembler）**：处理TCP数据包的乱序、丢失、重复和交叉重叠，确保数据按正确顺序重组。
- **TCP 接收器（TCPReceiver）**：负责接收数据并送入流重组器，同时返回ACK和窗口大小，实现流量控制。
- **TCP 发送器（TCPSender）**：实现重传定时器，处理超时重传，并对数据进行分片和发送。
- **TCP 连接（TCPConnection）**：整合TCP发送器和接收器，实现完整的TCP连接和数据收发。

### 性能测试结果

```bash
$ ./apps/tcp_benchmark 
#----------------------------------------------------------------------------------
CPU-limited throughput                : 1.00 Gbit/s
CPU-limited throughput with reordering: 0.94 Gbit/s
```

---

## 常见问题与解决方案

### 1. 字节流模块

字节流模块使用了 `queue<char> _buff` 来实现，具有容量限制 `capacity`，可用于监控缓冲区的使用情况。缓冲区为空且输入结束时，会标记为 `eof`。

### 2. 流重组器模块

流重组器负责处理TCP传输中的乱序、重叠、丢失和重复等情况。传输的数据必须先写入字节流，然后由接收方读取。流重组器使用了自定义的 `struct segment` 结构，并重载了 `<` 操作符，以便使用 `set` 数据结构进行排序。

### 3. 项目难点

项目的主要难点在于如何在流重组器中处理乱序、重叠、丢失和重复的数据。建议重点参考 **Lab 1** 的文档，那里详细描述了流重组器的实现细节。

---

## 贡献说明

非常欢迎大家的贡献！如果你有改进建议或发现了Bug，请Fork本仓库并提交Pull Request。在提交之前，请确保你的代码符合项目的编码规范，并通过所有测试。

---

## 项目作者

本项目由 **zzcodd** 开发和维护。如果你有任何问题或建议，欢迎通过以下方式联系我：

[![GitHub](https://img.shields.io/badge/GitHub-zzcodd-blue?style=flat-square&logo=github)](https://github.com/zzcodd)

---

感谢你对本项目的关注！祝你在编程旅程中不断进步！🚀

---

希望这个版本的README更符合你作为项目参与者的风格。如果还有其他需要调整的地方，请随时告诉我！
