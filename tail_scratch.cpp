/*
This code implements the command "tail" in Linux shell

*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int parse_opt(int argc, char *argv[]){
	const char *optstr = "n:";
	char opt;
	while((opt = getopt(argc, argv, optstr)) != -1){
		return atoi(optarg);
	}
	return -1;
}

int main(int argc, char *argv[]){
	int n = parse_opt(argc, argv);
	FILE *fin = fopen(argv[argc - 1], "r");
	if(!fin) exit(1);
	char c;
	if(n == 0){
		return 0;
	}
	if(n < 0){
		while((c = fgetc(fin)) != EOF) putchar(c);
		return 1;
	}
	fseek(fin, 0, SEEK_END);
	int p = ftell(fin), cnt = 0;
	while(p){
		// 由于文件末尾有一个 EOF 符号，因此要先 --p
		fseek(fin, --p, SEEK_SET);
		c = fgetc(fin);
		// 每一行最后都有 \n
		if(c == '\n'){
			if(cnt++ == n) break;
		}
	}
	// 下面这句是为了处理 p == 0 时，fgetc 把文件位置指针往后挪了一位的问题
	if(!p) fseek(fin, 0, SEEK_SET);
	while((c = fgetc(fin)) != EOF) putchar(c);
	return 0;
}
