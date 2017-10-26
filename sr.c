#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define FREE(p)  { if (p) { free(p); p=NULL; } }

typedef struct {
	ino_t i;
	pid_t p;
	union {
		char *n;
		long long *t;
	};
	void *cu; /*DUnit **/
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
};

#ifdef DEBUG
#define dump(u) \
 fprintf(stderr, "%-4d %4d:%-8s %-8s n:%-8s i:%-8lx p:%-4d f:%-4x l:%-2x\n", \
  getpid(), __LINE__, __func__, #u, (u)->n, (u)->i, (u)->p, (u)->f, (u)->cl)
#else
#define dump(u)
#endif

static void
dpeel(DUnit *du, int ei)
{
	int i, dL = du->cl;
	DUnit *cu = du->cu;

	dump(du);
	FREE(du->n)
	if (ei!=-1)
	{
		memcpy(du, &cu[ei], sizeof(DUnit));
		cu[ei].cu = NULL;
		du->n = NULL;
	}
	else
		memset(du, 0, sizeof(DUnit));
	for (i=dL; i--;)
	{
		if (cu[i].cu)
			dpeel(&cu[i], -1);
		else
			FREE(cu[i].n)
	}
	FREE(cu)
}

static int
dread(DUnit *du)
{
	DIR *dp;
	struct dirent *de;
	int i, z, n;
	DUnit *cu;

	dump(du);
	dp = opendir(du->n ? du->n : ".");
	if (!dp)
		return -1;
	if (!du->cl)
		du->cl = 8;
	if (!du->cu)
		du->cu = calloc(du->cl, sizeof(DUnit));
	cu = du->cu;
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		for (z=i=du->cl; i--;)
		{
			if (!(cu[i].i))
				z = i;
			else
				if (cu[i].i == de->d_ino)
					break;
		}
		if ((i!=-1) || (z==du->cl))
			continue;
		cu[z].i = de->d_ino;
		n = MAX(strlen(de->d_name)+1, sizeof(long long));
		cu[z].n = calloc(n, 1);
		strcpy(cu[z].n, de->d_name);
		cu[z].f = (de->d_type == DT_DIR ? UF_DIR : 0);
		if (cu[z].f)
			continue;
		switch (*(cu[z].t))
		{
			case UT_DOWN:
				du->f |= UF_IGN;
				break;
		}
	}
	closedir(dp);
	return 0;
}

static int
runall(char *arg0, int argl, DUnit *du)
{
	int i;
	DUnit *cu = du->cu;

	dump(du);
	for (i=du->cl; i--;)
	{
		if ((!cu[i].i) | cu[i].p)
			continue;
		if (cu[i].f & UF_DIR)
			dread(&cu[i]);
		if (cu[i].f & UF_IGN)
			continue;
		dump(&cu[i]);
		cu[i].p = fork();
		if (cu[i].p)
			continue;
		if (cu[i].f & UF_DIR)
		{
			strncpy(arg0, cu[i].n, argl);
			if (chdir(cu[i].n))
				return -1;
			dpeel(du, i);
			return 1;
		}
		if (*cu[i].t == UT_RUN)
		{
			dump(&cu[i]);
			execl(cu[i].n, arg0, NULL);
			return -1;
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int i, argl;
	pid_t p;
	DUnit du = {0}, *cu;

	argl = argv[argc-1] + strlen(argv[argc-1]) - argv[0];
	if (chdir( argc>1 ? argv[1] : "/var/sr" ))
		goto sr_9;
	argc = 1;

 sr_0:
	dread(&du);

 sr_1:
	switch (runall(argv[0], argl, &du))
	{
		case 0:
			break;
		case 1:
			dump(&du);
			goto sr_1;
		case -1:
		default:
			sleep(1);
			goto sr_9;
	}
	cu = du.cu;
	while ((p = wait(NULL)) != -1)
	{
		for (i=0; i<du.cl && (cu[i].p != p); i++)
			;
		if (i==du.cl)
			break;
		dump(&cu[i]);
		dpeel(&cu[i], -1);
		sleep(1);
		goto sr_0;
	}

 sr_9:
	return 0;
}
