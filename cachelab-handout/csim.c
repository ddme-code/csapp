#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "cachelab.h"
#define MAX_CACHE_SET 32
#define MAX_CACHE_LINE 32
#define DEBUG 0
int hit_cnt;
int miss_cnt;
int eviction_cnt;
int cmd_cnt;
int s,E,b;
struct cache_line
{
    int valid_bit;        //这个位置是否有数据
    int tag_bit;          //标志位
    int last_time;        //LRU参考
}cache[MAX_CACHE_SET][MAX_CACHE_LINE];

void init();              //初始化
void args_parse(int argc, char *argv[]);    //分析输入参数
void cmd_parse(char *cmd,long long addr);   //分析要执行哪条命令，trace数据对应的命令
void exec_cmd(long long addr);              //执行命令
void addr_parse(long long addr,int *tag_bit,int *set_id);   //存入cache中的哪个位置
int main(int argc,char *argv[]);

void init(){
    hit_cnt=miss_cnt=eviction_cnt=cmd_cnt=0;
    memset(cache,0,sizeof(cache));
    return;
}
void args_parse(int argc, char *argv[]){
    char ch;
    while((ch=getopt(argc, argv,"s:E:b:t:"))!=-1){
        switch (ch)
        {
        case 's':
            s=atoi(optarg);
            break;
        case 'E':
            E=atoi(optarg);
            break;
        case 'b':
            b=atoi(optarg);
            break;
        case 't':
            freopen(optarg, "r", stdin);
        }
    }
    return;
}
void cmd_parse(char *cmd,long long addr){
    switch (cmd[0])
    {
    case 'I':
        break;
    case 'L':
        exec_cmd(addr);
        break;
    case 'S':
        exec_cmd(addr);
        break;
    case 'M':
        exec_cmd(addr);
        exec_cmd(addr);
        break;
    }
    return;
}
void addr_parse(long long addr,int *tag_bit,int *set_id){
    int tmp=0;
    for(int i=0;i<s;i++){               //对数据存放位置的计算，约定
        tmp=(tmp<<1)+1;
    }
    *set_id=((int)(addr>>b)&tmp)%(1<<s);
    *tag_bit=(int)(addr>>(b+s));
    return;
}
void exec_cmd(long long addr){
    int tag_bit,set_id;
    cmd_cnt++;
    addr_parse(addr,&tag_bit,&set_id);
    if(DEBUG) printf("%d %d ",set_id,tag_bit);
    for(int i=0;i<E;i++){
        if((cache[set_id][i].valid_bit)   //命中则return
        &&(cache[set_id][i].tag_bit)==tag_bit
        ){
            cache[set_id][i].last_time=cmd_cnt;
            hit_cnt++;
            if(DEBUG) printf("hit\n");
            return;
        }
    }
    miss_cnt++;
    for(int i=0;i<E;i++){
        if(!cache[set_id][i].valid_bit){    //未命中，且有空位
            cache[set_id][i].valid_bit=1;
            cache[set_id][i].tag_bit=tag_bit;
            cache[set_id][i].last_time=cmd_cnt;
            if(DEBUG) printf("miss\n");
            return;
        }
    }
    eviction_cnt++;                         //未命中且无空位
    int victim_id=0;
    for(int i=0;i<E;i++){
        if(cache[set_id][i].last_time<cache[set_id][victim_id].last_time){
            victim_id=i;                    //求出最早未使用
        }
    }
    cache[set_id][victim_id].tag_bit=tag_bit;   //将其替换
    cache[set_id][victim_id].last_time=cmd_cnt;
    if(DEBUG) printf("miss eviction\n");
    return;
}
int main(int argc,char *argv[])
{
    init();
    args_parse(argc,argv);
    char cmd[10];
    long long addr;
    int blocksize;
    while(~scanf("%s %llx,%d",cmd,&addr,&blocksize)){
        cmd_parse(cmd,addr);                  //对trace中数据格式的分析
    }
    printSummary(hit_cnt, miss_cnt, eviction_cnt);    //输出结果
    return 0;
}

