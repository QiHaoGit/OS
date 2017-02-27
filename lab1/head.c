#include "head.h"
/*
char time[40];
printf("%s", get_time(time));
*/
// 得到当前时间
char* get_time(char* s) {
    struct tm *local;
    time_t t;
    // time_t time(time_t * timer);如果参数为空（NULL），函数将只通过返回值返回现在的日历时间
    t = time(NULL);
    // localtime()函数是将日历时间转化为本地时间
    local = localtime(&t);
    sprintf(s, "Local time is: %02d:%02d:%02d\n", local->tm_hour, local->tm_min, local->tm_sec);
    return s;
}
// 得到当前CPU信息
CPU get_cpu() {
	char buf[128];
	FILE* fp = fopen("proc/stat", "r");
	fgets(buf, sizeof(buf), fp);
	CPU cpu;
	sscanf(buf, "%s %u %u %u %u %u %u %u", cpu.name, cpu.user, cpu.nice, \
		   cpu.system, cpu.idle, cpu.iowait, cpu.irq, cpu.softirq);
	fclose(fp);
	return cpu;
}
// 计算CPU占用率
double cacl_cpu(CPU *cpu1, CPU *cpu2) {






	double cpu_used = 0;
	return cpu_used;
}