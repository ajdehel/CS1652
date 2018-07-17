#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for Project 2, unidirectional and bidirectional
   data transfer protocols.  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0

/**************** MACROS ****************/
/******** possible events: ********/
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2
#define  OFF    0
#define  ON     1
#define  A      0
#define  B      1

/**************** STRUCT DEFINITIONS ****************/
/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
};

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
};

/**************** GLOBALS ****************/
int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int   ncorrupt;            /* number corrupted by media*/

/**************** FUNCTION DECLARATIONS ****************/
void init();
void generate_next_arrival();
void tolayer5(int AorB, char datasent[20]);
void tolayer3(int AorB, struct pkt packet);
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void insertevent(struct event *p);
void init();
float jimsrand();
void printevlist();

/* ******************************************************************
 * STUDENT DECLARATIONS
 *
 *********************************************************************/

/**************** MACROS ****************/

/******** FSM ********/
/**** TX ****/
#define WAIT_FOR_0_FROM_ABOVE   &TX_STATE_MACHINE[0]
#define WAIT_FOR_ACK0           &TX_STATE_MACHINE[1]
#define WAIT_FOR_1_FROM_ABOVE   &TX_STATE_MACHINE[2]
#define WAIT_FOR_ACK1           &TX_STATE_MACHINE[3]
/**** RX ****/
#define WAIT_FOR_SEQ0 &RX_STATE_MACHINE[0]
#define WAIT_FOR_SEQ1 &RX_STATE_MACHINE[1]

/******** DEBUG ********/
/**** SWITCHES ****/
#define DEBUG_MESSAGES
#define DEBUG_PACKETS
#define DEBUG_STATES
/**** PRINTS ****/
/** PACKET **/
#ifdef DEBUG_MESSAGES
#  define DEBUG(x) printf("\nDEBUG***"); printf x
#else
#  define DEBUG(x) do {} while (0)
#endif
/** STATE **/
#ifdef DEBUG_STATES
#  define DEBUG_TX_STATE(x, y) printf("\nDEBUG***"); if (x == A) printf("ATX"); else printf("BTX"); printf(" -> STATE_ID:%d",y->state_num)
#  define DEBUG_RX_STATE(x, y) printf("\nDEBUG***"); if (x == A) printf("ARX"); else printf("BRX"); printf(" -> STATE_ID:%d",y->state_num)
#else
#  define DEBUG_TX_STATE(x, y) do {} while (0);
#  define DEBUG_RX_STATE(x, y) do {} while (0);
#endif
/** MISC **/
#ifdef DEBUG_PACKETS
#  define DEBUG_PKT(x) printf("\nDEBUG***PACKET -> SEQNUM:%d | ACKNUM:%d | CHECKSUM:%u | PAYLOAD:%s", x->seqnum, x->acknum, x->checksum, x->payload);
#else
#  define DEBUG_PKT(x) do {} while (0)
#endif

/******** OTHER ********/
/**** TIMER INCREMENT ****/
#define TIMER_INCREMENT 20.0f
/**** PACKET STATUSES ****/
#define PKT_OK      0
#define PKT_CORRUPT 1
#define PKT_TIMEOUT 2
#define PKT_NAK     3
#define PKT_DUP     4

/**************** STRUCT DEFINITIONS ****************/
struct TX_State
{
    int state_num;
    int seqnum;
    int acknum;
    struct TX_State *next_state;
};

struct RX_State
{
    int state_num;
    int seqnum;
    int acknum;
    struct RX_State *next_state;
};

/**************** GLOBALS ****************/

struct TX_State TX_STATE_MACHINE[] = {
/*WAIT_FOR_0_FROM_ABOVE*/   {0, 0, -1, WAIT_FOR_ACK0},
/*WAIT_FOR_ACK0*/           {1, 0, 0,  WAIT_FOR_1_FROM_ABOVE},
/*WAIT_FOR_1_FROM_ABOVE*/   {2, 1, -1, WAIT_FOR_ACK1},
/*WAIT_FOR_ACK1*/           {3, 1, 1,  WAIT_FOR_0_FROM_ABOVE}
};
struct RX_State RX_STATE_MACHINE[] = {
/*WAIT_FOR_SEQ0*/   {0, 0, 0, WAIT_FOR_SEQ1},
/*WAIT_FOR_SEQ1*/   {1, 1, 1, WAIT_FOR_SEQ0}
};
int CHANNEL_IN_USE; //used to only allow one message to be sent on the medium at a time
struct TX_State *TX_STATE[2];
struct TX_STATE *ATX_STATE = WAIT_FOR_0_FROM_ABOVE;
struct RX_STATE *ARX_STATE = WAIT_FOR_SEQ0;
struct RX_State *RX_STATE[2];
struct TX_STATE *BTX_STATE = WAIT_FOR_0_FROM_ABOVE;
struct RX_STATE *BRX_STATE = WAIT_FOR_SEQ0;
struct pkt LAST_UNACKED_PKT[2];


/**************** FUNCTION DECLARATIONS ****************/
struct pkt* pkt_init(struct pkt *pkt, int seqnum, int acknum, struct msg *msg);
struct pkt* ackpkt_init(struct pkt *pkt, int acknum);
struct pkt* nakpkt_init(struct pkt *pkt);
int validate_checksum(struct pkt* pkt);
int generate_checksum(struct pkt* pkt);
struct msg* msg_init(struct msg *msg, char** data);
int evaluate_tx_state(struct TX_State *state, struct pkt *pkt);
int evaluate_rx_state(struct RX_State *state, struct pkt *pkt);
int toggle(int binary_num);

/**************** FUNCTION DEFINITIONS ****************/

/******** HELPER FUNCTIONS ********/
struct pkt* pkt_init(struct pkt *pkt, int seqnum, int acknum, struct msg *msg)
{
  pkt->seqnum = seqnum;
  pkt->acknum = acknum;
  strcpy(pkt->payload, msg->data);
  int i;
  pkt->checksum = generate_checksum(pkt);
  return pkt;
}

struct pkt* ackpkt_init(struct pkt *pkt, int acknum)
{
  int i;
  pkt->seqnum = -1;
  pkt->acknum = acknum;
  for (i = 0; i < 20; i++) {
    pkt->payload[i] = "A";
  }
  pkt->checksum = generate_checksum(pkt);
  return pkt;
}

struct pkt* nakpkt_init(struct pkt *pkt)
{
  int i;
  pkt->seqnum = -1;
  pkt->acknum = -2;
  for (i = 0; i < 20; i++) {
    pkt->payload[i] = "A";
  }
  pkt->checksum = generate_checksum(pkt);
  return pkt;
}

int validate_checksum(struct pkt* pkt)
{
  int i;
  int checksum = 0;
  checksum += pkt->seqnum;
  checksum += pkt->acknum;
  for (i = 0; i < 20; i++)
  {
    checksum += (unsigned int)pkt->payload[i];
  }
  return checksum == pkt->checksum;
}

int generate_checksum(struct pkt* pkt)
{
  int i;
  int checksum = 0;
  checksum += pkt->seqnum;
  checksum += pkt->acknum;
  for (i = 0; i < 20; i++)
  {
    checksum += (unsigned int)pkt->payload[i];
  }
  return checksum;
}

struct msg* msg_init(struct msg *msg, char** data)
{
  int i;
  for (i = 0; i < 20; i++)
  {
    msg->data[i] = data[i];
  }
  return msg;
}

int evaluate_tx_state(struct TX_State *state, struct pkt *pkt)
{
    if (!validate_checksum(pkt))
    {
        return PKT_CORRUPT;
    }
    if (pkt->acknum == -2)
    {
        return PKT_NAK;
    }
    if ((state == WAIT_FOR_ACK0 || state == WAIT_FOR_ACK1 ) && state->acknum == pkt->acknum)
    {
        return PKT_OK;
    }
    if ((state == WAIT_FOR_0_FROM_ABOVE && pkt->acknum == 1) ||
        (state == WAIT_FOR_1_FROM_ABOVE && pkt->acknum == 0))
    {
        return PKT_DUP;
    }
    return -1;
}

int evaluate_rx_state(struct RX_State *state, struct pkt *pkt)
{
    if (!validate_checksum(pkt) && state != NULL)
    {
        return PKT_CORRUPT;
    }
    if ((state == WAIT_FOR_SEQ0 || state == WAIT_FOR_SEQ1) && state->seqnum == pkt->seqnum)
    {
        return PKT_OK;
    }
    if ((state == WAIT_FOR_SEQ0 && pkt->seqnum == 1) ||
        (state == WAIT_FOR_SEQ1 && pkt->seqnum == 0))
    {
        return PKT_DUP;
    }
    return -1;
}


/** MISC HELPERS **/
int toggle(int binary_num)
{
  return ((binary_num + 1) % 2);
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
void output(int A_or_B, struct msg *msg, struct pkt *pkt)
{
    /*locals*/
    /*locals init*/
    /*function logic*/
    DEBUG_TX_STATE(A_or_B, TX_STATE[A_or_B]);
    DEBUG_RX_STATE(A_or_B, RX_STATE[A_or_B]);
    if (CHANNEL_IN_USE)
    {
        DEBUG(("Channel already in use"));
        stoptimer(A_or_B);
    }
    pkt_init(pkt, TX_STATE[A_or_B]->seqnum, TX_STATE[A_or_B]->acknum, msg);
    DEBUG(("->Created packet"));
    DEBUG_PKT(pkt);
    tolayer3(A_or_B, *pkt);
    starttimer(A_or_B, TIMER_INCREMENT);
    CHANNEL_IN_USE = 1;
    TX_STATE[A_or_B] = TX_STATE[A_or_B]->next_state;
    LAST_UNACKED_PKT[A_or_B] = *pkt;
    DEBUG(("->Sent packet"));
}

void input(int A_or_B, struct pkt *pkt, struct msg *msg)
{
    /*locals*/
    struct pkt *retpkt;
    /*locals init*/
    retpkt = (struct pkt *)malloc(sizeof(struct pkt));
    /*function logic*/
    DEBUG_TX_STATE(A_or_B, TX_STATE[A_or_B]);
    DEBUG_RX_STATE(A_or_B, RX_STATE[A_or_B]);
    DEBUG(("->Received Packet"));
    DEBUG_PKT(pkt);
    CHANNEL_IN_USE = 0;
    if (A_or_B == A) {
      switch (evaluate_tx_state(TX_STATE[A_or_B], pkt)) {
        case PKT_CORRUPT :
           DEBUG(("-->Packet Corrupted"));
           //Do Nothing
           break;
         case PKT_NAK :
           DEBUG(("-->Packet is a NAK"));
           //Do Nothing
           break;
         case PKT_OK :
           DEBUG(("-->Received ACK"));
           stoptimer(A_or_B);
           TX_STATE[A_or_B] = TX_STATE[A_or_B]->next_state;
           break;
       }
    }
    if (A_or_B == B) {
      switch (evaluate_rx_state(RX_STATE[A_or_B], pkt)) {
        case PKT_CORRUPT :
          DEBUG(("-->Packet Corrupted"));
          nakpkt_init(retpkt);
          tolayer3(A_or_B, *retpkt);
          DEBUG(("->Sent NAK"));
          DEBUG_PKT(retpkt);
          break;
        case PKT_DUP :
          DEBUG(("-->Packet is a duplicate"));
          ackpkt_init(retpkt, pkt->seqnum);
          tolayer3(A_or_B, *retpkt);
          DEBUG(("->Sent ACK%d",retpkt->acknum));
          DEBUG_PKT(retpkt);
          break;
        case PKT_OK :
          DEBUG(("-->Packet contains valid data segment"));
          ackpkt_init(retpkt, pkt->seqnum);
          tolayer3(A_or_B, *retpkt);
          RX_STATE[A_or_B] = RX_STATE[A_or_B]->next_state;
          DEBUG(("->Sent ACK%d",retpkt->acknum));
          DEBUG_PKT(retpkt);
          break;
      }
    }
    free(retpkt);
}

void timerinterrupt(int A_or_B)
{
    DEBUG(("->Timed out"));
    tolayer3(A_or_B, LAST_UNACKED_PKT[A_or_B]);
    starttimer(A_or_B, TIMER_INCREMENT);
    DEBUG(("->Resent packet"));
    DEBUG_PKT((&LAST_UNACKED_PKT[A_or_B]));
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    /*locals*/
    struct pkt *pkt;
    /*locals init*/
    pkt = (struct pkt *)malloc(sizeof(struct pkt));
    /*function logic*/
    DEBUG(("Side 'A'"));
    output(A, &message, pkt);
    free(pkt);
    DEBUG(("\n"));
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    /*local variables*/
    struct msg *msg;
    /*locals init*/
    msg = (struct msg*)malloc(sizeof(struct msg));
    /*function logic*/
    DEBUG(("Side 'A'..."));
    input(A, &packet, msg);
    free(msg);
    DEBUG(("\n"));
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    DEBUG(("Side 'A'..."));
    timerinterrupt(A);
    DEBUG(("\n"));
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    TX_STATE[A] = ATX_STATE;
    RX_STATE[A] = ARX_STATE;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 5, passed the data to be sent to other side */
void B_output(struct msg message)
{
    /*locals*/
    struct pkt *pkt;
    /*locals init*/
    pkt = (struct pkt *)malloc(sizeof(struct pkt));
    /*function logic*/
    DEBUG(("Side 'B'..."));
    output(B, &message, pkt);
    free(pkt);
    DEBUG(("\n"));
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    /*local variables*/
    struct msg *msg;
    /*locals init*/
    msg = (struct msg*)malloc(sizeof(struct msg));
    /*function logic*/
    DEBUG(("Side 'B'..."));
    input(B, &packet, msg);
    free(msg);
    DEBUG(("\n"));
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
    DEBUG(("Side 'B'..."));
    timerinterrupt(B);
    DEBUG(("\n"));
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    TX_STATE[B] = BTX_STATE;
    RX_STATE[B] = BRX_STATE;
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you definitely should not have to modify
******************************************************************/

struct event *evlist = NULL;   /* the event list */
int main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();

   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
   return 0;
}



void init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(1);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
   double x;
   struct event *evptr;
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


void insertevent(struct event *p)
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB, float increment)
{

 struct event *q;
 struct event *evptr;

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 float lastime, x;
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(int AorB, char datasent[20])
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
