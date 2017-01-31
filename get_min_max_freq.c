#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#define MSR_PLATFORM_INFO		0x000000ce
#define MSR_IA32_POWER_CTL		0x000001fc

int base_cpu = 0;
double bclk=0.1;

static long long read_msr(int fd, int which) {

        uint64_t data;

        if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
                perror("rdmsr:pread");
                exit(127);
        }

        return (long long)data;
}

static int open_msr(int core) {

        char msr_filename[BUFSIZ];
        int fd;

        sprintf(msr_filename, "/dev/cpu/%d/msr", core);
        fd = open(msr_filename, O_RDONLY);
        if ( fd < 0 ) {
                if ( errno == ENXIO ) {
                        fprintf(stderr, "rdmsr: No CPU %d\n", core);
                        exit(2);
                } else if ( errno == EIO ) {
                        fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
                                        core);
                        exit(3);
                } else {
                        perror("rdmsr:open");
                        fprintf(stderr,"Trying to open %s\n",msr_filename);
                        exit(127);
                }
        }

        return fd;
}


void get_min_max_freq_info(float* min, float* max)
{
	unsigned long long msr;
	unsigned int ratio;
	int fd =open_msr(0);
	long long result = read_msr(fd,MSR_PLATFORM_INFO);

	ratio = (result >> 40) & 0xFF;
	*min = ratio * bclk;
	//printf("base efficiency:%.2fGHz\n",min);	
	
	ratio = (result >> 8) & 0xFF;
	*max = ratio * bclk;
	//printf("max efficiency:%.2fGHz\n",max);	

	return;
}

int main(int argc, char *argv[])
{
float min,max;
get_min_max_freq_info(&min,&max);
printf("base efficiency:%.2fGHz\n",min); 
printf("max efficiency:%.2fGHz\n",max); 
return 0;
}

