#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "tpcc.h"
#include "vista.h"
#include <signal.h>

/* Global Variable */
int	sc;	/* TPC-C scaling factor */

/* use macros for random ==> 2x speed */

#define myRand(min, max) ( ( (min) + (rand()%( 1 + (max) - (min) ) ) ) )

/* function prototypes */

void initDatabase(wareType [], int []);
void newOrder(vista_segment *, wareType [], int []);
void payment(vista_segment *, wareType []);
void delivery(vista_segment *, wareType []);

main(int argc, char *argv[])
{
    /* database */
    wareType *ware;	/* array of warehouse records */
    int itemPrice[ITEMS]; /* this is never modified */
    vista_segment *vistaSegPtr;

    int trans;
    int newOrderTrans=0;
    struct timeval tStart, tEnd;
    int type;
    double elapsed, mb;
    int numTrans, nullTrans;
    int	tps;

    if (argc != 2) {
	printf("error: usage: %s <scaling factor>\n", argv[0]);
	exit(1);
    }

    /* get scaling factor from command line */
    sc = atoi(argv[1]);

    /* set up persistent heap */
    unlink("data");
    unlink("data.meta");

    vista_init();
    vistaSegPtr = vista_map("data");
    if (vistaSegPtr == NULL) {
	vista_perror("vista_map failed");
	exit(1);
    }
    ware = (wareType *) vista_malloc(vistaSegPtr, sc*sizeof(wareType),
					VISTA_NO_TRANS);
    
    if (ware == NULL) {
	vista_perror("vista_malloc failed");
	exit(1);
    }

    info(sc);
    initDatabase(ware, itemPrice);

#ifdef REMOVE
    fprintf(stderr, "Doing warmup... ");
    for (trans=0; trans<WARMUP; trans++) {
	type = myRand(0,99);
	if (type < 45) {
	    newOrder(vistaSegPtr, ware, itemPrice);
	} else if (type < 88) {
	    payment(vistaSegPtr, ware);
	} else if (type < 92) {
	    /* orderStatus not implemented */
	} else if (type < 96) {
	    delivery(vistaSegPtr, ware);
	} else {
	    /* stockLevel not implemented */
	}
    }
    fprintf(stderr, "done.\n");
#endif

    fprintf(stderr, "Beginning run... ");

    nullTrans = 0;
    numTrans = LOOPS;
    /* timed test begins here */
    gettimeofday(&tStart, NULL);
    for (trans=0; trans<numTrans; trans++) {
	type = myRand(0,99);
	if (type < 45) {
	    newOrder(vistaSegPtr, ware, itemPrice);
	    newOrderTrans++;
	} else if (type < 88) {
	    payment(vistaSegPtr, ware);
	} else if (type < 92) {
	    /* orderStatus not implemented */
	    nullTrans++;
	} else if (type < 96) {
	    delivery(vistaSegPtr, ware);
	} else {
	    /* stockLevel not implemented */
	    nullTrans++;
	}
    }
    gettimeofday(&tEnd, NULL);
    /* timed test ends here */

    fprintf(stderr, "done.\n");

    elapsed = (tEnd.tv_sec - tStart.tv_sec) +
	(tEnd.tv_usec - tStart.tv_usec) / 1000000.0;
    printf("elapsed time: %.1lf seconds\n", elapsed);
    printf("throughput: %.1lf transactions/second\n", trans/elapsed);
    trans -= nullTrans;
    tps = trans / elapsed;
    mb = (sc * sizeof(wareType)) / 1048576.0;
    printf("tpmC: %.1lf new order transactions/minute\n",
	   newOrderTrans/elapsed*60.0);
    printf("%d: %d tps; %.2f secs; %.2f mb\n", sc, tps, elapsed, mb);
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

#ifdef UNDEF
/* generate a random integer between min and max (inclusive) */
myRand(int min, int max)
{
    if (max==min) {
	/* this prevents a divide-by-zero error */
	return(max);
    } else {
	return(min + 1 + rand()%(max-min)); /* use rand because it's fast */
	/* return(min + 1 + random()%(max-min)); */
    }
}
#endif

nonUnifRand(int min, int max, int a)
{
    return( ( ((myRand(0,a) | myRand(min, max)) + myRand(0,a)) % (max-min+1))
	    + min);
}

info(int scaling_factor)
{
    printf("total memory used=\t%d\n",
		    scaling_factor*sizeof(wareType) + ITEMS*sizeof(int));
    printf("sizeof(wareType)=\t%d\n", sizeof(wareType));
    printf("sizeof(distType)=\t%d\n", sizeof(distType));
    printf("sizeof(stockType)=\t%d\n", sizeof(stockType));
    printf("sizeof(customerType)=\t%d\n", sizeof(customerType));
    printf("sizeof(historyType)=\t%d\n", sizeof(historyType));
    printf("sizeof(orderType)=\t%d\n", sizeof(orderType));
    printf("sizeof(orderLineType)=\t%d\n", sizeof(orderLineType));
}

void newOrder(vista_segment *vistaSegPtr, wareType ware[], int itemPrice[])
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
    int tid;

    tid = vista_begin_transaction(vistaSegPtr, VISTA_EXPLICIT);
    if (tid < 0) {
	vista_perror("vista_begin_transaction");
	exit(1);
    }

    orderId = ware[w].dist[d].nextOrderId % ORDERSPERDIST;

    /* insert new row in order table.  spec says to add row (not replace row) */

    if (!vista_set_range(vistaSegPtr, &(ware[w].dist[d].order[orderId]),
			    sizeof(ware[w].dist[d].order[orderId]), tid ) ) {
	vista_perror("vista_set_range in newOrder");
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

    if (!vista_set_range(vistaSegPtr, &(ware[w].dist[d].nextOrderId),
			    sizeof(ware[w].dist[d].nextOrderId), tid ) ) {
	vista_perror("vista_set_range in newOrder");
	exit(1);
    }
    ware[w].dist[d].nextOrderId++;

    /* spec says to compute total amount, using tax */

    if (myRand(0,99) == 99) {
	/* abort 1% of newOrder transactions */
	/* printf("aborting transaction\n");*/
	vista_abort_transaction(vistaSegPtr, tid);
    }
    else {
        vista_end_transaction(vistaSegPtr, tid);
    }

}

void payment(vista_segment *vistaSegPtr, wareType ware[])
{
    int w = myRand(0, sc-1);			/* warehouse number */
    int d = myRand(0, DISTSPERWARE-1);		/* district number */
    int c = nonUnifRand(0, CUSTOMERSPERDIST-1, 1023);	/* customer number */
	/* spec says to select by last name 60% of the time),
	    but this can be done easily with a hash table) */
    int custDist, custWare;
    int amount = myRand(1,5000);		/* payment amount */
    int historyId;
    int tid;

    tid = vista_begin_transaction(vistaSegPtr, VISTA_EXPLICIT);
    if (tid < 0) {
	vista_perror("vista_begin_transaction");
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
    if (!vista_set_range(vistaSegPtr, &(ware[w].ytd),
			    sizeof(ware[w].ytd), tid ) ) {
	vista_perror("vista_set_range in payment");
	exit(1);
    }

    ware[w].ytd += amount;

    /* update district */
    if (!vista_set_range(vistaSegPtr, &(ware[w].dist[d].ytd),
			    sizeof(ware[w].dist[d].ytd), tid ) ) {
	vista_perror("vista_set_range in payment");
	exit(1);
    }

    ware[w].dist[d].ytd += amount;

    /* update customer */
    if (!vista_set_range(vistaSegPtr,
			    &(ware[custWare].dist[custDist].customer[c]),
			    sizeof(ware[custWare].dist[custDist].customer[c]),
			    tid ) ) {
	vista_perror("vista_set_range in payment");
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
    if (!vista_set_range(vistaSegPtr,
		    &(ware[custWare].dist[custDist].history[historyId]),
		    sizeof(ware[custWare].dist[custDist].history[historyId]),
		    tid ) ) {
	vista_perror("vista_set_range in payment");
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

    vista_end_transaction(vistaSegPtr, tid);
}

void delivery(vista_segment *vistaSegPtr, wareType ware[])
{
    int w = myRand(0, sc-1);			/* warehouse number */
    int d;					/* district number */
    int c;					/* customer number */
    int carrier = myRand(1, 10);
    int orderId;
    int ol;
    int sum;
    int tid;

    tid = vista_begin_transaction(vistaSegPtr, VISTA_EXPLICIT);
    if (tid < 0) {
	vista_perror("vista_begin_transaction");
	exit(1);
    }

    for (d=0; d<DISTSPERWARE; d++) {
	sum = 0;
	orderId = myRand(0, ORDERSPERDIST-1); /* different from spec (should
				    be chosen by searching newOrder table */
	c = ware[w].dist[d].order[orderId].customerId;

	/* note: this logs more data than necessary (only really need
	    carrierId and the delivery dates of each order line) */
	if (!vista_set_range(vistaSegPtr, &(ware[w].dist[d].order[orderId]),
			    sizeof(ware[w].dist[d].order[orderId]), tid ) ) {
	    vista_perror("vista_set_range in delivery");
	    exit(1);
	}

	/* note: this logs more data than necessary (only really need
	    balance and deliveryCount) */
	if (!vista_set_range(vistaSegPtr, &(ware[w].dist[d].customer[c]),
			    sizeof(ware[w].dist[d].customer[c]), tid ) ) {
	    vista_perror("vista_set_range in delivery");
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
    vista_end_transaction(vistaSegPtr, tid);
}
