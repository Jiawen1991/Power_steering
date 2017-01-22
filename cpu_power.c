/*
gcc -o cpu_power cpu_power.c -lm
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>

#define MSR_RAPL_POWER_UNIT             0x606

/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT        0x610
#define MSR_PKG_ENERGY_STATUS           0x611
#define MSR_PKG_PERF_STATUS             0x613
#define MSR_PKG_POWER_INFO              0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT             0x638
#define MSR_PP0_ENERGY_STATUS           0x639
#define MSR_PP0_POLICY                  0x63A
#define MSR_PP0_PERF_STATUS             0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT             0x640
#define MSR_PP1_ENERGY_STATUS           0x641
#define MSR_PP1_POLICY                  0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT            0x618
#define MSR_DRAM_ENERGY_STATUS          0x619
#define MSR_DRAM_PERF_STATUS            0x61B
#define MSR_DRAM_POWER_INFO             0x61C


static int open_msr(int core) {

        char msr_filename[BUFSIZ];
        int fd;

        sprintf(msr_filename, "/dev/cpu/%d/msr", core);
        //fd = open(msr_filename, O_RDONLY);
        fd = open(msr_filename, O_RDWR);
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

static long long read_msr(int fd, int which) {

        uint64_t data;

        if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
                perror("rdmsr:pread");
                exit(127);
        }

        return (long long)data;
}

static long long write_msr(int fd, int which, uint64_t data) {

        if ( -1 == pwrite(fd, &data, sizeof data, which)) {
                perror("rdmsr:pwrite");
                exit(127);
        }

        return (long long) data;
}

#define CPU_SANDYBRIDGE         42
#define CPU_SANDYBRIDGE_EP      45
#define CPU_IVYBRIDGE           58
#define CPU_IVYBRIDGE_EP        62
#define CPU_HASWELL             60      // 69,70 too?
#define CPU_HASWELL_EP          63
#define CPU_BROADWELL           61      // 71 too?
#define CPU_BROADWELL_EP        79
#define CPU_BROADWELL_DE        86
#define CPU_SKYLAKE             78      // 94 too?

static int detect_cpu(void) {

        FILE *fff;

        int family,model=-1;
        char buffer[BUFSIZ],*result;
        char vendor[BUFSIZ];

        fff=fopen("/proc/cpuinfo","r");
        if (fff==NULL) return -1;

        while(1) {
                result=fgets(buffer,BUFSIZ,fff);
                if (result==NULL) break;

                if (!strncmp(result,"vendor_id",8)) {
                        sscanf(result,"%*s%*s%s",vendor);

                        if (strncmp(vendor,"GenuineIntel",12)) {
                                printf("%s not an Intel chip\n",vendor);
                                return -1;
                        }
                }

                if (!strncmp(result,"cpu family",10)) {
                        sscanf(result,"%*s%*s%*s%d",&family);
                        if (family!=6) {
                                printf("Wrong CPU family %d\n",family);
                                return -1;
                        }
                }

                if (!strncmp(result,"model",5)) {
                        sscanf(result,"%*s%*s%d",&model);
                }

        }

        fclose(fff);

        printf("\nFound ");

        switch(model) {
                case CPU_SANDYBRIDGE:
                        printf("Sandybridge");
                        break;
                case CPU_SANDYBRIDGE_EP:
                        printf("Sandybridge-EP");
                        break;
                case CPU_IVYBRIDGE:
                        printf("Ivybridge");
                        break;
                case CPU_IVYBRIDGE_EP:
                        printf("Ivybridge-EP");
                        break;
                case CPU_HASWELL:
                        printf("Haswell");
                        break;
                case CPU_HASWELL_EP:
                        printf("Haswell-EP");
                        break;
                case CPU_BROADWELL:
                        printf("Broadwell");
                        break;
                default:
                        printf("Unsupported model %d\n",model);
                        model=-1;
                        break;
        }

        printf(" Processor type\n");

        return model;
}

#define MAX_CPUS        1024
#define MAX_PACKAGES    16

static int total_cores=0,total_packages=0;
static int package_map[MAX_PACKAGES];

static int detect_packages(void) {

        char filename[BUFSIZ];
        FILE *fff;
        int package;
        int i;

        for(i=0;i<MAX_PACKAGES;i++) package_map[i]=-1;

        printf("\t");
        for(i=0;i<MAX_CPUS;i++) {
                sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
                fff=fopen(filename,"r");
                if (fff==NULL) break;
                fscanf(fff,"%d",&package);
                printf("%d (%d)",i,package);
                if (i%8==7) printf("\n\t"); else printf(", ");
                fclose(fff);

                if (package_map[package]==-1) {
                        total_packages++;
                        package_map[package]=i;
                }

        }

        printf("\n");

        total_cores=i;

        printf("\tDetected %d cores in %d packages\n\n",
                total_cores,total_packages);

        return 0;
}


        int fd;
        long long result;
        double power_units,time_units;
        double cpu_energy_units[MAX_PACKAGES],dram_energy_units[MAX_PACKAGES];
        double package_before[MAX_PACKAGES],package_after[MAX_PACKAGES];
        double pp0_before[MAX_PACKAGES],pp0_after[MAX_PACKAGES];
        double pp1_before[MAX_PACKAGES],pp1_after[MAX_PACKAGES];
        double dram_before[MAX_PACKAGES],dram_after[MAX_PACKAGES];
        double thermal_spec_power,minimum_power,maximum_power,time_window;
        int j;
	int cpu_model;

void power_capping(int min_power, int max_power) {
	cpu_model=detect_cpu();
        detect_packages();

printf("\tPower Capping:\n");

        if (cpu_model<0) {
                printf("\tUnsupported CPU model %d\n",cpu_model);
                return ;
        }

        for(j=0;j<total_packages;j++) {

                fd=open_msr(package_map[j]);

                /* Calculate the units used */
                result=read_msr(fd,MSR_RAPL_POWER_UNIT);

                power_units=pow(0.5,(double)(result&0xf));
                cpu_energy_units[j]=pow(0.5,(double)((result>>8)&0x1f));
                time_units=pow(0.5,(double)((result>>16)&0xf));

                /* On Haswell EP the DRAM units differ from the CPU ones */
                if (cpu_model==CPU_HASWELL_EP) {
                        dram_energy_units[j]=pow(0.5,(double)16);
                }
                else {
                        dram_energy_units[j]=cpu_energy_units[j];
                }
/*			
		printf("\t\tPower units = %.3fW\n",power_units);
                printf("\t\tCPU Energy units = %.8fJ\n",cpu_energy_units[j]);
                printf("\t\tDRAM Energy units = %.8fJ\n",dram_energy_units[j]);
                printf("\t\tTime units = %.8fs\n",time_units);
                printf("\n");
*/		
/* Change package power limit */
                result=read_msr(fd,MSR_PKG_RAPL_POWER_LIMIT);
	      /*long long result1 = (long long)(0x005F86E0001B81B0);*/
		long long result1 = (long long)(0x0001800000018000);
		result1 = result1 + (min_power/power_units) + (max_power/power_units * pow(16,8));
		result1=write_msr(fd,MSR_PKG_RAPL_POWER_LIMIT,result1);
                printf("\t\tPackage power limits are %s\n", (result1 >> 63) ? "locked" : "unlocked");
                double pkg_power_limit_1 = power_units*(double)((result1>>0)&0x7FFF);
                double pkg_time_window_1 = time_units*(double)((result1>>17)&0x007F);
		//#1
                printf("\t\tPackage power min(#1): %.3fW (%s, %s)\n",
                        pkg_power_limit_1,
                        (result1 & (1LL<<15)) ? "enabled" : "disabled",
                        (result1 & (1LL<<16)) ? "clamped" : "not_clamped");
                double pkg_power_limit_2 = power_units*(double)((result1>>32)&0x7FFF);
                double pkg_time_window_2 = time_units*(double)((result1>>49)&0x007F);
		//#2
                printf("\t\tPackage power max(#2): %.3fW (%s, %s)\n",
                        pkg_power_limit_2,
                        (result1 & (1LL<<47)) ? "enabled" : "disabled",
                        (result1 & (1LL<<48)) ? "clamped" : "not_clamped");
/*		 Show package power info 
                result=read_msr(fd,MSR_PKG_POWER_INFO);
                thermal_spec_power=power_units*(double)(result&0x7fff);
                printf("\t\tPackage thermal spec: %.3fW\n",thermal_spec_power);
                minimum_power=power_units*(double)((result>>16)&0x7fff);
                printf("\t\tPackage minimum power: %.3fW\n",minimum_power);
                maximum_power=power_units*(double)((result>>32)&0x7fff);
                printf("\t\tPackage maximum power: %.3fW\n",maximum_power);
                time_window=time_units*(double)((result>>48)&0x7fff);
                printf("\t\tPackage maximum time window: %.6fs\n",time_window);
*/	
                close(fd);

        }
        printf("\n");
}

void energy_measure_before()
{
        for(j=0;j<total_packages;j++) {

                fd=open_msr(package_map[j]);

                /* Package Energy */
                result=read_msr(fd,MSR_PKG_ENERGY_STATUS);
                package_before[j]=(double)result*cpu_energy_units[j];

                /* PP0 energy */
                /* Not available on Haswell-EP? */
                result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
                pp0_before[j]=(double)result*cpu_energy_units[j];

                /* PP1 energy */
                /* not available on *Bridge-EP */
                if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
                        (cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {

                        result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
                        pp1_before[j]=(double)result*cpu_energy_units[j];
                }
/* Broadwell have DRAM support too                              */
                if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP) ||
                        (cpu_model==CPU_HASWELL_EP) ||
                        (cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {

                        result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
                        dram_before[j]=(double)result*dram_energy_units[j];
                }

                close(fd);
        }
}

void energy_measure_after()
{
printf("\tPower Measurement:\n");
        for(j=0;j<total_packages;j++) {

                fd=open_msr(package_map[j]);

                printf("\tPackage %d:\n",j);

                result=read_msr(fd,MSR_PKG_ENERGY_STATUS);
                package_after[j]=(double)result*cpu_energy_units[j];
                printf("\t\tPackage energy: %.6fJ\n",
                        package_after[j]-package_before[j]);

                result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
                pp0_after[j]=(double)result*cpu_energy_units[j];
                printf("\t\tPowerPlane0 (cores): %.6fJ\n",
                        pp0_after[j]-pp0_before[j]);

                if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
                        (cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {
                        result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
                        pp1_after[j]=(double)result*cpu_energy_units[j];
                        printf("\t\tPowerPlane1 (on-core GPU if avail): %.6f J\n",
                                pp1_after[j]-pp1_before[j]);
                }

                if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP) ||
                        (cpu_model==CPU_HASWELL_EP) ||
                        (cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {

                        result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
                        dram_after[j]=(double)result*dram_energy_units[j];
                        printf("\t\tDRAM: %.6fJ\n",
                                dram_after[j]-dram_before[j]);
                }
                close(fd);
        }
        printf("\n");
}
/*
int main(int argc, char **argv) {

	power_capping(54,220);
	power_measure_before();
        printf("\n\tSleeping 1 second\n\n");
        sleep(1);
	power_measure_after();
}
*/
