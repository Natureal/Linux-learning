#include <sys/socket.h>
#include <netinet/in.h>
#include <arps/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#define BUFFER_SIZE 4096

// main SM states: parsing request line, parsing header
enum CHECK_STATE{ CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER };

// sub SM states: read a complete line, read error, line incomplete
enum LINE_STATUS{ LINE_OK = 0, LINE_BAD, LINE_OPEN };

// results of handling HTTP request:
// NO_REQUEST: request incomplete, continue to read client data
// GET_REQUEST: got a complete request
// BAD_REQUEST: grammer error in request
// FORBIDDEN_REQUEST: client has no permission for accessing
// INTERNAL_ERROR: internal error in server
// CLOSED_CONNECTION: client has closed connection
enum HTTP_CODE{ NO_REQUEST, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST,
				INTERNAL_ERROR, CLOSED_CONNECTION };

// ===================================================
// for simplicity, we just reply client with success or failure information.
static const char* szret[] = {"I get a correct result\n", "Something wrong\n"};

// sub SM, used to parse a single line
LINE_STATUS parse_line(char* buffer, int& checked_index, int& read_index){
	char temp;
	// checked_index points to the bypte under parsing
	// read_index points to the next byte of the tail of client data
	// Thus, [0, checked_index) of buffer has been parsed.
	// Next, let us parse [checked_index, read_index)
	for(; checked_index < read_index; ++checked_index){
		// get a byte that we want to parse
		temp = buffer[checked_index];
		// if the byte equals to '\r', it is possible to get a complete line!
		if(temp == '\r'){
			// if this is the last byte, we still need to read more data
			if((checked_index + 1) == read_index){
				return LINE_OPEN;
			}
			// if the next byte is '\n', then this line is complete
			else if(buffer[checked_index + 1] == '\n'){
				// set the last two byte to 0
				buffer[checked_index++] = '\0';
				buffer[checked_index++] = '\0';
				return LINE_OK;
			}
			// otherwise, the HTTP request has grammer error(s).
			return LINE_BAD;
		}
		// if the byte equals to '\n', it is possible to get a complete line!
		else if(temp == '\n'){
			if((checked_index > 1) && buffer[checked_index - 1] == '\r'){
				buffer[checked_index - 1] = '\0';
				buffer[checked_index++] = '\0';
				return LINE_OK;
			}
		}
		return LINE_BAD;
	}
	// no ending, continue to read data
	return LINE_OPEN;
}

// parse request lines
HTTP_CODE parse_requestline(char* temp, CHECK_STATE& checkstate){
	// strpbrk(): string pointer break
	// find the first occurrence in str1 of any of the char that is part of str2
	char* url = strpbrk(temp, " \t");
	// if there is no space or \t in the request line, there must be a problem
	if(!url){
		return BAD_REQUEST;
	}
	*url++ = '\0';

	char* method = temp;
	if(strcasecmp(method, "GET") == 0){ // only support GET method
		printf("The request method is GET\n");
	}
	else{
		return BAD_REQUEST;
	}
	
	// span all characters that are space or \t
	url += strspn(url, " \t"); 
	char* version = strpbrk(url, " \t");
	if(!version){
		return BAD_REQUEST;
	}
	*version++ = '\0';
	version += strspn(version, " \t");
	// only support HTTP/1.1
	if(strcasecmp(version, "HTTP/1.1") != 0){
		return BAD_REQUEST;
	}
	// check whether URL is correct or not
	if(strncasecmp(url, "http://", 7) == 0){
		url += 7;
		url = strchr(url, '/');
	}

	if(!url || url[0] != '/'){
		return BAD_REQUEST;
	}
	printf("The request URL is: %s\n", url);
	// parsed thr request line, state transfers to parse headers
	checkstate = CHECK_STATE_HEADER;

	// incomplete
	return NO_REQUEST;
}

// parse headers
HTTP_CODE parse_headers(char* temp){
	// if this is an empty line, correct
	if(temp[0] == '\0'){
		return GET_REQUEST;
	}
	else if(strncasecmp(temp, "Host:", 5) == 0){ // handle HOST header
		temp += 5;
		temp += strspn(temp, " \t");
		printf("the request host is: %s\n", temp);
	}
	else{ // ignore other kinds of headers
		printf("I can not handle this header\n");
	}
	// incomplete
	return NO_REQUEST;
}

// entrance of parsing HTTP request
HTTP_CODE parse_content(char* buffer, int& checked_index, CHECK_STATE& checkstate,
						int& read_index, int& start_line){
	LINE_STATUS linestatus = LINE_OK; // current state of reading line
	HTTP_CODE retcode = NO_REQUEST; // current result of parsing
	// main SM, used to extract all complete lines from buffer
	while((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK){
		char* temp = buffer + start_line; // start point of line in buffer
		start_line = checked_line; // record the start point of the next line
		switch(checkstate){
			case CHECK_STATE_REQUESTLINE:{ // request line
				retcode = parse_requestline(temp, checkstate);
				if(retcode == BAD_REQUEST){
					return BAD_REQUEST;
				}
				break;
			}
			case CHECK_STATE_HEADER:{
				retcode = parse_headers(temp);
				if(retcode == BAD_REQUEST){
					return BAD_REQUEST;
				}
				else if(retcode == GET_REQUEST){
					return GET_REQUEST;
				}
				break;
			}
			default:{
				return INTERNAL_ERROR;
			}
		}
	}
	// if not read a complete line, we need more data
	if(linestatus == LINE_OPEN){
		return NO_REQUEST;
	}
	else{
		return BAD_REQUEST;
	}
}

int main(int argc, char* argv[]){
	if(argc <= 2){
		printf("usage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	// init a socket address
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET; // sin_family
	inet_pton(AF_INET, ip, &address.sin_addr); // sin_addr
	address.sin_port = htons(port); // sin_port

	// init a socket instance
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	
	// bind the socket to an address
	int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	struct sockaddr_in client_address;
	socklen_t client_addrlength = sizeof(client_address);
	int fd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
	if(fd < 0){
		printf("errno is: %d\n", errno);
	}
	else{
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);
		int data_read = 0;
		int read_index = 0;
		int checked_index = 0;
		int start_line = 0;
		// set the initial state of main SM
		CHECK_STATE checkstatee = CHECK_STATE_REQUESTLINE;
		while(1){
			data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
			if(data_read == -1){
				printf("reading failed\n");
				break;
			}
			else if(data_read == 0){
				printf("remote client has closed the connection\n");
				break;
			}
			read_index += data_read;
			// parse the data that we've got currently
			HTTP_CODE result = parse_content(buffer, checked_index, checkstate,
												read_index, start_line);
			if(result == NO_REQUEST){ // request incomplete
				continue;
			}
			else if(result == GET_REQUEST){ // got a complete request line
				send(fd, szret[0], strlen(szret[0]), 0);
				break;
			}
			else{ // other situations: error
				send(fd, szret[1], strlen(szret[1]), 0);
				break;
			}
		}
		close(fd);
	}
	close(listenfd);
	return 0;
}
		








