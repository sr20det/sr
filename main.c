#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
	ino_t i;
	pid_t p;
	int n, f;
} dUnit;

int
dread(DIR *dp, dUnit *du, int l)
{
	struct dirent *de;
	int i, z;

	seekdir(dp, 1);
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		for (z=i=0; i<l; i++)
		{
			if (du[i].i == 0)
				z = i;
			else
				if (du[i].i == de->d_ino)
					break;
		}
		if (i == l && z)
		{
			du[z].i = de->d_ino;
			du[z].f = !(de->d_type ^ DT_DIR);
			strncpy((char*)&(du[z].n), de->d_name, sizeof(du[z].n));
		}
	}
	return 0;
}

int
runall(char *arg0, int argl, dUnit *du, int l)
{
	int i;

	argl = MIN(argl, sizeof(du[i].n));
	for (i=0; i<l; i++)
	{
		if (du[i].i == 0)
			continue;
		du[i].p = fork();
		if (du[i].p == 0)
		{
			strncpy(arg0, (char*)&du[i].n, argl);
			return 0;
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
	dUnit du[dL];
	
	argl = argv[argc-1] + strlen(argv[argc-1]) - argv[0];
	dn	 = strdup( argc>1 ? argv[1] : "/var/sr" );
	argc = 1;

 sr_0:
	for (i=0; i<dL; i++)
		memset(&du[i], 0, sizeof(dUnit));
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
