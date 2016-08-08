#include "getword.h"

//Tri Nguyen
//masc0055
//CS 570
//Dr. John Carroll
/* The purpose of this program is to read character from standard in and returning
*  the word count and the word. There are special metacharacters that are: 
*  "<", ">", "'", "|", ";", and "&". THere special metacharacters determine what delimits
*  a word or how it interacts and composes a word.
*/
int dollar = 0;

int getword(char *w){
        int inputChar, i;

        //character count
        int count = 0;

        //metacharacters in ASCII: "<", ">", "|", and "&".
        char metaC[4] = {60,62,124,38};           
        
        //getting char from stream
        inputChar = getchar();

        //checking for leading spaces
        if(inputChar == ' '){ 
            for(;;){                          
                inputChar = getchar();
                if(inputChar == ' '){
                    inputChar = getchar();
                }
                else{
                    break;
                }
            }
        }

        //new line then terminate.
        if(inputChar == '\n'){
            *w = '\0';
            return count;
        } 

        //case for ";". ; = new line
        if(inputChar == ';'){
            *w = '\n';
            *w = '\0';
            return count;
        }

        //add metachar. to array for output with the count
        if(inputChar == 60 || inputChar == 62 
                ||inputChar == 124 || inputChar == 38){
            *w = inputChar;
            *w++;
            *w = '\0';  
            count++;                            
            return count;
        }

        //Checking EOF
        while(inputChar != EOF){                        
            for(;;){

                //handle buffer overruns
                if(count >= 254){
                    *w = '\0';
                    ungetc(inputChar, stdin);
                    ungetc(inputChar, stdin);
                    inputChar = getchar();
                    return count;
                }       

                //check for space
                if(inputChar == ' '){
                    //null terminate                   
                    *w = '\0';                          
                    return count;
                }

                //case for ";". ; = new line
                //HERE
                if(inputChar == ';'){
                    *w = '\n';
                    *w = '\0';
                    ungetc(inputChar, stdin); 
                    return count;
                }

                //case for ";". ; = new line
                if(inputChar == ';'){
                    *w = '\n';
                    *w = '\0';
                    return count;
                }

                //check for newline
                if(inputChar == '\n'){   
                    //null terminate                
                    *w = '\0';
                    ungetc(inputChar, stdin);                        
                    return count;
                } 

                //check EOF
                if(inputChar == EOF){
                    //null terminate
                    *w = '\0';                          
                    return count;
                }

                //case for meta char
                for(i=0; i<=3; i++){                
                    if(inputChar == metaC[i]){
                        //null terminate
                        *w = '\0'; 

                        //return metachar to stream                     
                        ungetc(inputChar, stdin);                                 
                        return count;
                    }
                }

                //case for "'" 
                while(inputChar == 39){
                    for(;;){
                        inputChar = getchar();

                        //find end "'"
                        if(inputChar == 39){
                            *w = '\0';  
                            return count;
                        }

                        //case for newline
                        if(inputChar == '\n'){
                            *w = '\0';
                            ungetc(inputChar, stdin);    
                            return count;
                        }

                        //case for "\" in side quotes
                        while(inputChar == 92){
                            for(;;){
                                int backS = 92;
                                inputChar = getchar();
                                int apost = 39;

                                //case for when "'" precedes 
                                while(inputChar == 39){
                                    *w = apost;
                                    *w++;
                                    count++;
                                    for(;;){
                                        inputChar = getchar();

                                        //find the end quote
                                        if(inputChar == 39){
                                            *w = '\0';
                                            return count;
                                        }

                                        *w = inputChar;
                                        *w++;
                                        count++;
                                    }
                                }

                                //case for when metachar precedes
                                for(i=0; i<=3; i++){                
                                    while(inputChar == metaC[i]){

                                        *w = backS;
                                        *w++;
                                        count++;
                                        *w = inputChar;
                                        *w++;
                                        count++;

                                        for(;;){
                                            inputChar = getchar();

                                            //case for "'" after metachar
                                            if(inputChar == 39){
                                                *w = '\0';
                                                return count;
                                            }

                                            *w = inputChar;
                                            *w++;
                                            count++;
                                        }
                                    }
                                }

                            }
                        }
                        
                        *w = inputChar;
                        *w++;
                        count++;
                    }
                }

                //case for "\"
                if(inputChar == 92){                  
                    for(;;){
                        inputChar = getchar();

                        //case for two "\" in a row
                        if(inputChar == 92){
                            *w = inputChar;
                            *w++;
                            count++;
                            inputChar = getchar();
                            //break;
                        }

                        //check for new line
                        if(inputChar == '\n'){          
                            *w = '\0';
                            ungetc(inputChar, stdin);
                            return count;
                        }

                        //checking EOF
                        if(inputChar == EOF){           
                            *w = '\0';
                            return count;
                        }

                        if(inputChar == 39){
                            *w = inputChar;
                            *w++;
                            count++;
                        }

                        break;
                    }
                }

                //add char to char array
                *w = inputChar;    
                //increase to next spot in array
                w++;                   
                //increase word count           
                count++; 
                //getting next char to check                              
                inputChar = getchar();  
            }
        }


    *w = '\0';
    return -1;
}
