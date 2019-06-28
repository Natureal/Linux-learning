#include "http_conn.h"

// 定义HTTP相应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n"; 
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get the file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";
const char *doc_root = "/var/www/html";

int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void addfd(int epollfd, int fd, bool one_shot){
	epoll_event event;
	event.data.fd = fd;
	// EPOLLRDHUP: 对方关闭连接，或者关闭写端
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if(one_shot){
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

// 关闭HTTP连接时需要的操作
void removefd(int epollfd, int fd){
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

void modfd(int epollfd, int fd, int ev){
	epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::close_conn(bool read_close = true){
	if(read_close && (m_sockfd != -1)){
		removefd(m_epollfd, m_sockfd);
		m_sockfd = -1;
		m_user_count--;
	}
}

void http_conn::init(int sockfd, const sockaddr_in &addr){
	m_sockfd = sockfd;
	m_address = addr;
	// 如下两行可以避免TIME_WAIT，仅用于测试，实际中应该注释掉
	//int reuse = 1;
	//etsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	addfd(m_epollfd, sockfd, true);
	m_user_count++;
	
	init();
}

void http_conn::init(){
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_linger = false; // 不保持HTTP连接
	
	m_method = GET;
	m_url = 0;
	m_version = 0;
	m_content_length = 0;
	m_host = 0; // 主机名
	m_start_line = 0; // 正在解析的行的起始位置
	m_checked_idx = 0;
	m_read_idx = 0;
	m_write_idx = 0;
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(m_read_file, '\0', FILENAME_LEN);
}

// 从状态机
http_conn::LINE_STATUS http_conn::parse_line(){
	char temp;
	for(; m_checked_idx < m_read_idx; ++m_checked_idx){
		temp = m_read_buf[m_checked_idx];
		if(temp == '\r'){
			if((m_checked_idx + 1) == m_read_idx){
				return LINE_OPEN;
			}
			else if(m_read_buf[m_checked_idx + 1] == '\n'){
				m_read_buf[m_checked_idx++] = '\0';
				m_read_buf[m_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if(temp == '\n'){
			if((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')){
				m_read_buf[m_checked_idx - 1] = '\0';
				m_read_buf[m_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

// 循环读取客户数据，直到无数据或者对方关系连接
bool http_conn::read(){
	if(m_read_idx >= READ_BUFFER_SIZE){
		return false;
	}

	int bytes_read = 0;
	while(true){
		bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
		if(bytes_read == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				break;
			}
			return false;
		}
		else if(bytes_read == 0){
			return false; // 关闭连接了
		}
		m_read_idx += bytes_read;
	}
	return true;
}

// 解析HTTP请求行，获取请求方法，目标URL，以及HTTP版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
	// strpbrk(): string pointer break
	// 找到任意字符出现的第一个位置
	m_url = strpbrk(text, " \t");
	if(!m_url){
		return BAD_REQUEST;
	}
	*m_url++ = '\0';

	char *method = text;
	if(strcasecmp(method, "GET") == 0){
		m_method = GET;
	}
	else{
		// 目前只处理GET
		return BAD_REQUEST;
	}

	m_url += strspn(m_url, " \t");
	m_version = strpbrk(m_url, " \t");
	if(!m_version){
		return BAD_REQUEST;
	}
	*m_version++ = '\0';
	m_version += strspn(m_version, " \t");
	if(strcasecmp(m_version, "HTTP/1.1") != 0){
		return BAD_REQUEST;
	}
	if(strncasecmp(m_url, "http://", 7) == 0){
		m_url += 7；
		m_url = strchr(m_url, '/');
	}

	if(!m_url || m_url[0] != '/'){
		return BAD_REQUEST;
	}

	m_check_state = CHECK_STATE_HEADER;
	
	// 尚未完成
	return NO_REQUEST;
}

// 解析HTTP请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
	// 遇到空行，解析完毕
	if(text[0] == '\0'){
		// 如果该HTTP请求还有消息体，则还需要读取 m_content_length 字节的消息体
		// 转到 CHECK_STATE_CONTENT 状态
		if(m_content_length != 0){
			m_check_state = CHECK_STATE_CONTENT;
			// 未完成
			return NO_REQUEST;
		}
		// 完整
		return GET_REQUEST;
	}
	// 处理connection头部字段
	else if(strncasecmp(text, "Connection:", 11) == 0){
		text += 11;
		text += strspn(text, " \t");
		if(strcasecmp(text, "keep-alive") == 0){
			m_linger = true;
		}
	}
	// 处理Content-Length头部字段
	else if(strncasecmp(text, "Content-Length", 15) == 0){
		text += 15;
		text += strspn(text, " \t");
		m_content_length = atol(text);
	}
	// 处理Host头部字段
	else if(strncasecmp(text, "Host:", 5) == 0){
		text += 5;
		text += strspn(text, " \t");
		m_host = text;
	}
	else{
		printf("oops! unknow header %s\n", text);
	}
	
	// 未完成
	return NO_REQUEST;
}














