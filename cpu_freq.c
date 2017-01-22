#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_cpu_freq(int from, int to, float *freq)
{
char cmd[100]  = "";
char cmd1[100] = "sudo cpupower -c ";
char cmd2[100] = " frequency-set -f ";
char cpu_cores[10] = "";
char cpu_freq[10]  = "";
char tmp[2] = "";

sprintf(cpu_cores, "%d", from);
strcat(cpu_cores,"-");
sprintf(tmp, "%d", to);
strcat(cpu_cores,tmp);

sprintf(cpu_freq,"%f",*freq);
strcat(cpu_freq,"GHz");

strcat(cmd,cmd1);
strcat(cmd,cpu_cores);
strcat(cmd,cmd2);
strcat(cmd,cpu_freq);

printf("\tControl CPU frequency:\n");
printf("\t\tCores:%d to %d in %.2fGHz\n",from,to,*freq);
//printf("cmd:%s\n",cmd);

system(cmd);
}

/*
int main(int argc, char *argv[])
{
set_cpu_freq(0,71,1.2);

return 0;
}
*/
