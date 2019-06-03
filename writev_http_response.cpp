#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

// define two kinds of status codes and info
static const char* status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char* argv[]){
	if(argc <= 3){
		printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	const char* file_name = argv[3];
	
	// information in address is all in network byte order
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	// initilize a socket
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);

	int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(sock, 5);
	assert(ret != -1);

	struct sockaddr_in client;
	socklen_t client_addrlength = sizeof(client);
	int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
	if(connfd < 0){
		printf("errno is: %d\n", errno);
	}
	else{
		// for storing http response's status lines, headers, an empty line
		char header_buf[BUFFER_SIZE];
		memset(header_buf, '\0', BUFFER_SIZE);
		// for storing the content of object file
		char* file_buf;
		// for acquiring the attributes of object file, such as whether dir, file size, etc
		struct stat file_stat;
		// if the object file is a valid file
		bool valid = true;
		// current size
		int len = 0;
		if(stat(file_name, &file_stat) < 0){
			// object file  does not exit
			valid = false;
		}
		else{
			if(S_ISDIR(file_stat.st_mode)){
				// object is a dir
				valid = false;
			}
			else if(file_stat.st_mode & S_IROTH){
				// current user has authentication
				int fd = open(file_name, O_RDONLY);
				// allocate memory for file_buf
				file_buf = new char[sizeof(file_buf)];
				memset(file_buf, '\0', file_stat.st_size + 1);
				if(read(fd, file_buf, sizeof(file_buf) - 1) < 0){
					valid = false;
				}
			}
			else{
				valid = false;
			}
		}
		// if the object file is valid, send http response
		if(valid){
			// write status lines, "Content-Length" header and an empty line into header_buf
'/tmp/evince-3414/image.TLPN2Z.png' 			ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
			len += ret;
			
			ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %d\r\n", file_stat.st_size);
			len += ret;

			ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");
			
			struct iovec iv[2];
			iv[0].iov_base = header_buf;
			iv[0].iov_len = strlen(header_buf);
			iv[1].iov_base = file_buf;
			iv[1].iov_len = file_stat.st_size;
			ret = writev(connfd, iv, 2);
		}
		else{
			// the object file is invalid, inform the client "internal error"
			ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
			len += ret;

			ret = snprintf(header_buf, BUFFER_SIZE - 1 - len, "%s", "\r\n");
			send(connfd, header_buf, strlen(header_buf), 0);
		}
		close(connfd);
		delete [] file_buf;
	}
	
	close(sock);
	return 0;
}






