#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "user.h"
#define USER_MAX_VAR_SIZE 10

struct user 
{
    char * nick;
    char * pw;
    bool oper;
    int room;
};

User * new_user()
{
    User * user = malloc(sizeof(User));
    user -> nick = malloc(USER_MAX_VAR_SIZE * sizeof(char));
    user -> pw = malloc(USER_MAX_VAR_SIZE * sizeof(char));
    user -> oper = false;
    user -> room = 0;

    strcpy(user -> nick, "ANON");
    return user;
}

bool isClientOnline(char buffer[USER_MAX_VAR_SIZE])
{
    buffer[strlen(buffer) - 1] = 0;
    char currentNick[USER_MAX_VAR_SIZE];
    FILE * fp = fopen("online.txt", "r");

    if (fp == NULL) 
    {
        return false;
    }

    while (fgets(currentNick, USER_MAX_VAR_SIZE, fp) != NULL) 
    {
        currentNick[strlen(currentNick) - 1] = 0;
        /* printf("%s é igual a %s? %d\n",buffer, currentNick, strcmp(currentNick, buffer)); */
        if (!strcmp(currentNick, buffer))
        {
            fclose(fp);
            return true;
        }
        
    }
    fclose(fp);
    return false;
}


bool isUserAuth(char nick[USER_MAX_VAR_SIZE], char buffer[USER_MAX_VAR_SIZE])
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[strlen(nick)], pwReg[strlen(buffer)];
    int op;

	FILE * fp = fopen("regs.txt", "r");

	if(fp == NULL)
    {
		return false;
    }


    while(fgets(allLine, 2*USER_MAX_VAR_SIZE+3, fp) != NULL)
    {
        for (int i = 0, j = 0,count = 0; i < strlen(allLine); i++)
        {
            if (count == 0) 
            {
                if (strncmp(&allLine[i], ":", 1) || strlen(nick) > i) // !=
                {
                    strncpy(&nickReg[i], &allLine[i], 1);
                    printf("char do txt %c\n", allLine[i]);
                    printf("NICK %s\n", nickReg);
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(nick, nickReg, strlen(nick))) // ==
                {
                    count++;
                }
                if (!strncmp(&allLine[i], ":", 1) && strncmp(nick, nickReg, strlen(nick)))
                {
                    break;
                }
            } 
            else if (count == 1)
            {
                if (strncmp(&allLine[i], ":", 1))
                {
                    strncpy(&pwReg[j], &allLine[i], 1);
                    j++;
                    printf("PW %s\n", pwReg);
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(buffer, pwReg, strlen(pwReg)))
                {
                    return true;
                }
            }
        }
        bzero(nickReg, USER_MAX_VAR_SIZE);
        bzero(pwReg, USER_MAX_VAR_SIZE);
    }
	return false;
}

// WIP

/* void regs_user(char nick[USER_MAX_VAR_SIZE], char pw[USER_MAX_VAR_SIZE])
{
    if(!isAlreadyRegs(nick))
    {
        FILE * fp = fopen("regs.txt", "a");
        fprintf(fp, "%s:%s:%d", nick,pw,0);
    } else {
        printf("Esse nickname já se encontra em uso.");
    }
} */