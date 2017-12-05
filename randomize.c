/* randomize argv's - stolen from beorn */
main(ac, av)
        int     ac;
        char    *av[ ];
{
        int i, j;

        srandom((getuid( ) << 7) ^ (getpid( ) << 4) ^ time(0));

        for (i = ac - 1; i; i--) {
                j = (random( ) % i) + 1;
                printf("%s ", av[j]);
                av[j] = av[i];
        }
        printf("\n");
	exit(0);
}
