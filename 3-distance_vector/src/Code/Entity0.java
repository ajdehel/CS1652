import java.util.Arrays;

public class Entity0 extends Entity
{

    public static final int INFINITY = 999;

    protected int label;

    protected int[] directCosts = new int[NetworkSimulator.NUMENTITIES];

    protected int[] neighbors;

    // Perform any necessary initialization in the constructor
    public Entity0()
    {
        this.label = 0;
        this.neighbors = new int[]{1, 2, 3};
        this.directCosts = new int[]{0, 1, 3, 7};
        //initialize all entries in the table to "infinity"
        for (int i = 0; i < NetworkSimulator.NUMENTITIES; i++) {
            for (int j = 0; j < NetworkSimulator.NUMENTITIES; j++) {
                this.distanceTable[i][j] = INFINITY;
            }
        }
        //initialize cost to itself as 0
        this.distanceTable[this.label][this.label] = 0;
        for (int j = 0; j < NetworkSimulator.NUMENTITIES; j++) {
            this.distanceTable[this.label][j] = this.directCosts[j];
        }
        System.out.println("");
        System.out.println("+=========================+");
        System.out.printf( "|At Time %-16f |\n", NetworkSimulator.getTime());
        System.out.println("+=========================+=========================+");
        this.sendUpdates();
        this.printCosts();
        System.out.println("+===================================================+");
        //super(0, new int[]{0, 1, 3, 7}, new int[]{1, 2, 3});
    }
    
    // Handle updates when a packet is received.  You will need to call
    // NetworkSimulator.toLayer2() with new packets based upon what you
    // send to update.  Be careful to construct the source and destination of
    // the packet correctly.  Read the warning in NetworkSimulator.java for more
    // details.


    public void update(Packet p)
    {
        /*locals*/
        int src;
        boolean costsChanged;
        /*init locals*/
        src = p.getSource();
        costsChanged = false;
        /*function body*/
        System.out.println("");
        System.out.println("+=========================+");
        System.out.printf( "|At Time %-16f |\n", NetworkSimulator.getTime());
        System.out.println("+=========================+=========================+");
        System.out.println(" Node:" + this.label + " Received Update from Node:" + src);
        //update table row for src
        for ( int i = 0; i < NetworkSimulator.NUMENTITIES; i++ ) {
            this.distanceTable[src][i] = p.getMincost(i);
        }
        costsChanged = computeEstimates();
        if ( costsChanged ) {
            System.out.println(" Node:" + this.label + " Changed estimates" );
            this.sendUpdates();
        }
        this.printDT();
        this.printCosts();
        System.out.println("+===================================================+");
    }

    public void linkCostChangeHandler(int whichLink, int newCost)
    {
        /*locals*/
        Packet p;
        /*function body*/
        this.directCosts[whichLink] = newCost;
        System.out.println("");
        System.out.println("+=========================+");
        System.out.printf( "|At Time %-16f |\n", NetworkSimulator.getTime());
        System.out.println("+=========================+=========================+");
        System.out.println(" Node:" + this.label + " Changed cost to " + whichLink + " to " + newCost + ".");
        this.computeEstimates();
        this.sendUpdates();
        this.printCosts();
        System.out.println("+===================================================+");
    }

    public void printDT()
    {
        System.out.println();
        System.out.println("           via");
        System.out.println("  D0 |   1   2   3");
        System.out.println(" ----+------------");
        for (int i = 1; i < NetworkSimulator.NUMENTITIES; i++)
        {
            System.out.print("    " + i + "|");
            for (int j = 1; j < NetworkSimulator.NUMENTITIES; j++)
            {
                if (distanceTable[i][j] < 10)
                {    
                    System.out.print("   ");
                }
                else if (distanceTable[i][j] < 100)
                {
                    System.out.print("  ");
                }
                else 
                {
                    System.out.print(" ");
                }
                
                System.out.print(distanceTable[i][j]);
            }
            System.out.println();
        }
    }

    protected boolean computeEstimates()
    {
        /*locals*/
        int tempCost;
        int src;
        int[] tempCosts;
        int minCost;
        boolean costsChanged;
        /*init locals*/
        tempCosts = new int[NetworkSimulator.NUMENTITIES];
        costsChanged = false;
        minCost = INFINITY;
        /*function body*/
        for ( int y = 0; y < NetworkSimulator.NUMENTITIES; y++ ) {
            minCost = INFINITY;
            if ( y != this.label ) {
                for ( int v = 0; v < NetworkSimulator.NUMENTITIES; v++ ) {
                    if ( v != this.label ) {
                        tempCost = this.directCosts[v] + this.distanceTable[v][y];
                        minCost = Math.min(minCost, tempCost);
                    }
                }
                tempCosts[y] = minCost;
            }
        }
        for ( int i = 0; i < NetworkSimulator.NUMENTITIES; i++ ) {
            if ( this.distanceTable[this.label][i] != tempCosts[i] ) {
                this.distanceTable[this.label][i] = tempCosts[i];
                costsChanged = true;
            }
        }
        return costsChanged;
    }

    protected void sendUpdates()
    {
        Packet updatePacket;
        for ( int i = 0; i < this.neighbors.length; i++ ) {
            updatePacket = new Packet(this.label, neighbors[i], this.distanceTable[this.label]);
            NetworkSimulator.toLayer2(updatePacket);
            System.out.println("    " + updatePacket.toString());
        }
        System.out.println(" Node:" + this.label + " Sent updates to neighbors" );
    }

    public void printCosts()
    {
        System.out.println(" Node:" + this.label + " Estimates: " + Arrays.toString(this.distanceTable[this.label]) );
    }

}
