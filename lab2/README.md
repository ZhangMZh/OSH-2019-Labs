# 编写shell程序

## 健壮性

### 拆解命令

将读入的命令从头到尾扫一遍，以空格、`|`、`;`、`&`、`(`和`)`作为分割的标志，把每一段字符串都存到`cmds[num]`中，并加上字符串结束符。如果遇到`|`和`;`，就将它作为一个单独的字符串存起来。这样处理能够忽略多余的空格，且使用这些语法时`&`、`|`或`;`两侧没有空格也能够识别。

```
for(i = 0, j = 0;i < len - 1;i++)
{
	if(cmd[i] != ' ' && cmd[i] != '|' && cmd[i] != ';' && cmd[i] != '(' && cmd[i] != ')' 	 && cmd[i] != '&')
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
	}
}
if(j > 0)
{
	cmds[num][j] = '\0';
	num++;
}
```

![](/home/mz_zhang/lab2/images/2.png)

### 错误处理

上网查得`chdir`、`fork`、`exec`等函数出错时的返回值，然后根据返回值进行相关的错误处理。

![](/home/mz_zhang/lab2/images/7.png)

## 管道与多命令

大致思路是利用递归实现，首先扫一遍拆解好的命令，遇到第一个出现`|`或`;`的地方停止。在子进程中执行它前面的命令，父进程先等待子进程结束，然后将它之后的命令作为一个整体递归执行。如果是管道的话，利用`pipe`与`dup2`函数将子进程的输出与父进程的输入连接起来。简要代码如下：

    //int recurse_cmd(int l,int r)表示执行从cmds[l]到cmds[r-1]的所有命令
    //flag指示cmds[l]到cmds[r-1]中第一次出现|或;的下标，初值为-1
    
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
![](/home/mz_zhang/lab2/images/3.png)

## 环境变量

### export

将`export`后面的字符串以等号为界分解为`name`与`value`，然后调用`setenv`函数修改或增加环境变量。实现较为简单，不再赘述。

### echo $

支持用`echo $NAME`来查询环境变量，直接调用`getenv`便可实现。

![](/home/mz_zhang/lab2/images/1.png)

## 文件重定向

首先扫描一遍命令，判断是哪种类型的文件重定向，`flag=1`表示`>`，`flag=2`表示`>>`，`flag=3`表示`<`。然后调用`open`函数时使用不同的参数，实现不同的功能。

```
O_RDONLY只读模式
O_WRONLY只写模式
O_RDWR读写模式
O_APPEND每次写操作都写入文件的末尾
O_CREAT如果指定文件不存在，则创建这个文件
O_TRUNC如果文件存在，并且以只写/读写方式打开，则清空文件全部内容
```

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
    	fd = open(filename,O_RDWR);
    	dup2(fd,STDIN_FILENO);
    	close(fd);
    }
![](/home/mz_zhang/lab2/images/4.png)

## 其他功能

### echo ~和cd ~

调用`getenv(HOME)`，没什么可说的……

![](/home/mz_zhang/lab2/images/5.png)

### jobs控制

如果命令的最后出现了&，那么就新`fork`一个进程，该命令在子进程后台执行，不影响其他命令在父进程继续执行。

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
![](/home/mz_zhang/lab2/images/6.png)

## Makefile

学习了Makefile的基本语法，然后写了一个四行的Makefile……

```
sh: shell.o
	gcc -static -o sh shell.o
shell.o: shell.c
	gcc -c -o shell.o shell.c
```

