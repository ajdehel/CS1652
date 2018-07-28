import java.util.Arrays;

public class Entity1 extends Entity
{

    public static final int INFINITY = 999;

    protected int label;

    protected int[] directCosts = new int[NetworkSimulator.NUMENTITIES];

    protected int[] neighbors;

    // Perform any necessary initialization in the constructor
    public Entity1()
    {
        this.label = 1;
        this.neighbors = new int[]{0, 2};
        this.directCosts = new int[]{1, 0, 1, INFINITY};
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
        //super(1, new int[]{1, 0, 1, INFINITY}, new int[]{0, 2});
    }
    
    // Handle updates when a packet is received.  Students will need to call
    // NetworkSimulator.toLayer2() with new packets based upon what they
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
        System.out.println("         via");
        System.out.println("  D1 |   0   2");
        System.out.println(" ----+--------");
        for (int i = 0; i < NetworkSimulator.NUMENTITIES; i++)
        {
            if (i == 1)
            {
                continue;
            }
            
            System.out.print("    " + i + "|");
            for (int j = 0; j < NetworkSimulator.NUMENTITIES; j += 2)
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
