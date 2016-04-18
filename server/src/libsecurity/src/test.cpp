#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "security.h"

int main(int argc, char *argv[])
{
    //char inData[] = "IqU78xNemECUErCzFqswuKt4U6TJlng17MhfEWt7dsY9755+o72/Z1y7Ubck7GYOtQ04J7SKGcWH+d/xCG96YsB29KN8HKzDcXoBDdCh9Tw=";
    //char key[] = "71741450870456302500000000000000";
    char *inData = argv[2];
    char *key = argv[1];
    char *outData = NULL;
    uint32_t outLen = 0;

    int ret = DecryptMsg(key, inData, (uint32_t)(strlen(inData)), &outData, outLen);
    printf("%s\n", outData);
    int n = strlen(outData);
    for (int i=0; i<n; i++)
    {
        uint8_t ch = (uint8_t)outData[i];
        printf("\\x%x", ch);
    }
    printf("\n");
    
    return 0;
}
