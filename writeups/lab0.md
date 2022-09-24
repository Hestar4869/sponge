Lab 0 Writeup
=============

## Writing *webget* 
首先，我们*webget*需要我们模拟浏览器发送http请求，模拟http需要按照http协议书写报文。

http请求一般分为get请求和post请求。请求格式如下：
``` json
<request-line>
<headers>
<blank line>
[<request-body>]
```

1. 第一行必须是一个请求行(request-line),用来说明请求类型,要访问的资源以及所使用的HTTP版本
2. 紧接着是一个请求头(header),用来说明服务器要使用的附加信息
3. 紧接着是一个空行(blank line)
4. 紧接着是请求体(request-body)
   

### get请求格式
```http request
GET /text.html HTTP/1.1
Accept: */*
Accept-Language: zh-cn
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022)
Host: 127.0.0.1
Connection: Keep-Alive


```
1. **第一部分请求行**，GET表示请求方法，`/text.html`表示请求的路径以及资源，
`HTTP/1.1`表示请求的HTTP协议版本，`1.0`表示连接之后立即断开，`1.1`表示资源传输结束后断开。
2. **第二部分请求头**，HOST将指出请求的目的地，User-Agent,服务器端和客户端脚本都能访问它，它是浏览器类型检测逻辑的重要基础。该信息由你的浏览器来定义,并且在每个请求中自动发送，Connection通常将浏览器操作设置为Keep-Alive，而没有响应后立即close这个连接
3. **第三部分请求体**，get请求没有请求体，所以就是单独空一行。

### post请求
```http request
POST /text.html HTTP1.1
Host:www.wrox.com
User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022)
Content-Type:application/x-www-form-urlencoded
Content-Length:40
Connection: Keep-Alive

name=Professional%20Ajax&publisher=Wiley
```

基本同get请求，但是请求体为表单数据
### 实验注意
1. 发送的http请求头，至少包含host和connection字段
2. sock传输结束后，不要忘记主动断开连接

## An in-memory reliable byte stream
```
Please open the libsponge/byte stream.hh and libsponge/byte stream.cc files, and
implement an object that provides this interface. As you develop your byte stream
implemantation,you can run the automated tests by "make check_lab0"
```
唯一需要注意的是eof()函数，eof()函数表示输出的结束，输出结束的条件应该有两个：
1. 当前byte_stream中的缓存为空，即缓冲区没有任何字节
2. 当前输入端已经结束输入，即ended_input()返回true