/*
This code implements the command "ls" in Linux
*/
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	DIR *cur_dir;
	struct dirent *item;

	if((cur_dir = opendir(argv[1])) == NULL){
		perror("Open dir error");
	}
	
	while((item = readdir(cur_dir)) != NULL){
		if(item->d_name[0] == '.')
			continue;
		printf("%s\n", item->d_name);
	}
	
	closedir(cur_dir);

	//return 0; // main 返回
	exit(0); // 强行退出程序
}
