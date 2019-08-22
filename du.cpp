#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

char s[1024];

unsigned int Dfs(DIR *dir, int p){
	struct stat st;
	struct dirent *item;
	long int sz = 4096; // 文件夹本身大小
	while(item = readdir(dir)){
		stat(strcat(s, item->d_name), &st);
		if(strcmp(item->d_name, "..") == 0 ||
		   strcmp(item->d_name, ".") == 0){

		}
		else if(S_ISDIR(st.st_mode)){
			DIR *child_dir = opendir(s);
			if(child_dir != NULL){
				int len = strlen(item->d_name);
				s[p + len] = '/';
				s[p + len + 1] = '\0';
				sz += Dfs(child_dir, p + len + 1);
			}
			closedir(child_dir);
		}
		else if(S_ISREG(st.st_mode)){
			sz += st.st_blocks * 512;
		}
		s[p] = '\0';
	}
	printf("%ldK %s\n", sz / 1024, s);
	return sz;
}

int main(int argc, char *argv[]){
	if(argc < 2){
		return 0;
	}

	DIR *dir = opendir(argv[1]);
	
	if(dir == NULL)
		return 0;
	
	s[0] = '.';
	s[1] = '/';

	Dfs(dir, 2);

	closedir(dir);

	return 0;
}
