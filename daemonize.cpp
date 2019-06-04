#include <unistd.h>

bool daemonize(){
	// create child process, close parent process
	pid_t pid = fork();
	if(pid < 0){
		return false;
	}
	else if(pid > 0){
		exit(0);
	}
	
	// set file permission mask. permission is mode & 0777 while create new file using
	// open(const char* pathname, int flags, mode_t mode)
	umask(0);

	// create a new session
	pid_t sid = setsid();
	if(sid < 0){
		return false;
	}

	// switch workdir
	if(chdir("/") < 0){
		return false;
	}

	// close stdin, stdout and stderrorout devices
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// close other opened fd, code omitted here !!

	// redirect stdin, stdout, stderrorout to the file /dev/null
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);
	return true;
}


// the following func do the same thing
// int daemon(int nochdir, int noclose);
