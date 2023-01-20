#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char *argv[]){

	int status = system("mpic++ main.cpp -o main");
	if(status != 0){
		exit(0);
	}
	
	int p, w, sum;
	char * command;
	if(argc < 2){
		p = 10;
		w = 2;
 
	}
	else{
		p = atoi(argv[1]);
		w = atoi(argv[2]);

	}
	sum = p + w;
	
	
	char stringSum[3];
	char stringP[3];
	char stringW[3];
	sprintf(stringSum, "%d", sum);
	sprintf(stringP, "%d", p);
	sprintf(stringW, "%d", w);
	
	char *word1 = malloc(100 * sizeof(char));
	char *word2 = malloc(23 * sizeof(char));
	char *word3 = malloc(1 * sizeof(char));
	strcpy(word1, "mpirun -np ");
	strcpy(word2, " -oversubscribe ./main ");
	strcpy(word3, " ");
	
	strcat(word1, stringSum);
	strcat(word1, word2);
	strcat(word1, stringP);
	strcat(word1, word3);
	strcat(word1, stringW);
	
	//printf("%ld",sizeof("mpirun -np 4 -oversubscribe ./main 2 2"));
	//char commend[39] = "mpirun -np 4 -oversubscribe ./main 2 2";
	status = system(word1);
	
	return 0;
}