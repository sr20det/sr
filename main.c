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
	union {
		char *n;
		long long *t;
	};
	void *cu;
	int   cl;
	int   f;
} DUnit;

enum {
	UT_RUN  = 0x6e7572LL,
	UT_DOWN = 0x6e776f64LL
};
enum {
	UF_DIR = 1,
	UF_IGN = 2,
	UF_RUN = 4
};

static int
dread(DUnit *, const int);

static int
dread(DUnit *du, const int dL)
{
	DIR *dp;
	struct dirent *de;
	int i, z, n;
	DUnit *dout;

	dp = opendir(du->n);
	if (!dp)
		return -1;
	if (!du->cu)
		du->cu = calloc(dL, sizeof(DUnit));
	dout = du->cu;
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		for (z=i=0; i<dL; i++)
		{
			if (dout[i].i == de->d_ino)
				break;
			if (!(dout[i].i|z))
				z = i;
		}
		if (i == dL && z)
		{
			dout[z].i = de->d_ino;
			n = MAX(strlen(de->d_name)+1, sizeof(long long));
			dout[z].n = calloc(n, 1);
			strcpy(dout[z].n, de->d_name);
			dout[z].f = !(de->d_type ^ DT_DIR);
			switch (*dout[z].t)
			{
				case UT_DOWN:
					du->f |= UF_IGN;
					break;
			}
		}
	}
	closedir(dp);
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
		dread(&du[i], dL);
		if (du[i].f == UF_IGN)
			continue;
		du[i].p = fork();
		if (du[i].p)
			continue;
		if (du[i].f == UF_DIR)
		{
			strncpy(arg0, du[i].n, argl);
			if (chdir(du[i].n))
				return -1;
			return UF_DIR;
		}
		if (*du[i].t == UT_RUN)
		{
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
	DUnit *du = {0}, *cu;
	
	argl = argv[argc-1] + strlen(argv[argc-1]) - argv[0];
	if (chdir( argc>1 ? argv[1] : "/var/sr" ))
		goto sr_9;
	argc = 1;
	du->n = ".";
	dread(du, dL);

 sr_0:
	//memset(du, 0, sizeof(DUnit) * dL);

 sr_1:
	switch (runall(argv[0], argl, du, dL))
	{
		case 0:
			break;
		case 1:
			for (i=0; i<dL; i++)
				free(cu[i].n);
			goto sr_0;
		case -1:
		default:
			sleep(5);
			goto sr_9;
	}
	cu = du->cu;
	while ((p = wait(NULL)) != -1)
	{
		for (i=0; cu[i].p != p && i<dL; i++)
			;
		cu[i].p = cu[i].i = 0;
		free(cu[i].n);
		sleep(3);
		goto sr_1;
	}

 sr_9:
	return 0;
}
