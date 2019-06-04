#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char* argv[]){
	if(argc != 2){
		printf("usage: %s <file>\n", argv[0]);
		return 1;
	}
	// O_CREAT: create if not exist
	// O_WRONLY: write only
	// O_TRUNC: delete if exist
	// 0666: permission code --> 000, 110, 110 --> ---rw-rw-
	int filefd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
	assert(filefd > 0);

	int pipefd_stdout[2];
	int ret = pipe(pipefd_stdout);
	assert(ret != -1);

	int pipefd_file[2];
	ret = pipe(pipefd_file);
	assert(ret != -1);

	// from stdin to pipefd_stdout
	ret = splice(STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32768, 
										SPLICE_F_MORE | SPLICE_F_MOVE);
	assert(ret != -1);

	// from pipefd_stdout to pipefd_file
	ret = tee(pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_NONBLOCK);
	assert(ret != -1);

	// from pipefd_file to filefd
	ret = splice(pipefd_file[0], NULL, filefd, NULL, 32768,
										SPLICE_F_MORE | SPLICE_F_MOVE);
	assert(ret != -1);

	// from pipefd_stdout to stdout
	ret = splice(pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 32768,
										SPLICE_F_MORE | SPLICE_F_MOVE);
	assert(ret != -1);

	close(filefd);
	close(pipefd_stdout[0]);
	close(pipefd_stdout[1]);
	close(pipefd_file[0]);
	close(pipefd_file[1]);
	return 0;
}
