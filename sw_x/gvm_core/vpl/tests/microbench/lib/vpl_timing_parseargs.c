
// Include declarations of our public API
#include "vpl_microbenchmark.h"

#include "gvmtest_load.h"  // GVM CPU-scheduling hooks; priority hooks; BSD _progname emulation

#include <getopt.h>
#include <unistd.h>
#include <sysexits.h>  // EX_*
#include <stdlib.h>    // strtoul()

#include <stdio.h>     // fprintf()

#include "microbench_profil.h"  // VPL broadon-internal profiling API (sic)


int
parse_args(int argc, char *argv[], int *nSamples, int *samples_flags)
{
    int optc;

#ifdef notyet
    FILE *f = stdout;
#endif

    while ((optc = getopt(argc, argv, "n:o:s")) != -1) {
        char *nextchar = optarg;
        int tempval;
        switch (optc) {
        case 'n':
            tempval = strtoul(optarg, &nextchar, 0);
            if (nextchar == optarg || nextchar == 0 || *nextchar != 0) {
                fprintf(stderr, " %s: parameter not an integer\n",
                        __progname);
                exit(EX_USAGE);
            }
            *nSamples = tempval;
            break;

        case 'o':
#ifdef notyet
            f = fopen(optarg, "w");
#endif // notyet
            break;


        case 's':
	  (*samples_flags) |= VPLPROFIL_SAVEDSAMPLES_SUMMARY_ONLY;
            break;

        default:
            fprintf (stderr, "usage: bad opt %c\n", optc);
            exit(EX_USAGE);
        }
    } 

#ifdef notyet
    if (outfilep != 0) {
        *outfilep = f;
    }
#endif

    return 0;
}
