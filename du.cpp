#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

void Dfs(DIR *dir){
	struct stat st;
	struct dirent *item;
	while(item = readdir(dir)){
		stat(item->d_name, &st);
		if(S_ISDIR(st.st_mode)){
			printf("%s\n", item->d_name);
		}
	}
}

int main(int argc, char *argv[]){
	DIR *dir = opendir(argv[1]);
	
	if(dir == NULL)
		return 0;
	
	Dfs(dir);

	return 0;
}
