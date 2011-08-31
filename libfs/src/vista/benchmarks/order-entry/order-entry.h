/* definitions for database size */

#define ITEMS 100000
#define WARES 1 /* this determines overall database size */
#define STOCKSPERWARE 100000
#define DISTSPERWARE 10 /* spec says 10 */
#define CUSTOMERSPERDIST 3000
#define HISTORYSPERDIST 3000
#define ORDERSPERDIST 3000
#define NEWORDERSPERDIST 900
#define ORDERLINESPERORDER 15

#define WARMUP 5000
#define LOOPS 50000

/* definitions for default data values */
#define DATA "default"
#define NAME "default"

/* type definitions */

typedef struct {
    int quantity;
    char dist[DISTSPERWARE][10]; /* spec says 25 */
} stockType;

typedef struct {
    int itemId;
    int supplyWareId;
    long deliveryDate;
    int quantity;
    int amount;
    char distInfo[10]; /* spec says 25 */
} orderLineType;

typedef struct {
    int customerId;
    long entryDate;
    int carrierId;
    int orderLineCount;
    int allLocal;
    orderLineType orderLine[ORDERLINESPERORDER];
} orderType;

typedef struct {
    int customerId;
    int distId;
    int wareId;
    long date;
    int amount;
    char data[25];
} historyType;

typedef struct {
    long balance;
    int deliveryCount;
    char credit[3];
    long ytdPayment;
    int paymentCount;
    char data[100]; /* different from spec */
} customerType;

typedef struct {
    char name[11];
    int ytd;
    int nextOrderId;

    /* my data structure */
    int nextHistoryId;

    customerType customer[CUSTOMERSPERDIST];
    orderType order[ORDERSPERDIST];
    /* newOrderType newOrder[NEWORDERSPERDIST]; */
	/* different from spec.  We don't need a newOrder table because all
	the info is in how we organize the data structures */
    historyType history[HISTORYSPERDIST];
} distType;

typedef struct {
    /* int id; */
    char name[11];
    int ytd;

    stockType stock[STOCKSPERWARE];
    distType dist[DISTSPERWARE];
} wareType;
