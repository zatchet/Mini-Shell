#include <stdio.h>
#include "vect.h"
#include <string.h>
#include "tokenizer.h"
#include <stdlib.h>

/*
 * Tokenize offers the functionality for the methods required in the tokenize.c
 * driver file
 */
vect_t *tokenize (char * input) {
	
	int i = 0;

	vect_t *tokens = vect_new(); // input will be stored in a vector data structure
	
	int startIdx = 0; // starting index of the current token being processed
	int inQuote = 0; // keeps track of whether the current token being processed is in a set of quotation marks
	while (input[i] != '\0') { // iterates until the end of the input (stops at null byte)
		switch (input[i]) {
			case '\n': //treat all white space the same
			case '\t':
			case ' ':
				;
				if (inQuote == 1) {
					break; // if the current token being processed is in a set of quotation marks
					       // disregard the whitespace
				}
				char *token = calloc(i - startIdx + 1, sizeof(char));
				strncpy(token, &input[startIdx], i - startIdx);
			        
				token[i-startIdx] = '\0';
				if (i - startIdx != 0) {
                                        vect_add(tokens, token);
                                }
				startIdx = i + 1;
				free(token);
				
				break;
			case '(': // treat all these tokens the same
			case ')':
			case '<':
			case '>':
			case ';':
			case '|':
				;
				if (inQuote == 1) {
					break;
				}
				if (i - startIdx != 0) {
					char *token2 = calloc(i - startIdx + 1, sizeof(char));
        	                        strncpy(token2, &input[startIdx], i - startIdx);
	                                token2[i-startIdx] = '\0';
					vect_add(tokens, token2);
					free(token2);
				}
                                startIdx = i;
                                //free(token2);
				
				char *c = calloc(2, sizeof(char));
				strncpy(c, &input[i], 1);
				c[1] = '\0';
				vect_add(tokens, c);
				startIdx ++;
				free(c);
				break;

			case '\"':
                                ;
				if (inQuote == 0) { // if a quotation mark is reached and we are not currently
					   	    // processing a token that already has a quotation mark
						    // then now it does
					inQuote = 1;
				} else {
					inQuote = 0; // otherwise we have reached the second quotation mark
						     // for this token and we can process it
					char *token3 = calloc(i - startIdx + 1, sizeof(char));
                                	strncpy(token3, &input[startIdx], i - startIdx);

                                	token3[i-startIdx] = '\0';
                                	if (i - startIdx != 0) {
                                        	vect_add(tokens, token3);
                                	}
                                	free(token3);
				}
				startIdx = i + 1;
                                break;
		
		}

		i++;
	}
	
	// add the last word in the string if there wasn't a space after it
	if (startIdx - i != 0) {
		char *yoken = calloc(i - startIdx + 1, sizeof(char));
        	strncpy(yoken, &input[startIdx], i - startIdx);
        	yoken[i - startIdx] = '\0';
        	vect_add(tokens, yoken);
		free(yoken);
	}

        return tokens;


}

