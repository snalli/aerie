#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "tpcc.h"
#include "rvm.h"

/* global variable */
int	sc;   /* TPC-C scaling factor */

/* use macros for random ==> 2x speed */

#ifndef MORE_RAND
/* #define myRand(min, max) ((min==max)?(max): (min) + (rand()%(  (max) - (min) ) ) )  */
#define myRand(min, max) ( (min) + (rand()%( 1 + (max) - (min) ) ) )
#else
int myRand(int, int);
#endif

/* function prototypes */

void initDatabase(wareType [], int []);
void newOrder(wareType [], int []);
void payment(wareType []);
void delivery(wareType []);

main(int argc, char *argv[])
{
    /* database */
    wareType *ware;	/* array of warehouse records */
    int itemPrice[ITEMS]; /* this is never modified */

    int trans;
    int newOrderTrans=0;
    struct timeval tStart, tEnd;
    int type;
    double elapsed, mb;
    int numTrans;
    rvm_options_t *optPtr;
    rvm_region_t *regionPtr;
    rvm_return_t code;
    char command[256];
    int	nTruncates, tps, groupsize, num_buffered, nullTrans;

#ifndef GROUP_COMMIT
    if (argc != 2) {
	printf("error: usage: %s <scaling factor>\n", argv[0]);
	exit(1);
    }
#else
    if (argc != 3) {
	printf("error: usage: %s <scaling factor> <group size>\n", argv[0]);
	exit(1);
    }
    groupsize = atoi(argv[2]);
#endif GROUP_COMMIT

    sc = atoi(argv[1]);  /* get tpc scaling factor */

#ifndef RAW
    printf("initializing data file\n");
    sprintf(command, "dd if=/dev/zero of=%s bs=1024k count=%d>/dev/null 2>&1",
	DATAFILE, 1 + (sc*sizeof(wareType))/1048576);
    system(command);
#endif

    /* set up persistent heap */
    optPtr = rvm_malloc_options();
    optPtr->flags = RVM_ALL_OPTIMIZATIONS;
    optPtr->truncate = 50;
    optPtr->log_dev = LOGFILE;

    if ( (code = RVM_INIT(optPtr)) != RVM_SUCCESS) {
	fprintf(stderr, "rvm initialization failed, code=%s\n",
	    rvm_return(code));
	exit(1);
    }

    regionPtr = rvm_malloc_region();
    regionPtr->data_dev = DATAFILE;
#ifdef RAW
    regionPtr->dev_length = RVM_MK_OFFSET(0, 1000000000);
    regionPtr->length = (rvm_length_t) (sc * sizeof(wareType));
#endif
    regionPtr->no_copy = TRUE;
    if (rvm_map(regionPtr, NULL) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_map failed\n");
	exit(1);
    }

    ware = (wareType *) regionPtr->vmaddr;
    
    info();
    initDatabase(ware, itemPrice);

#ifdef REMOVE
    fprintf(stderr, "Doing warmup... ");
    num_buffered = 0;
    for (trans=0; trans<WARMUP; trans++) {
	type = myRand(0,99);
	if (type < 45) {
	    newOrder(ware, itemPrice);
	} else if (type < 88) {
	    payment(ware);
	} else if (type < 92) {
	    /* orderStatus not implemented */
	} else if (type < 96) {
	    delivery(ware);
	} else {
	    /* stockLevel not implemented */
	}
	num_buffered++;
	GROUP_FLUSH();
    }
    fprintf(stderr, "done.\n");

    fprintf(stderr, "Truncating... ");
    rvm_truncate();
    fprintf(stderr, "done.\n");
#endif

    fprintf(stderr, "Beginning run... ");

    nTruncates = rvm_num_truncations;
    nullTrans = 0;
    num_buffered = 0;
    /* timed test begins here */
    gettimeofday(&tStart, NULL);
    for (trans=0; nTruncates == rvm_num_truncations; trans++) {
	type = myRand(0,99);
	if (type < 45) {
	    newOrder(ware, itemPrice);
	    newOrderTrans++;
	} else if (type < 88) {
	    payment(ware);
	} else if (type < 92) {
	    /* orderStatus not implemented */
	    nullTrans++;
	    num_buffered--;
	} else if (type < 96) {
	    delivery(ware);
	} else {
	    /* stockLevel not implemented */
	    nullTrans++;
	    num_buffered--;
	}
	num_buffered++;
	GROUP_FLUSH();
    }
    gettimeofday(&tEnd, NULL);
    /* timed test ends here */

    fprintf(stderr, "done.\n");

    elapsed = (tEnd.tv_sec - tStart.tv_sec) +
	(tEnd.tv_usec - tStart.tv_usec) / 1000000.0;
    printf("elapsed time: %.1lf seconds\n", elapsed);
    printf("throughput: %.1lf transactions/second\n", trans/elapsed);
    printf("tpmC: %.1lf new order transactions/minute\n",
	   newOrderTrans/elapsed*60.0);
    trans -= nullTrans;
    tps = trans / elapsed;
    mb =  (sc * sizeof(wareType)) / 1048576.0;
#ifndef GROUP_COMMIT
    printf("%d: %d tps; %.2f secs; %.2f mb; %d trans\n", sc, tps, elapsed, mb, trans);
#else
    printf("%d: %d tps; %.2f secs; %.2f mb; %d trans; group-commit %d\n", sc, tps, elapsed, mb, trans, groupsize);
#endif
}

void initDatabase(wareType ware[], int itemPrice[])
{
    int i, w, s, d, c, h, o, ol, n; /* indexes into arrays */

    printf("initializing database\n");

    srandom(13);

    /* initialize items */
    for (i=0; i<ITEMS; i++) {
	itemPrice[i] = myRand(1,100);
    }

    /* initialize wares */
    for (w=0; w<sc; w++) {
	printf("\twarehouse %d\n", w);
	strcpy(ware[w].name, NAME);
	ware[w].ytd = 300000;

	/* initialize stocks */
	for (s=0; s<STOCKSPERWARE; s++) {
	    ware[w].stock[s].quantity = myRand(10,100);
	    for (d=0; d<DISTSPERWARE; d++) {
		strcpy(ware[w].stock[s].dist[d], NAME);
	    }
	}

	/* initialize dists */
	for (d=0; d<DISTSPERWARE; d++) {
	    printf("\t\tdistrict %d\n", d);
	    strcpy(ware[w].dist[d].name, NAME);
	    ware[w].dist[d].ytd = 30000;
	    ware[w].dist[d].nextOrderId = ORDERSPERDIST;

	    /* initialize customers */
	    for (c=0; c<CUSTOMERSPERDIST; c++) {
		if (myRand(0,9)==9) {
		    strcpy(ware[w].dist[d].customer[c].credit, "BC");
		} else {
		    strcpy(ware[w].dist[d].customer[c].credit, "GC");
		}
		ware[w].dist[d].customer[c].balance = -10;
		ware[w].dist[d].customer[c].ytdPayment = 10;
		ware[w].dist[d].customer[c].paymentCount = 1;
		ware[w].dist[d].customer[c].deliveryCount = 0;
		strcpy(ware[w].dist[d].customer[c].data, DATA);

	    }

	    /* initialize history record */
	    for (h=0; h<HISTORYSPERDIST; h++) {
		ware[w].dist[d].history[h].customerId = h%CUSTOMERSPERDIST;

		ware[w].dist[d].history[h].date =
		    myRand(0,100000);
		ware[w].dist[d].history[h].amount = 10;
		strcpy(ware[w].dist[d].history[h].data, DATA);
	    }
	    ware[w].dist[d].nextHistoryId = HISTORYSPERDIST;

	    /* initialize orders */
	    for (o=0; o<ORDERSPERDIST; o++) {
		ware[w].dist[d].order[o].customerId=o; /* spec says this should
			be random and unique (see top of p. 63) */
		ware[w].dist[d].order[o].entryDate=myRand(0,100000);
		ware[w].dist[d].order[o].carrierId=myRand(0,9);
		ware[w].dist[d].order[o].orderLineCount=myRand(5,15);
		ware[w].dist[d].order[o].allLocal=1;

		for (ol=0;
		    ol<ware[w].dist[d].order[o].orderLineCount;
		    ol++) {

		    ware[w].dist[d].order[o].orderLine[ol].itemId =
			myRand(0,ITEMS-1);
		    ware[w].dist[d].order[o].orderLine[ol].supplyWareId = w;
		    if (ol <= 2000) {
			ware[w].dist[d].order[o].orderLine[ol].deliveryDate =
			ware[w].dist[d].order[o].entryDate;
		    } else {
			ware[w].dist[d].order[o].orderLine[ol].deliveryDate = 0;
		    }
		    ware[w].dist[d].order[o].orderLine[ol].quantity = 5;
		    ware[w].dist[d].order[o].orderLine[ol].amount = 0;
		    strcpy(
			ware[w].dist[d].order[o].orderLine[ol].distInfo,
			DATA);
		}
	    }

	}
    }
}

#ifdef MORE_RAND
/* generate a random integer between min and max (inclusive) */
myRand(int min, int max)
{
    return(min + rand()%(1 + max-min)); /* rand is faster */
    /* return(min + random()%(1 + max-min));  */
}
#endif

nonUnifRand(int min, int max, int a)
{
#ifndef UNIFORM_RAND
    return( ( ((myRand(0,a) | myRand(min, max)) + myRand(0,a)) % (max-min+1))
	    + min);
#else
    return myRand(min, max);
#endif
}

info()
{
#ifdef RAW
    int	raw = TRUE;
#else
    int	raw = FALSE;
#endif
#ifdef GROUP_COMMIT
    int	gc = TRUE;
#else
    int	gc = FALSE;
#endif
#ifdef UNIFORM_RAND
    int	uni = TRUE;
#else
    int	uni = FALSE;
#endif
    printf("Version: TPC-C, RVM, %s%s%s\n", (raw?"raw":"cooked"), (gc?", group-commit":""), (uni?", uniform random":""));
    printf("total memory used=\t%d\n",
		    sc*sizeof(wareType) + ITEMS*sizeof(int));
    printf("sizeof(wareType)=\t%d\n", sizeof(wareType));
    printf("sizeof(distType)=\t%d\n", sizeof(distType));
    printf("sizeof(stockType)=\t%d\n", sizeof(stockType));
    printf("sizeof(customerType)=\t%d\n", sizeof(customerType));
    printf("sizeof(historyType)=\t%d\n", sizeof(historyType));
    printf("sizeof(orderType)=\t%d\n", sizeof(orderType));
    printf("sizeof(orderLineType)=\t%d\n", sizeof(orderLineType));
}

void newOrder(wareType ware[], int itemPrice[])
{
    int w = myRand(0, sc-1);			/* warehouse number */
    int d = myRand(0, DISTSPERWARE-1);		/* district number */
    int c = nonUnifRand(0, CUSTOMERSPERDIST-1, 1023);	/* customer number */
    int itemId;
    int olCount = myRand(5, 15);		/* ol_count */
    int ol;
    int orderId;
    int supplyWare;
    int quantity; /* quantity of an item in the order */
    rvm_tid_t *tidPtr;

    tidPtr = rvm_malloc_tid();
    if (tidPtr == NULL) {
	fprintf(stderr, "rvm_malloc_tid failed\n");
	exit(1);
    }
    if (rvm_begin_transaction(tidPtr, restore) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_begin_transaction\n");
	exit(1);
    }

    orderId = ware[w].dist[d].nextOrderId % ORDERSPERDIST;

    /* insert new row in order table.  spec says to add row (not replace row) */

    if (rvm_set_range(tidPtr, &(ware[w].dist[d].order[orderId]),
			    sizeof(ware[w].dist[d].order[orderId]))
	    != RVM_SUCCESS) {

	fprintf(stderr, "rvm_set_range in newOrder\n");
	exit(1);
    }

    ware[w].dist[d].order[orderId].customerId = c;
    ware[w].dist[d].order[orderId].entryDate = myRand(0,100000);
					/* spec says use current date */
    ware[w].dist[d].order[orderId].carrierId = 0;
    ware[w].dist[d].order[orderId].orderLineCount = olCount;
    ware[w].dist[d].order[orderId].allLocal = 1;

    /* spec says to add row in newOrder table */

    for (ol=0; ol<olCount; ol++) {
	/* choose supply warehouse */
	if (myRand(0,99) == 99) {
	    /* remote supply warehouse */
	    supplyWare =  myRand(0, sc-1);
	    ware[w].dist[d].order[orderId].allLocal = 0;
	} else {
	    supplyWare =  w;
	}

        itemId = nonUnifRand(0, ITEMS-1, 8191);
	quantity = myRand(1,10);
	ware[w].dist[d].order[orderId].orderLine[ol].itemId = itemId;
	ware[w].dist[d].order[orderId].orderLine[ol].supplyWareId = supplyWare;
	ware[w].dist[d].order[orderId].orderLine[ol].deliveryDate = 0;
	ware[w].dist[d].order[orderId].orderLine[ol].quantity = quantity;
	ware[w].dist[d].order[orderId].orderLine[ol].amount =
	    quantity * itemPrice[itemId];
	strcpy(ware[w].dist[d].order[orderId].orderLine[ol].distInfo,
	    ware[w].stock[itemId].dist[d]);
    }

    if (rvm_set_range(tidPtr, &(ware[w].dist[d].nextOrderId),
			    sizeof(ware[w].dist[d].nextOrderId))
	    != RVM_SUCCESS) {
	fprintf(stderr, "rvm_set_range in newOrder\n");
	exit(1);
    }
    ware[w].dist[d].nextOrderId++;

    /* spec says to compute total amount, using tax */

    if (myRand(0,99) == 99) {
	/* abort 1% of newOrder transactions */
	/* printf("aborting transaction\n"); */
	rvm_abort_transaction(tidPtr);
    } else {
	if (rvm_end_transaction(tidPtr, FLUSH) != RVM_SUCCESS) {
	    fprintf(stderr, "rvm_end_transaction failed\n");
	    exit(1);
	}
    }

}

void payment(wareType ware[])
{
    int w = myRand(0, sc-1);			/* warehouse number */
    int d = myRand(0, DISTSPERWARE-1);		/* district number */
    int c = nonUnifRand(0, CUSTOMERSPERDIST-1, 1023);	/* customer number */
	/* spec says to select by last name 60% of the time),
	    but this can be done easily with a hash table) */
    int custDist, custWare;
    int amount = myRand(1,5000);		/* payment amount */
    int historyId;
    rvm_tid_t *tidPtr;

    tidPtr = rvm_malloc_tid();
    if (tidPtr == NULL) {
	fprintf(stderr, "rvm_malloc_tid failed\n");
	exit(1);
    }
    if (rvm_begin_transaction(tidPtr, restore) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_begin_transaction\n");
	exit(1);
    }

    if (myRand(0,99) < 85) {
	/* remote customer */
	custWare = myRand(0, sc-1);
	custDist = myRand(0,DISTSPERWARE-1);
    } else {
	custWare = w;
	custDist = d;
    }
    historyId = ware[custWare].dist[custDist].nextHistoryId % HISTORYSPERDIST;

    /* update warehouse */
    if (rvm_set_range(tidPtr, &(ware[w].ytd),
			    sizeof(ware[w].ytd)) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_set_range1 in payment\n");
	exit(1);
    }

    ware[w].ytd += amount;

    /* update district */
    if (rvm_set_range(tidPtr, &(ware[w].dist[d].ytd),
			    sizeof(ware[w].dist[d].ytd)) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_set_range2 in payment\n");
	exit(1);
    }

    ware[w].dist[d].ytd += amount;

    /* update customer */
    if (rvm_set_range(tidPtr,
		&(ware[custWare].dist[custDist].customer[c]),
		sizeof(ware[custWare].dist[custDist].customer[c]) ) 
	!= RVM_SUCCESS) {
	fprintf(stderr, "rvm_set_range3 in payment\n");
	exit(1);
    }
    ware[custWare].dist[custDist].customer[c].balance -= amount;
    ware[custWare].dist[custDist].customer[c].ytdPayment += amount;
    ware[custWare].dist[custDist].customer[c].paymentCount++;

    if (!strcmp(ware[custWare].dist[custDist].customer[c].credit, "BC")) {
	/* bad credit */
	sprintf(ware[custWare].dist[custDist].customer[c].data,
	    "%d %d %d %d %d %d", c, custDist, custWare, d, w, amount);
    }

    /* update history */
    if (rvm_set_range(tidPtr,
		&(ware[custWare].dist[custDist].history[historyId]),
		sizeof(ware[custWare].dist[custDist].history[historyId]))
	!= RVM_SUCCESS) {
	fprintf(stderr, "rvm_set_range4 in payment\n");
	exit(1);
    }
    ware[custWare].dist[custDist].history[historyId].customerId = c;
    ware[custWare].dist[custDist].history[historyId].distId = d;
    ware[custWare].dist[custDist].history[historyId].wareId = w;
    ware[custWare].dist[custDist].history[historyId].date = myRand(0,100000);
					/* spec says use current date */
    ware[custWare].dist[custDist].history[historyId].amount = amount;
    sprintf(ware[custWare].dist[custDist].history[historyId].data,
	"%s    %s", ware[w].name, ware[w].dist[d].name);

    if (rvm_end_transaction(tidPtr, FLUSH) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_end_transaction failed\n");
	exit(1);
    }
}

void delivery(wareType ware[])
{
    int w = myRand(0, sc-1);			/* warehouse number */
    int d;					/* district number */
    int c;					/* customer number */
    int carrier = myRand(1, 10);
    int orderId;
    int ol;
    int sum;
    rvm_tid_t *tidPtr;

    tidPtr = rvm_malloc_tid();
    if (tidPtr == NULL) {
	fprintf(stderr, "rvm_malloc_tid failed\n");
	exit(1);
    }
    if (rvm_begin_transaction(tidPtr, restore) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_begin_transaction\n");
	exit(1);
    }

    for (d=0; d<DISTSPERWARE; d++) {
	sum = 0;
	orderId = myRand(0, ORDERSPERDIST-1); /* different from spec (should
				    be chosen by searching newOrder table */
	c = ware[w].dist[d].order[orderId].customerId;

	/* note: this logs more data than necessary (only really need
	    carrierId and the delivery dates of each order line) */
	if (rvm_set_range(tidPtr, &(ware[w].dist[d].order[orderId]),
			    sizeof(ware[w].dist[d].order[orderId]))
	    != RVM_SUCCESS) {
	    fprintf(stderr, "rvm_set_range in delivery\n");
	    exit(1);
	}

	/* note: this logs more data than necessary (only really need
	    balance and deliveryCount) */
	if (rvm_set_range(tidPtr, &(ware[w].dist[d].customer[c]),
		sizeof(ware[w].dist[d].customer[c])) != RVM_SUCCESS) {
	    fprintf(stderr, "rvm_set_range in delivery\n");
	    exit(1);
	}

	ware[w].dist[d].order[orderId].carrierId = carrier;
	for (ol=0; ol<ware[w].dist[d].order[orderId].orderLineCount; ol++) {
	    ware[w].dist[d].order[orderId].orderLine[ol].deliveryDate =
		    myRand(0,100000);
		    /* spec says use current date */
	    ware[w].dist[d].customer[c].balance +=
		    ware[w].dist[d].order[orderId].orderLine[ol].amount;
	}
	ware[w].dist[d].customer[c].deliveryCount++;
    }
    if (rvm_end_transaction(tidPtr, FLUSH) != RVM_SUCCESS) {
	fprintf(stderr, "rvm_end_transaction failed\n");
	exit(1);
    }
}
