#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"

/*
 * Driver class for tokenizer.c. Allows a client to tokenize a string and see its outputs
 * in the terminal. To use, type [echo 'sample string "with << special) character;s"' | ./tokenize]
 */
int main(int argc, char **argv) {
	
	// TODO: Implement the tokenize demo.
	//char test_str1[500] = "\n";
	char *inp; //string storing input from stdin
	size_t bufsize = 255;
	
	inp = (char *) calloc(bufsize, sizeof(char)); //allocate memory to the string

	getline(&inp,&bufsize,stdin); //get input froms stdin


	vect_t *tokens = tokenize(inp); //tokenize the input in a vector
	int a = vect_size(tokens);
	for (int i = 0; i < a; i++) { //iterate for every item in vector
  		char *c = vect_get_copy(tokens,i);
		
		printf(c); 
		printf("\n");
		free(c); // free the item we have just allocated
	}
	vect_delete(tokens);
	free(inp);
}
