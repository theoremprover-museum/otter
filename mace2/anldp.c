/* ANLDP 2.0
 *
 * Copyright (C) 2001 by Argonne National Laboratory
 *
 * William McCune
 * Mathematics and Computer Science Division
 * Argonne National Laboratory
 * Argonne, IL  60439
 * U.S.A.
 *
 * E-mail: mccune@mcs.anl.gov
 * Web:    http://www.mcs.anl.gov/~mccune
 *         http://www.mcs.anl.gov/AR/mace
 */

#define VERSION "2.2"
#define VDATE "August 2003"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>  /* for getopt */

#ifdef TP_NAMES  /* for calls to getpwuid() and gethostname() */
#  include <sys/types.h>
#  include <pwd.h>
#  include <unistd.h>
#endif

/******** Types of exit ********/

#define MACE_ABEND_EXIT         11
#define UNSATISFIABLE_EXIT      12
#define MACE_MAX_SECONDS_EXIT   13
#define MACE_MAX_MEM_EXIT       14
#define MAX_MODELS_EXIT         15
#define ALL_MODELS_EXIT         16
#define MACE_SIGINT_EXIT        17
#define MACE_SEGV_EXIT          18
#define MACE_INPUT_ERROR_EXIT   19

/************* END OF ALL GLOBAL CONSTANT DEFINITIONS ****************/

#include "Opts.h"
#include "Stats.h"
#include "Avail.h"
#include "Clock.h"
#include "Miscellany.h"

#include "Dp.h"

int First_order;        /* used by dp.c */
int Domain_size = 0;    /* used by stats.c */
int Init_wall_seconds;  /* used by stats.c */
int Models = 0;         /* used with dp.c, miscellany.c */

/* I didn't want ANLDP to depend on any of the otter code in the
 * parent directory, so I copied the few routines that it needs
 * and put them here.
 */

/*************
 *
 *    void MACE_print_banner(argc, argv)
 *
 *************/

static void MACE_print_banner(int argc, char **argv)
{
    int i;

    printf("----- ANLDP %s, %s -----\n", VERSION, VDATE);
    printf("The process was started by %s on %s,\n%s",
	   username(), hostname(), get_time());

    printf("The command was \"");
    for(i = 0; i < argc; i++)
        printf("%s%s", argv[i], (i < argc-1 ? " " : ""));
    printf("\".\n");
    
}  /* MACE_print_banner */

/*************
 *
 *   usage_message()
 *
 *************/

static void usage_message(void)
{
    printf("\nANLDP %s -- Search for finite models.\n\n", VERSION);
    printf("\nInput clauses are taken from standard input.\n");
    printf("\ncommand-line options:\n");

    printf("   -p      | Print all models as they are found.\n");

    printf("   -m n    | Stop when the n-th model is found.\n");
    printf("   -t n    | Stop after n seconds.\n");
    printf("   -k n    | Tp_allocate at most n K bytes.\n");

    printf("   -s      | Perform unit subsumption. (Always done on input.)\n");
    printf("   -h      | Print this message.\n");
    printf("\n");
}  /* usage_message */

/*************
 *
 *   process_command_line_args()
 *
 *************/

static int process_command_line_args(int argc, char **argv)
{
    extern char *optarg;

    int c, n;
    int error = 0;

    while ((c = getopt(argc, argv, "spPIhcm:t:k:n:d:x:")) != EOF) {
	switch (c) {
	  case 's':
	    MACE_change_flag(stderr, "subsume", 1);
	    break;
	  case 'p':
	    MACE_change_flag(stderr, "print_models", 1);
	    break;
	  case 'm':
	    n = atoi(optarg);
	    if (MACE_change_parm(stderr, "max_models", n) == -1)
		error++;
	    break;
	  case 't':
	    n = atoi(optarg);
	    if (MACE_change_parm(stderr, "max_seconds", n) == -1)
		error++;
	    break;
	  case 'k':
	    n = atoi(optarg);
	    if (MACE_change_parm(stderr, "max_mem", n) == -1)
		error++;
	    break;
	  case 'h':
	  case '?':
	  default:
	    error = 1;
	    break;
	    }
	}

    return(error == 0);

}  /* process_command_line_args */

/*************
 *
 *   MACE_sig_handler()
 *
 *************/

#ifdef TP_SIGNAL
#include <signal.h>

static void MACE_sig_handler(int condition)
{
  if (condition == SIGSEGV) {

    char message[] =
      "\n"
      "+----------------------------------------------------------+\n"
      "| SEGMENTATION FAULT!!  This is probably caused by a bug   |\n"
      "| in MACE.  Please send copy of the input file to          |\n"
      "| otter@mcs.anl.gov, let us know what version of MACE you  |\n"
      "| are using, and send any other info that might be useful. |\n"
      "+----------------------------------------------------------+\n\n";

    fprintf(stderr, "%s\007", message);

    exit_with_message(MACE_SEGV_EXIT, 1);
  }
  else if (condition == SIGINT) {
    exit_with_message(MACE_SIGINT_EXIT, 1);
  }
  else {
    char s[100];
    sprintf(s, "MACE_sig_handler, cannot handle signal %d.\n", condition);
    MACE_abend(s);
  }
}  /* MACE_sig_handler */

#endif  /* TP_SIGNAL */

/*************
 *
 *    main
 *
 *************/

int main(int argc, char **argv)
{
  int rc;

#ifdef TP_SIGNAL
  signal(SIGINT, MACE_sig_handler);
  signal(SIGSEGV, MACE_sig_handler);
#endif

  MACE_print_banner(argc, argv);
  init_dp();  /* This is MACE's initialization. */
  Init_wall_seconds = wall_seconds();
  rc = process_command_line_args(argc, argv);
  if (!rc) {
    usage_message();
    exit(0);
  }

  init();  /* This is Otter's initialization. */

  /*************************************************************************/

  rc = read_all_clauses(stdin);
  rc = more_setup();

  MACE_CLOCK_START(DECIDE_TIME);
  rc = dp_prover();
  MACE_CLOCK_STOP(DECIDE_TIME);

  exit_with_message(rc, 1);

  exit(99);  /* This is to shut up the compiler. */

}  /* main */
