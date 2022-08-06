# File-loader based on the sender sliding window protocol #
# 基于发送端滑窗协议的文件传输模块 #

## 综述 ##
 本项目实现了一个基于发送端滑窗协议的文件传输模块。支持安装用户配置的传输速率限速上传、下载文件。
 模块内自协商窗口大小，单帧文件块大小为速度的1/4（最大1MB）。支持上传、下载的续传。
 同时，整个socket通信框架是一个基于poll的单线程轻量级架构，满足多用户、多任务在一个线程内顺序执行。
 另外，整个项目文件除了"listnode.h"文件中有引用自linux源码的函数，其它部分全部由本人基于系统API实现。
 
## 结束语 ##   
 **Make a More useful thing**  
 
 
                               
