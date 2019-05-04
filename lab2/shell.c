#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define SUCCESS    1
#define ERROR      0
#define TRUE       1
#define FALSE      0

int split_cmd(char *cmd,int len);//拆解命令
int background_cmd(int num);//后台运行
int recurse_cmd(int l,int r);//处理'|'与';' 递归执行
int redir_cmd(int l,int r);//处理重定向
int builtin_cmd(int l,int r);//执行内建命令
int external_cmd(int l,int r);//执行外部命令

char cmds[128][256];
int pid_num = 0;

int main()
{
    char cmd[256];

    while (1) 
    {
	char wd[4096];
        printf("\033[34;1m%s\033[0m$ ",getcwd(wd, 4096));
        fflush(stdin);
        fgets(cmd, 256, stdin);

        int len;
	len = strlen(cmd);
        cmd[len-1] = '\0';
	int num = split_cmd(cmd,len);
	
	if(background_cmd(num)) continue;
	else pid_num = 0;

        if (!num) continue;

	int infd = dup(STDIN_FILENO);
        int outfd = dup(STDOUT_FILENO);
	recurse_cmd(0,num);
	dup2(infd, STDIN_FILENO);
	dup2(outfd, STDOUT_FILENO);
    }
}

int split_cmd(char *cmd,int len)
{
    int i,j,num = 0;
    for(i = 0, j = 0;i < len - 1;i++)
    {
	if(cmd[i] != ' ' && cmd[i] != '|' && cmd[i] != ';' && cmd[i] != '(' && cmd[i] != ')' && cmd[i] != '&' && cmd[i] != '<')
	{
	    cmds[num][j] = cmd[i];
	    j++;
	}
	else 
	{
	    if(j > 0)
	    {
	    	cmds[num][j] = '\0';
	    	num++;
	    	j = 0;
	    }
	    if(cmd[i] == '|')
	    {
		cmds[num][0] = '|';
		cmds[num][1] = '\0';
		num++;
	    }
	    if(cmd[i] == ';')
	    {
		cmds[num][0] = ';';
		cmds[num][1] = '\0';
		num++;
	    }
	    if(cmd[i] == '&')
	    {
		cmds[num][0] = '&';
		cmds[num][1] = '\0';
		num++;
	    }
	    if(cmd[i] == '<')
	    {
		cmds[num][0] = '<';
		cmds[num][1] = '\0';
		num++;
	    }
	}
    }
    if(j > 0)
    {
	cmds[num][j] = '\0';
	num++;
    }
    return num;
}

int background_cmd(int num)
{
    if(num == 0) return FALSE;
    if(strcmp(cmds[num-1],"&") == 0)
    {
	pid_t pid = fork();
	
        if(pid == -1)
	{
	    printf("Error:fail to fork\n");
	    return ERROR;
	}
	if(pid > 0)
	{
	    pid_num++;
    	    printf("[%d] %d\n",pid_num,pid);
	}
    	if(pid == 0)
	    exit(recurse_cmd(0,num-1));
	return TRUE;
    }
    else return FALSE;
}

int recurse_cmd(int l,int r)
{
    int i,flag = -1;
    if(l >= r) return SUCCESS;
    for(i = l;i < r;i++)
	if(strcmp(cmds[i],"|") == 0 || strcmp(cmds[i],";") == 0)
	{
	    flag = i;
    	    break;
	}
    if(flag == -1) return(redir_cmd(l,r));
    else if(flag + 1 == r && strcmp(cmds[i],"|") == 0) 
    {
	printf("Error:miss parameter\n");
	return ERROR;
    }
    else
    {
        int fd[2];
        pipe(fd);
    	pid_t pid = fork();
    	if(pid == 0)
    	{
	    if(strcmp(cmds[flag],"|") == 0)
	    {
	        close(fd[0]);
	        dup2(fd[1],STDOUT_FILENO);
	        close(fd[1]);
	    }	    
	    exit(redir_cmd(l,flag));
        }
        else if(pid == -1)
	{
	    printf("Error:fail to fork\n");
	    return ERROR;
	}
        else
        {
	    wait(NULL);
	    if(strcmp(cmds[flag],"|") == 0)
	    {
	        close(fd[1]);
	        dup2(fd[0],STDIN_FILENO);
	        close(fd[0]);
	    }
	    recurse_cmd(flag+1,r);//递归执行
        }
	return SUCCESS;
    }
}

int redir_cmd(int l,int r)
{
    int i,flag = 0;
    int rr = r;
    if(l >= r) return SUCCESS;
    for(i = l;i < r;i++)
	if(strcmp(cmds[i],">") == 0)
	{
	    flag = 1;
	    rr = i;
	    break;
	}
	else if(strcmp(cmds[i],">>") == 0)
	{
	    flag = 2;
	    rr = i;
	    break;
	}
	else if(strcmp(cmds[i],"<") == 0)
	{
	    flag = 3;
	    rr = i;
	    break;
	}
    char *filename;
    if(flag) filename = cmds[i+1];

    int fd;
    if(flag == 1)
    {
	fd = open(filename,O_RDWR | O_CREAT | O_TRUNC,0644);
	dup2(fd,STDOUT_FILENO);
	close(fd);
    }
    else if(flag == 2)
    {
	fd = open(filename,O_RDWR | O_CREAT | O_APPEND,0644);
	dup2(fd,STDOUT_FILENO);
	close(fd);
    }
    else if(flag == 3)
    {
	fd = open(filename,O_RDONLY);
	dup2(fd,STDIN_FILENO);
	close(fd);
    }
    if(builtin_cmd(l,rr)) return SUCCESS;
    else return(external_cmd(l,rr));
}

int builtin_cmd(int l,int r)
{
    int i;
    if(l >= r) return SUCCESS;

    if (strcmp(cmds[l], "cd") == 0)
    {
        if (l+2 <= r && cmds[l+1])
	{
	    if(strcmp(cmds[l+1], "~") == 0)
		chdir(getenv("HOME"));
            else if(chdir(cmds[l+1]) == -1)
	    printf("Error:fail to cd\n");
	}
	else printf("Error:miss parameter\n");
        return SUCCESS;
    }

    if (strcmp(cmds[l], "pwd") == 0)
    {
	char wd[4096];
        puts(getcwd(wd, 4096));
        return SUCCESS;
    }

    if(strcmp(cmds[l], "export") == 0)
    {
        if (l+2 <= r && cmds[l+1])
	{
    	    char name[256],value[256];
	    int len1 = strlen(cmds[l+1]);
	    int flag = -1;
            for(i = 0;i < len1;i++)
       	        if(cmds[l+1][i] == '=')
		{
	    	    flag = i;
    	    	    break;
		}
	    	else name[i] = cmds[l+1][i];
	    if(flag == -1) return SUCCESS;
	    name[flag] = '\0';
	    for(i = 0;i < len1-flag;i++)
		value[i] = cmds[l+1][flag+i+1];
	    setenv(name,value,1);
	}
	else printf("Error:miss parameter\n");
	return SUCCESS;
    }

    if(strcmp(cmds[l], "echo") == 0 && l+2 <= r && cmds[l+1])
    {
	if(cmds[l+1][0] == '$')
	{
	    char *envname = &cmds[1][1];
	    char *s = getenv(envname);
	    if(s) printf("%s\n",s);
	    else printf("\n");
	    return SUCCESS;
	}
	else if(strcmp(cmds[l+1], "~") == 0)
	{
	    printf("%s\n",getenv("HOME"));
	    return SUCCESS;
	}
    }

    if (strcmp(cmds[0], "exit") == 0) exit(SUCCESS);
    return(ERROR);
}

int external_cmd(int l,int r)
{
    char *args[128];
    int i;
    if(l >= r) return SUCCESS;
    for(i = l;i < r;i++)
	args[i] = cmds[i];
    args[r] = NULL;
    pid_t pid = fork();
    if(pid == 0)
    {
	execvp(args[l], args+l);
        printf("Error:fail to excute\n");
        exit(ERROR);
    }
    else if(pid == -1)
    {
	printf("Error:fail to fork\n");
	return ERROR;
    }
    else wait(NULL);
    return SUCCESS;
}
