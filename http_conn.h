#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

// 提供对POSIX API的访问功能，提供对系统调用的封装，如fork,pipe,read,write
#include <unistd.h>
// 定义信号处理部分
#include <signal.h>
// 提供基础数据类型，如pid_t,size_t,time_t,clock_t,etc
#include <sys/types.h>
// 提供epoll多路复用I/O
#include <sys/epoll.h>
// 包含open,fcntl,fclose,etc
#include <fcntl.h>
// 包含套接字的定义
#include <sys/socket.h>
// 包含socketaddr_in,htons的定义
#include <netinet/in.h>
// 包含htons(),htonl(),inet_pton()的定义
#include <arpa/inet.h>
// 包含assert宏
#include <assert.h>
// 包含stat()
#include <sys/stat.h>
// C语言的字符串库
#include <string.h>
// 提供线程支持
#include <pthread.h>
// C语言标准I/O
#include <stdio.h>
// standard library，包含malloc(),calloc(),free(),atoi(),exit(),etc
#include <stdlib.h>
// memory management
#include <sys/mman.h>
// standard arguments，为了让函数能够接受可变参数 ...
#include <stdarg.h>
// 定义了通过错误码来回报错误信息的宏
#include <errno.h>
// 自定义的锁
#include "locker.h"

class http_conn{
public:
// 文件名最大长度
static const int FILENAME_LEN = 200;
// 读缓冲区的大小
static const int READ_BUFFER_SIZE = 2048;
// 写缓冲区的大小
static const int WRITE_BUFFER_SIZE = 1024;
// HTTP 的请求方法
// GET：向特定的资源发出请求
// POST：向指定资源提交数据进行处理请求（如：提交表单，上传文件）
// HEAD：类似GET请求，只不过返回的响应中没有具有的内容，只有报头
// PUT：向指定资源位置上传其最新内容
// DELETE：请求服务器删除Request-URI所标识的资源
// TRACE：回显服务器收到的请求，主要用于测试或诊断
// OPTIONS：返回服务器针对特定资源所支持的HTTP请求方法，也可以发送'*'来测试服务器的功能性
// CONNECT：HTTP/1.1协议中预留给能够将连接改为管道方式的代理服务器
enum METHOD{ GET=0, POST, HEAD, PUT, DELETE, TRACE,
					OPTIONS, CONNECT, PATCH };

// 解析客户请求时，主状态机所处的状态
enum CHECK_STATE{ CHECK_STATE_REQUESTLINE = 0,
				  CHECK_STATE_HEADER,
				  CHECK_STATE_CONNECT };

// 行的读取状态
enum LINE_STATUS{ LINE_OK = 0, LINE_BAD, LINE_OPEN };

// 行的读取状态
enum LINE_STATUS{ LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
	http_conn(){}
	~http_conn(){}

public:
	// 初始化新接受的连接
	void init(int sockfd, const sockaddr_in &addr);
	// 关闭连接
	void close_conn(bool read_close = true);
	// 处理客户请求
	void process();
	// 非阻塞读
	bool read();
	// 非阻塞写
	bool write();

private:
	// 空参初始化连接
	void init();
	// 解析HTTP请求
	HTTP_CODE process_read();
	// 填充HTTP应答
	bool process_write(HTTP_CODE ret);

	// 下面这一组函数被process_read调用以分析HTTP请求
	HTTP_CODE parse_request_line(char *text);
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	HTTP_CODE do_request();
	char* get_line(){ return m_read_buf + m_start_line; }
	LINE_STATUS parse_line();
	
	// 下面这一组函数被process_write调用以填充HTTP应答
	void unmap();
	bool add_response(const char *format, ...);
	bool add_content(const char *content);
	bool add_status_line(int status, const char *title);
	bool add_headers(int content_length);
	bool add_content_length(int content_length);
	bool add_linger();
	bool add_blank_line();

public:
	// 所有socket上的事件都被注册到同一个epoll内核时间表中，所以将epoll fd描述为静态的
	static int m_epollfd;
	// 统计用户数量
	static int m_user_count;

private:
	// 该HTTP连接的socket和对方的socket地址
	int m_sockfd;
	sockaddr_in m_address;

	// 读缓冲区
	char m_read_buf[READ_BUFFER_SIZE];
	// 已经读入的客户数据最后一个字节的下一个位置
	int m_read_idx;
	// 当前正在分析的字符在缓冲区的位置
	int m_checked_idx;
	// 当前正在解析的行的起始位置
	int m_start_line;
	// 写缓冲区
	char m_write_buf[WRITE_BUFFER_SIZE];
	// 写缓冲区中待发送的字节
	int m_write_idx;
	
	// 主状态机的当前状态
	CHECK_STATE m_check_state;
	// 请求方法
	METHOD m_method;

	// 客户请求的文件的完整路径，内容为：doc_root(网站根目录） + m_url
	char m_real_file[FILENAME_LEN];
	// 客户请求的目标文件的文件名
	char *m_url;
	// HTTP协议版本号，这里仅支持HTTP/1.1
	char *version;
	// 主机名
	char *m_host;
	// HTTP请求的消息体长度
	int m_content_length;
	// HTTP请求是否要求保持连接
	bool m_linger;

	// 客户请求的目标文件被mmap到内存中的起始位置
	char *m_file_address;

	// 目标文件的状态，通过它判断文件是否存在，是否为目录，是否可读，文件大小等
	struct stat m_file_stat;
	// 采用 writev() 聚集写，收集内存中分散的若干缓冲区，写至文件的连续区域中
	/*
	   struct iovec{
			ptr_t iov_base; // starting address
			size_t iov_len; // length in bytes
	   }

	*/
	struct iovec m_iv[2];
	// 表示被写内存块的数量
	int m_iv_count;
};

#endif






