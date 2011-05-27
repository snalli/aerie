/*
 * bench.h
 *
 *   Header information for debit-credit benchmark exercising vista library.
 */

/**
 ** Include Files
 **/

#include <vista.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>


/**
 ** Defines
 **/

#define LOOPS 500000
/* #define LOOPS 50000 */
#define WARMUP 150000
/* #define WARMUP 5000 */

#define TPS 5
#define NACCOUNTS 100000
#define NTELLERS 10
#define NBRANCHES 1
#define NHISTRECS 40000
/*
#define NHISTRECS (LOOPS + WARMUP)
#define NHISTRECS LOOPS
*/
#define HISTFLUSH 283569


/**
 ** Types
 **/

typedef struct account_s {
	int	account_id;
	int	branch_id;
	int	balance;
	char	pad[100 - (3 * sizeof(int))];
} account;

typedef struct branch_s {
	int	branch_id;
	int	branch_balance;
	char	pad[100 - (2 * sizeof(int))];
} branch;

typedef struct teller_s {
	int	teller_id;
	int	branch_id;
	int	teller_balance;
	char	pad[100 - (3 * sizeof(int))];
} teller;

typedef struct history_s {
	int	account_id;
	int	teller_id;
	int	branch_id;
	int	amount;
	time_t	timestamp;
	char	pad[50 - ((4 * sizeof(int)) + sizeof(time_t))];
} history;


