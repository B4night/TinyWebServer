# TinyWebServer

本项目为epoll多路复用实现的webserver, 使用到的技术有

- epoll多路复用
- 定时器
- 线程池
- http报文解析



## epoll多路复用

在`epoll_wait`监测的事件返回时, 将要做的任务分配给线程池, 并发执行效率更高.

## 线程池

`struct task`的对象为一个任务, 该结构体的两个成员分别是子线程调用的函数和函数的参数. 使用`std::list`来存储任务列表, 新来的任务在`list`的尾部, 子线程获得执行机会时从`list`头部取出任务并执行.

使用条件变量`not_empty`来表示任务列表`list`非空, 如果为空, 则子线程阻塞在`pthread_cond_wait`上, 直到`add_job`添加任务并调用`pthread_cond_signal`后.

## 定时器

使用`setitimer`让主线程每隔`interval`秒后就收到`SIGALRM`信号, 并使用`sigaction`设置信号处理函数.

为了使信号函数可以重入(Reentrancy), 在信号处理函数中向管道的写端写数据, 将管道的读端加入epoll中监测. 当监测到管道读端有`EPOLLIN`事件时说明有信号, 在主线程中处理信号. 

注意: 由于信号`epoll_wait`会返回-1且errno被设置为`EINTR`, 此时继续执行即可



## usage

在linux下输入下列命令

> g++ *.cpp -o server

或者在linux下使用vscode打开当前文件夹后 `run`即可

在浏览器中输入`localhost:4000`来访问
