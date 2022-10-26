# TinyWebServer

本项目为epoll多路复用实现的webserver, 使用到的技术有

- epoll多路复用
- 定时器
- 线程池
- http报文解析



## epoll多路复用



## 线程池



## 定时器

使用`setitimer`让主线程每隔`interval`秒后就收到`SIGALRM`信号, 并使用`sigaction`设置信号处理函数.

为了使信号函数可以重入(Reentrancy), 在信号处理函数中向管道的写端写数据, 将管道的读端加入epoll中监测. 当监测到管道读端有`EPOLLIN`事件时说明有信号, 在主线程中处理信号. 

注意: 由于信号`epoll_wait`会返回-1且errno被设置为`EINTR`, 此时继续执行即可
