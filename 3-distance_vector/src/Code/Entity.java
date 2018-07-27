import java.util.Arrays;

public class Entity extends AbstractEntity
{

    public static final int INFINITY = 999;

    private int label;

    private int[] directCosts = new int[NetworkSimulator.NUMENTITIES];

    protected int[] neighbors;

    public Entity()
    {
    }

    public Entity(int label, int[] costs, int[] neighbors)
    {
        this.label = label;
        this.neighbors = neighbors;
        this.directCosts = costs;
        //initialize all entries in the table to "infinity"
        for (int i = 0; i < NetworkSimulator.NUMENTITIES; i++) {
            for (int j = 0; j < NetworkSimulator.NUMENTITIES; j++) {
                this.distanceTable[i][j] = INFINITY;
            }
        }
        //initialize cost to itself as 0
        this.distanceTable[this.label][this.label] = 0;
        for (int j = 0; j < NetworkSimulator.NUMENTITIES; j++) {
            this.distanceTable[this.label][j] = costs[j];
        }
        System.out.println("");
        System.out.println("+========================+");
        System.out.printf( "|At Time %-15f |\n", NetworkSimulator.getTime());
        System.out.println("+========================+========================+");
        this.printCosts();
        this.sendUpdates();
        System.out.println("+=================================================+");
    }

    private boolean computeEstimates()
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

    private void sendUpdates()
    {
        Packet updatePacket;
        for ( int i = 0; i < this.neighbors.length; i++ ) {
            updatePacket = new Packet(this.label, neighbors[i], this.distanceTable[this.label]);
            NetworkSimulator.toLayer2(updatePacket);
            System.out.println(updatePacket.toString());
        }
        System.out.println("Node:" + this.label + " Sent updates to neighbors" );
    }

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
        System.out.println("+========================+");
        System.out.printf( "|At Time %-15f |\n", NetworkSimulator.getTime());
        System.out.println("+========================+========================+");
        System.out.println("Node:" + this.label + " Received Update from Node:" + src);
        //update table row for src
        for ( int i = 0; i < NetworkSimulator.NUMENTITIES; i++ ) {
            this.distanceTable[src][i] = p.getMincost(i);
        }
        costsChanged = computeEstimates();
        if ( costsChanged ) {
            System.out.println("Node:" + this.label + " Changed estimates" );
            this.sendUpdates();
        }
        this.printDT();
        System.out.println("+=================================================+");
    }

    public void linkCostChangeHandler(int whichLink, int newCost)
    {
        /*locals*/
        Packet p;
        /*function body*/
        this.directCosts[whichLink] = newCost;
        System.out.println("");
        System.out.println("+========================+");
        System.out.printf( "|At Time %-15f |\n", NetworkSimulator.getTime());
        System.out.println("+========================+========================+");
        System.out.println("Node:" + this.label + " Changed cost to " + whichLink + " to " + newCost + ".");
        this.computeEstimates();
        this.sendUpdates();
        System.out.println("+=================================================+");
    }

    // Print the distance table of the current entity.
    protected void printDT()
    {
    }

    public void printCosts()
    {
        System.out.println("Node:" + this.label + " Costs: " + Arrays.toString(this.distanceTable[this.label]) );
    }
}

abstract class AbstractEntity
{
    // Each entity will have a distance table
    protected int[][] distanceTable = new int[NetworkSimulator.NUMENTITIES]
                                           [NetworkSimulator.NUMENTITIES];

    // The update function.  Will have to be written in subclasses by you
    public abstract void update(Packet p);

    // The link cost change handler.  Will have to be written in appropriate
    // subclasses by you.  Note that only Entity0 and Entity1 will need
    // this
    public abstract void linkCostChangeHandler(int whichLink, int newCost);

    // Print the distance table of the current entity.
    protected abstract void printDT();

}


