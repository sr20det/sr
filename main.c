#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


#define MIN(a,b) (((a)<(b))?(a):(b))
#define UT_RUN	0x6e7572LL
#define UT_DONW 0x6e776f64LL

typedef struct {
	ino_t	i;
	pid_t	p;
	int		f;
	union {
		long long t;
		char n[sizeof(long long)];
	};
} DUnit;

int
dread(DIR *dp, DUnit *du, const int l)
{
	struct dirent *de;
	int i, z, n;

	seekdir(dp, 1);
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		for (z=i=0; i<l; i++)
		{
			if (du[i].i == de->d_ino)
				break;
			if (du[i].i == 0)
				z = i;
		}
		if (i == l && z)
		{
			du[z].i = de->d_ino;
			du[z].f = !(de->d_type ^ DT_DIR);
			n = MIN(strlen(de->d_name), sizeof(du[z].n));
			strncpy(du[z].n, de->d_name, n);
		}
	}
	return 0;
}

int
runall(char *arg0, int argl, DUnit *du, const int dL)
{
	int i;

	argl = MIN(argl, sizeof(du[i].n));
	for (i=0; i<dL; i++)
	{
		if (du[i].i == 0)
			continue;
		du[i].p = fork();
		if (du[i].p != 0)
			continue;
		if (du[i].f == 1)
		{
			strncpy(arg0, du[i].n, argl);
			return 1;
		}
		switch (du[i].t)
		{
			case UT_RUN:
				execl("run", arg0, NULL);
				return 2;
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	DIR *dp;
	struct dirent *de;
	char *dn;
	const int dL=256;
	int i, argl;
	pid_t p;
	DUnit du[dL];
	
	argl = argv[argc-1] + strlen(argv[argc-1]) - argv[0];
	dn	 = strdup( argc>1 ? argv[1] : "/var/sr" );
	argc = 1;

 sr_0:
	memset(du, 0, sizeof(DUnit) * dL);
	dp = opendir(dn);
	if (dp == NULL)
		return -1;
	if (chdir(dn))
		goto sr_9;

 sr_1:
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		if (de->d_type == DT_DIR)
		{
			for (i=0; du[i].i != de->d_ino && i<dL; i++)
				;
			if (i != dL)
				continue;
			for (i=0; du[i].i && i<dL; i++)
				;
			if (i == dL)
				break;
			du[i].p = fork();
			if (du[i].p == 0)
			{
				closedir(dp);
				free(dn);
				dn = strdup(de->d_name);
				strncpy(argv[0], dn, argl);
				goto sr_0;
			}
			else
				du[i].i = de->d_ino;
		}
		else
		{
			if (de->d_type == DT_REG)
				if (strcmp(de->d_name, "run") == 0)
				{
					for (i=0; du[i].i != de->d_ino && i<dL; i++)
						;
					if (i != dL)
						continue;
					for (i=0; du[i].i && i<dL; i++)
						;
					if (i == dL)
						break;
					du[i].p = fork();
					if (du[i].p == 0)
						execl("run", dn, NULL);
					else
						du[i].i = de->d_ino;
				}
		}
	}
	while ((p = wait(NULL)) != -1)
	{
		for (i=0; du[i].p != p && i<dL; i++)
			;
		du[i].i = 0;
		sleep(3);
		seekdir(dp, 1);
		goto sr_1;
	}

 sr_9:
	closedir(dp);
	return 0;
}
