#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#define MQNAME "/my_message_queue"

/*
Execution:
gcc -o pword pword.c
./pword 64 merhaba 1 input.txt
*/
  
struct wordFrequencyTuple {
    char *word;
    int frequency;
};

int main(int argc,char* argv[])
{
    // Message queue creation
    mqd_t mq;
    mq = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
    if (mq == -1) { perror("mq_open failed\n"); exit(1); }

    /*
    parameters obtained:
    - messageSize
    - numberOfTextFiles
    - outFileName
    - inputFileNames
    */

    #pragma region input_parsing

    // Input parsing
    int messageSize = atoi(argv[1]);
    int numberOfTextFiles = atoi(argv[3]);
    char outFileName[50];
    strcpy(outFileName, argv[2]);
    char* inputFileNames[numberOfTextFiles];

    for(int i = 4; i < argc; i++)
    {
        inputFileNames[i - 4] = argv[i];
    }

    printf("----- Input details -----\nMessage size: %d\nNumber of text files: %d\nOutput file name: %s\n-------------------------\n", messageSize, numberOfTextFiles, outFileName);

    #pragma endregion input_parsing

    struct wordFrequencyTuple tupleList[100000];
    int tupleListSize = 0;
    char word[64];

    #pragma region child_creation
    // Child creation
    pid_t parentPid = getpid();
    printf("Parent pid: %d\n", parentPid);
    
    int fileIndexToProcess = 0;
    for(int i = 0; i < numberOfTextFiles; i++)
    {
        printf("Creating child...\n");
        pid_t n = fork();
        if(n == 0) 
        {
            fileIndexToProcess = i;
            break;
        }
    }
    #pragma endregion child_creation

    if( getpid() == parentPid)
    {        
        int runningThreads = numberOfTextFiles;
        char *bufferp;  // pointer to receive buffer - allocated with malloc()
        int bufferlen; // length of the receive buffer      

        struct mq_attr mq_attr;
        mq_getattr(mq, &mq_attr);
        
        bufferlen = mq_attr.mq_msgsize;
        bufferp = (char *) malloc(bufferlen);   

        while (1) 
        {
            int n = mq_receive(mq, bufferp, bufferlen, NULL);
            if (n == -1) {
                perror("mq_receive failed\n");
                exit(1);
            }
            printf("\n----------------\nMessage Received\n----------------\n\n");
            //usleep(300000);
            
            // extract the number of messages
            int * numberOfMessages = (int *) bufferp;

            // if message contains tuples
            if(*numberOfMessages != 0)
            {
                char * currentBufferLocation = bufferp + 4;
                char * messageEndLocation = bufferp + messageSize;

                //printf("Current buff: %p Message End: %p\n", currentBufferLocation, messageEndLocation);
                //printf("Total number of messages in the following message: %d\n", *numberOfMessages);

                bool readingInt = true;
                int counter = 0;
                char space = ' ';
                char * tempWord = malloc(64);
                int wordSize = 0;
                char * wordStart;
                int * frequencyPointer;

                while(currentBufferLocation < messageEndLocation && counter < *numberOfMessages)
                {
                    if(readingInt)
                    {
                        int remainder = ((uintptr_t) currentBufferLocation) % 4;
                        if (remainder != 0)
                        {
                            currentBufferLocation += (4 - remainder);
                        }

                        frequencyPointer = (int*) (currentBufferLocation);
                        //printf("\nReading int %d from address \t\t%p : ", *frequencyPointer, frequencyPointer);
                        readingInt = false;

                        wordStart = currentBufferLocation + 4;
                        int wordSize = 0;
                    }
                    else
                    {
                        wordSize++;
                        //printf("%c", *currentBufferLocation);
                        if(*currentBufferLocation == space)
                        {
                            readingInt = true;
                            counter++;
                            //printf("%s", wordStart);

                            //printf("%c", *wordStart);

                            bool found = false;
                            for(int i = 0; i < tupleListSize; i++) // Search for the word in the existing tuples
                            {
                                // IF FOUND
                                if (strcmp(tupleList[i].word, wordStart) == 0) // If found, print the following
                                {
                                    //printf("%s++\n", tupleList[i].word, word);
                                    found = true;
                                    tupleList[i].frequency += *frequencyPointer;
                                    break;
                                }
                            }

                            // IF NOT FOUND
                            if(!found)
                            {
                                //printf("Create: %s\n", word);
                                tupleList[tupleListSize].word = (char*) malloc( wordSize * sizeof(char));
                                strcpy(tupleList[tupleListSize].word, wordStart);
                                tupleList[tupleListSize].frequency = *frequencyPointer;
                                tupleListSize = tupleListSize + 1;
                            }
                            
                        }
                        currentBufferLocation++;
                    }              
                }
            }
            else
            {
                runningThreads--;
                printf("%d\n", runningThreads);
                if (runningThreads <= 0)
                {
                    printf("--------- Message Stream Ended ----------\n");
                    for(int i = 0; i < tupleListSize; i++)
                    {
                        printf("-- Word: %s, frequency: %d\n", tupleList[i].word, tupleList[i].frequency);
                    }
                    mq_close(mq);

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

                    exit(0);
                }
            }
        }
    }
    else
    {     
        char fileName[30];
        strcpy(fileName, inputFileNames[fileIndexToProcess]);
        printf("\nStarted operation with file: %s, pid: %d\n", fileName, getpid());
        
        #pragma region ReadFile
        FILE *fp = fopen(fileName, "r");

        // traverses for all words independently
        while(fscanf(fp, "%s", word) != EOF )
        {
            int length = sizeof(word);
            
            // TURN INTO UPPER CASE CHARACTER
            for(int i = 0; i < length; i++)
            {
                if(word[i] >= 'a' && word[i] <= 'z')
                {
                    word[i] = word[i] - 32;
                }
            }
            
            bool found = false;
            
            // printf(word);
            //printf("tupleListSize: %d\n", tupleListSize);
            
            for(int i = 0; i < tupleListSize; i++) // Search for the word in the existing tuples
            {
                // IF FOUND
                if (strcmp(tupleList[i].word, word) == 0) // If found, print the following
                {
                    //printf("%s++\n", tupleList[i].word, word);
                    found = true;
                    tupleList[i].frequency++;
                    break;
                }
            }

            // IF NOT FOUND
            if(!found)
            {
                //printf("Create: %s\n", word);
                tupleList[tupleListSize].word = (char*) malloc( strlen(word) * sizeof(char));
                strcpy(tupleList[tupleListSize].word, word);
                tupleList[tupleListSize].frequency = 1;
                tupleListSize = tupleListSize + 1;
            }
        }
        // close the file
        fclose(fp);
        #pragma endregion ReadFile

        // print the tuples created
        for(int i = 0; i < tupleListSize; i++)
        {
            //printf("-- Word: %s, frequency: %d\n", tupleList[i].word, tupleList[i].frequency);
        }

        int index = 0;
        char *buffer = (char *) malloc(messageSize);
        char *bufferIndex;
        
        // SENDING MESSAGES
        while(index < tupleListSize)
        {
            for(int i = 0; i < messageSize; i++)
            {
                strcpy(buffer + i, " ");
            }

            bufferIndex = buffer;
            bool loop = true;
            // CREATİNG THE MESSAGE
            char * endOfBuffer = buffer + messageSize;
            
            int * messageTupleNumber = (int * ) buffer;
            *messageTupleNumber = 0;
            bufferIndex += 4;

            while(loop && (bufferIndex < endOfBuffer) && index < tupleListSize)
            {
                int wordSize = strlen(tupleList[index].word); // get the next tuple, calculate word size, add 6
                int tupleSize = wordSize + 5;

                // check if the size is smaller than remaining message size
                if(tupleSize <= (endOfBuffer - bufferIndex))
                {
                    (*messageTupleNumber)++;
                    //printf("TupleSize: %d\n", tupleSize);
                    int * intLocation = (int*)(bufferIndex);
                    *intLocation = tupleList[index].frequency;
                    char * wordLocation = (char *) (bufferIndex + 4);
                    strcpy(wordLocation,tupleList[index].word);  // add the tuple to the message
                    
                    //printf("%s \t%p \t| %d \t%p\n", wordLocation, wordLocation, *intLocation, intLocation);

                    bufferIndex = bufferIndex + tupleSize + 1;
                    bufferIndex[tupleSize] = ' ';
                    int remainder = ((uintptr_t) bufferIndex) % 4;
                    int advance = 0;

                    if(remainder != 0)
                    {
                        advance += (4 - remainder);
                    }

                    for(int i = 0; i < advance; i++)
                    {
                        *bufferIndex = ' ';
                        bufferIndex++;
                    }
                    index++; // increase index
                }
                else
                {
                    loop = false;
                }
            }

            int n = mq_send(mq, (const char *) buffer, messageSize, 0);
            if (n == -1) 
            {
                perror("mq_send failed\n"); 
                exit(1); 
            }
        }

        int * messageTupleNumber = (int * ) buffer;
        *messageTupleNumber = 0;

        int n = mq_send(mq, (const char *) buffer, messageSize, 0);
        if (n == -1) 
        {
            perror("mq_send failed\n"); 
            exit(1); 
        }

        mq_close(mq);

        // printf("I am child with input: %s\n", fileNames[fileIndexToProcess]);
        printf("Child process with pid: %d ended.\n", getpid());
        _Exit(0);
    }

    printf("Parent process has ended\n");
    return 0;
}