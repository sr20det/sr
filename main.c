#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


int
main(int argc, char *argv[])
{
	DIR *dp;
	struct dirent *de;
	int i, argl;
	pid_t p;
	const int dL=256;
	char *dn;
	struct {
		pid_t p;
		ino_t i;
	} dm[dL];

	argl = argv[argc-1] + strlen(argv[argc-1]) - argv[0];
	dn	 = strdup( argc>1 ? argv[1] : "/var/sr" );

 sr_0:
	for (i=0; i<dL; i++)
	{
		dm[i].i=0;
		dm[i].p=0;
	}
	dp = opendir(dn);
	if (dp == NULL)
		return 0;
	if (chdir(dn))
		goto sr_9;

 sr_1:
	while ((de = readdir(dp)))
	{
		if (de->d_name[0] == '.')
			continue;
		if (de->d_type == DT_DIR)
		{
			for (i=0; dm[i].i != de->d_ino && i<dL; i++)
				;
			if (i != dL)
				continue;
			for (i=0; dm[i].i && i<dL; i++)
				;
			if (i == dL)
				break;
			dm[i].p = fork();
			if (dm[i].p == 0)
			{
				closedir(dp);
				free(dn);
				dn = strdup(de->d_name);
				argc = 1;
				strncpy(argv[0], dn, argl);
				goto sr_0;
			}
			else
				dm[i].i = de->d_ino;
		}
		else
		{
			if (de->d_type == DT_REG)
				if (strcmp(de->d_name, "run") == 0)
				{
					for (i=0; dm[i].i != de->d_ino && i<dL; i++)
						;
					if (i != dL)
						continue;
					for (i=0; dm[i].i && i<dL; i++)
						;
					if (i == dL)
						break;
					dm[i].p = fork();
					if (dm[i].p == 0)
						execl("run", dn, NULL);
					else
						dm[i].i = de->d_ino;
				}
		}
	}
	while ((p = wait(NULL)) != -1)
	{
		for (i=0; dm[i].p != p && i<dL; i++)
			;
		dm[i].i = 0;
		sleep(3);
		seekdir(dp, 1);
		goto sr_1;
	}

 sr_9:
	closedir(dp);
	return 0;
}
