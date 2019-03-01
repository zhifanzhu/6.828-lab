#include <inc/lib.h>

void
usage(void)
{
	printf("Usage: touch ... FILE...\n");
	exit();
}

void
umain(int argc, char **argv)
{
    int f, i;

    binaryname = "touch";
    if (argc == 1)
        usage();
    else
        for (i = 1; i < argc; i++) {
            f = open(argv[i], O_WRONLY|O_CREAT);
            if (f < 0)
                printf("can't create %s: %e\n", argv[i], f);
            close(f);
        }
}
