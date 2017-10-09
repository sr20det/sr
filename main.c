#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
	ino_t i;
	pid_t p;
	int   f;
	union {
		char *n;
		long long *t;
	};
} DUnit;

enum {
	UT_RUN  = 0x6e7572LL,
	UT_DOWN = 0x6e776f64LL
};

static int
dread(DIR *dp, DUnit *du, const int dL)
{
	struct dirent *de;
	int i, z, n;

	seekdir(dp, 1);
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		for (z=i=0; i<dL; i++)
		{
			if (du[i].i == de->d_ino)
				break;
			if (!(du[i].i|z))
				z = i;
		}
		if (i == dL && z)
		{
			du[z].i = de->d_ino;
			du[z].f = !(de->d_type ^ DT_DIR);
			n = MAX(strlen(de->d_name)+1, sizeof(long long));
			du[z].n = calloc(n, 1);
			strcpy(du[z].n, de->d_name);
		}
	}
	return 0;
}

static int
runall(char *arg0, int argl, DUnit *du, const int dL)
{
	int i;

	for (i=0; i<dL; i++)
	{
		if ((!du[i].i) | du[i].p)
			continue;
		du[i].p = fork();
		if (du[i].p)
			continue;
		if (du[i].f == 1)
		{
			strncpy(arg0, du[i].n, argl);
			if (chdir(du[i].n))
				return -1;
			return 1;
		}
		switch (*du[i].t)
		{
			case UT_RUN:
				execl("run", arg0, NULL);
				return -1;
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	DIR  *dp=NULL;
	const int dL=256;
	int i, argl;
	pid_t p;
	DUnit du[dL];
	
	argl = argv[argc-1] + strlen(argv[argc-1]) - argv[0];
	if (chdir( argc>1 ? argv[1] : "/var/sr" ))
		goto sr_9;
	argc = 1;

 sr_0:
	memset(du, 0, sizeof(DUnit) * dL);
	dp = opendir(".");
	if (!dp)
		return -1;

 sr_1:
	dread(dp, du, dL);
	switch (runall(argv[0], argl, du, dL))
	{
		case 0:
			break;
		case 1:
			closedir(dp);
			for (i=0; i<dL; i++)
				if (du[i].n)
					free(du[i].n);
			goto sr_0;
		case -1:
		default:
			sleep(5);
			goto sr_9;
	}

	while ((p = wait(NULL)) != -1)
	{
		for (i=0; du[i].p != p && i<dL; i++)
			;
		du[i].p = du[i].i = 0;
		free(du[i].n);
		sleep(3);
		goto sr_1;
	}

 sr_9:
	closedir(dp);
	return 0;
}
