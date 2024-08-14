## Networking by hand

这部分主要是一些命令行的操作。

### Fetch a Web page

1. 在浏览器中打开 `http://cs144.keithw.org/hello`，会看到 `Hello, CS144!`

2. 命令行中运行 `telnet cs144.keithw.org http`，会看到以下内容：

    ```bash
    $ telnet cs144.keithw.org http
    Trying 104.196.238.229...
    Connected to cs144.keithw.org.
    Escape character is '^]'.
    ```

    *可以按 `ctrl+]`，再输入 close 和回车退出。*

    接着根据描述输入请求：

    ```bash
    $ telnet cs144.keithw.org http
    Trying 104.196.238.229...
    Connected to cs144.keithw.org.
    Escape character is '^]'.
    # type a request:
    GET /hello HTTP/1.1
    Host: cs144.keithw.org
    Connection: close
    
    # There is a blank line above!
    
    # response:
    HTTP/1.1 200 OK
    Date: Wed, 12 Oct 2022 03:15:51 GMT
    Server: Apache
    Last-Modified: Thu, 13 Dec 2018 15:45:29 GMT
    ETag: "e-57ce93446cb64"
    Accept-Ranges: bytes
    Content-Length: 14
    Connection: close
    Content-Type: text/plain
    
    Hello, CS144!
    Connection closed by foreign host.
    ```

    留意这里的 `ETag`：

    > Etag 是 Entity tag 的缩写，可以理解为“被请求变量的实体值”，Etag 是服务端的一个资源的标识，在 HTTP 响应头中将其传送到客户端。服务器单独负责判断记号是什么及其含义，并在 HTTP 响应头中将其传送到客户端。比如，浏览器第一次请求一个资源的时候，服务端给予返回，并且返回了 `ETag: "50b1c1d4f775c61:df3"` 这样的字样给浏览器，当浏览器再次请求这个资源的时候，浏览器会将 `If-None-Match: W/"50b1c1d4f775c61:df3"` 传输给服务端，服务端拿到该 ETAG，对比资源是否发生变化，如果资源未发生改变，则返回 304 状态码，不返回具体的资源。

3. 根据要求发送 SUNet ID 来获取 secret code。因为并没有真实的 ID，所以这里用随机的数字替代。

    ```
    $ telnet cs144.keithw.org http
    Trying 104.196.238.229...
    Connected to cs144.keithw.org.
    Escape character is '^]'.
    # type a request:
    GET /lab0/1234 HTTP/1.1
    Host: cs144.keithw.org
    Connetction: close
    
    # There is a blank line above!
    
    # response:
    HTTP/1.1 200 OK
    Date: Wed, 12 Oct 2022 03:20:59 GMT
    Server: Apache
    X-You-Said-Your-SunetID-Was: 1234
    X-Your-Code-Is: 147679
    Content-length: 108
    Vary: Accept-Encoding
    Content-Type: text/plain
    
    Hello! You told us that your SUNet ID was "1234". Please see the HTTP headers (above) for your secret code.
    Connection closed by foreign host.
    ```

    关于这里的 `X-You...` 和 `X-Your...` 都是非标准用法，具体可以看 [stackoverflow](https://stackoverflow.com/questions/1810915/is-safe-to-use-x-header-in-a-http-response) 上的回答。

### Send yourself an email

这部分需要真实的 SUNet ID，所以跳过。

### Listening and connecting

```
# terminal 1
$ netcat -v -l -p 9090
Listening on 0.0.0.0 9090
Connection received on localhost 52578
hello, cs144!
^C
# terminal 2
$ telnet localhost 9090
Trying ::1...
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
hello, cs144!
Connection closed by foreign host.
```

## Writing a network program using an OS stream socket

先把 [sponge](https://github.com/cs144/sponge) 这个项目克隆到本地。

### Writing webget

一些注意事项：

1. HTTP 请求中**换行**必须使用 `\r\n` 而不能只使用 `\n`。
2. 同时这里需要包含 `Connection: close`，表示远程服务器处理完当前请求后立即关闭。
3. 注意代码片段中的 `shutdown(SHUT_WR)` 的用法，虽然没有这行代码也能通过测试。想要了解 `shutdown` 与 `close` 的区别可以查看《Linux 高性能服务器编程》5.7 节的内容：简单来讲 `close` 只能同时关闭 socket 的读写，但是 `shutdown` 可以分别关闭。
4. 要求把所有的输出都打印出来，所以需要利用循环 `while(!sock.eof())` 来多次调用 `read()`。

## An in-memory reliable byte stream

我们需要在内存中实现一个可读、可写，支持流量控制的有序字节流。

注意事项：

1. `write()` 中传入的 `data` 的长度是可以大于缓冲区的容量，但是超过剩余空间大小的部分会被丢弃。
2. 留意 `eof()` 成立的条件：需要 writer 停止写入，同时 reader 读取完缓冲区中的全部内容。