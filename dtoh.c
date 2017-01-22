#include<stdio.h>

#include<math.h>

long long dec_hex_1(int dec)
{
printf("%x\n",dec);
int rem[4],i=0,length=0;
while(dec>0)
   {
      rem[i]=dec%16;
      dec=dec/16;
      i++;
      length++;
   }

long long hex = 0x0;
for(i = 0; i<=length-1; i++)
        hex += rem[i] * pow(16,i);

printf("%llx\n",hex);
}
 
void dec_hex(long int num)   // Function Definition
{
long int rem[50],i=0,length=0;
 
while(num>0)
   {
      rem[i]=num%16;
      num=num/16;
      i++;
      length++;
   }

int hex = 0x0 + rem[0]+ rem[1]*16 + rem[2] * 16 *16;
printf("%X\n",hex);
 
printf("Hexadecimal number : ");
for(i=length-1;i>=0;i--)
  {
    switch(rem[i])
    {
      case 10:
          printf("A");
          break;
      case 11:
          printf("B");
          break;
      case 12:
          printf("C");
          break;
      case 13:
          printf("D");
          break;
      case 14:
          printf("E");
          break;
      case 15:
          printf("F");
          break;
      default :
         printf("%ld",rem[i]);
    }
  }
printf("\n");
}


int main() {
long long result = (long long)(0x005F8000001B8000);
int dec = 432;
long long dec1 = 1760*pow(16,8); 
result = result + dec + dec1;
printf("%llx\n",result);
	return 0;
}
