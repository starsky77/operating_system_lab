#include <stdio.h>
#include <string.h>

int main()
{
	char ch;
	int output=0;
	FILE *fp;
	
	//内核模块输出的日志文件 
	fp=fopen("/var/log/kern.log","r");
	if(fp==NULL)
    {
        //找不到文件，返回错误
		printf("Can't open log file!\n");
		return -1;
	}
	
	//读取日志文件
	fseek(fp,0,SEEK_SET);
	ch=fgetc(fp);
	//找到内核模块输出记录开头
    while(ch!=EOF)
    {
        while(ch!=EOF)
        {
            //输出记录开头的特殊标记为"#$#" 
            if(ch=='#')
            {
                ch=fgetc(fp);
                if(ch=='$')
                {
                    ch=fgetc(fp);
                    if(ch=='#')
                    {
                        printf("Find my module log head!\n");
                        break;
                    }
                }
            }
            ch=fgetc(fp);
        }
        //打印出内核模块的输出记录
        while(ch!=EOF){
            if(ch=='@')
            {
                output=1;
            }
            else if(ch=='\n')
            {
                printf("\n");
                output=0;
            }
            else if(ch=='#')
            {//输出记录结尾的特殊标记为"#*#" 
                ch=fgetc(fp);
                if(ch=='*')
                {
                    ch=fgetc(fp);
                    if(ch=='#')
                    {
                        printf("My module log ends!\n");
                        break;
                    }
                }
            }
            else if(output)
            {
                printf("%c",ch);
            }
            ch=fgetc(fp);
        }
    }
	printf("Program end\n");
	//关闭日志文件
	fclose(fp); 
	return 0;
}