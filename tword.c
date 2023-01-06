#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

/*  
Execution:
gcc -o tword tword.c
./tword outFile 5 input1.txt input2.txt input3.txt input4.txt input5.txt
*/

struct wordFrequencyTuple {
    char *word;
    int frequency;
};

struct threadData {
    char fileName[20];
    struct wordFrequencyTuple tupleList[1000];
    int tupleCount;
};

struct wordFrequencyTuple tupleList[1000];
int tupleListSize = 0;
pthread_t tid[10];
pthread_mutex_t mutex;

void *readFile(void * arg_ptr)
{
    struct threadData *tData = (struct threadData *) arg_ptr;
    
    //printf("ReadFile Start...\n");
    // open file
    char word[64];
    FILE *fp = fopen(tData->fileName, "r");
    //printf("Print %s\n", tData->fileName);

    tData->tupleCount = 0;

    // - Read and increase tuples -
    while(fscanf(fp, "%s", word) != EOF)
    {
        //printf("while\n");
        int length = sizeof(word);
        
        // turn word into uppercase
        for(int i = 0; i < length; i++)
        {
            if(word[i] >= 'a' && word[i] <= 'z')
            {
                word[i] = word[i] - 32;
            }
        }
        
        // handle tuple list
        bool found = false;
        for(int i = 0; i < tData->tupleCount; i++) // Search for the word in the existing tuples
        {
            if (strcmp(tData->tupleList[i].word, word) == 0) // if tuple exists
            {
                //printf("%s++\n", tupleList[i].word, word);
                found = true;
                tData->tupleList[i].frequency++;
                //printf("%d\n", tData->tupleList[i].frequency);
                break;
            }
        }
        if(!found)
        {
            //printf("Create: %s\n", word);
            tData->tupleList[tData->tupleCount].word = (char*) malloc( strlen(word) * sizeof(char));
            strcpy(tData->tupleList[tData->tupleCount].word, word);
            tData->tupleList[tData->tupleCount].frequency = 1;
            tData->tupleCount = tData->tupleCount + 1;
        }
    }
    //printf("ReadFile...\n");   
}

int main(int argc,char* argv[])
{
    // - Input parsing -
    int numberOfTextFiles = atoi(argv[2]);
    char outFileName[30];
    strcpy(outFileName, argv[1]);
    struct threadData fileNames[numberOfTextFiles];

    for(int i = 3; i < argc; i++)
    {
        strcpy(fileNames[ i - 3 ].fileName, argv[i]);
    }

    // - Thread creation -
    for(int i = 0; i < numberOfTextFiles; i++)
    {
        int threadCreation = pthread_create(&(tid[i]), NULL, &readFile, (void *) &fileNames[i]);
        if (threadCreation != 0)
            printf("\ncan't create thread :[%s]", strerror(threadCreation));
    }

    // - Wait for threads -
    for(int i = 0; i < numberOfTextFiles; i++)
    {
        //printf("Start...\n");   
        pthread_join(tid[i], NULL);
        bool found = false;
        for(int j = 0; j < fileNames[i].tupleCount; j++)
        {
            int tupleCount = fileNames[i].tupleCount;
            char word[64];
            strcpy(word, fileNames[i].tupleList[j].word);

            for(int k = 0; k < tupleListSize; k++) // Search for the word in the existing tuples
            {
                // IF FOUND
                if (strcmp(tupleList[k].word, word) == 0) // If found, print the following
                {
                    //printf("%s++\n", tupleList[i].word, word);
                    found = true;
                    tupleList[k].frequency += fileNames[i].tupleList[j].frequency;
                    break;
                }
            }

            // IF NOT FOUND
            if(!found)
            {
                //printf("Create: %s\n", word);
                tupleList[tupleListSize].word = (char*) malloc( strlen(word) * sizeof(char));
                strcpy(tupleList[tupleListSize].word, word);
                tupleList[tupleListSize].frequency = fileNames[i].tupleList[j].frequency;
                tupleListSize = tupleListSize + 1;
            }    
        }
    }
    //printf("All are jointed\n");



    // - Sort the results -
    char tempString[64];
    for(int i = 0; i < tupleListSize; i++)
    {
      for(int j = i + 1; j < tupleListSize; j++)
      {
         if(strcmp(tupleList[i].word, tupleList[j].word) > 0)
         {
            strcpy(tempString, tupleList[i].word);
            strcpy(tupleList[i].word, tupleList[j].word);
            strcpy(tupleList[j].word, tempString);
         }
      }
   }

    // - Print to file -
    FILE *outputFile = fopen(outFileName, "w");
    if (outputFile == NULL)
    {
        printf("Error opening file\n");
        exit(1);
    }

    for(int i = 0; i < tupleListSize; i++)
    {
        fprintf(outputFile, "%s %d\n", tupleList[i].word, tupleList[i].frequency);
        //printf("-- Word: %s, frequency: %d\n", tupleList[i].word, tupleList[i].frequency);
    }
    fclose(outputFile);

    // - Done -
    return 0;
}

