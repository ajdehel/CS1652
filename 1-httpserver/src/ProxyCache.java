/**
 * ProxyCache.java - Simple caching proxy
 *
 * $Id: ProxyCache.java,v 1.3 2004/02/16 15:22:00 kangasha Exp $
 *
 */

import java.net.*;
import java.io.*;
import java.util.*;

public class ProxyCache {
    /** Port for the proxy */
    private static int port;
    /** Socket for client connections */
    private static ServerSocket socket;

    /** Create the ProxyCache object and the socket */
    public static void init(int p) {
        port = p;
        try {
            socket = new ServerSocket(port)/* Fill in */;
        } catch (IOException e) {
            System.out.println("Error creating socket: " + e);
            System.exit(-1);
        }
    }

    /** Read command line arguments and start proxy */
    public static void main(String args[]) {
        int myPort = 0;

        try {
            myPort = Integer.parseInt(args[0]);
        } catch (ArrayIndexOutOfBoundsException e) {
            System.out.println("Need port number as argument");
            System.exit(-1);
        } catch (NumberFormatException e) {
            System.out.println("Please give port number as integer.");
            System.exit(-1);
        }

        init(myPort);

        /** Main loop. Listen for incoming connections and spawn a new
         * thread for handling them */
        Socket client = null;

        while (true) {
            try {
                client = socket.accept()/* Fill in */;
                ProxyRunnable pr = new ProxyRunnable(client);
                Thread thread = new Thread(pr);
                thread.start();
            } catch (IOException e) {
                System.out.println("Error reading request from client: " + e);
                /* Definitely cannot continue processing this request,
                 * so skip to next iteration of while loop. */
                continue;
            }
        }

    }
}

final class ProxyRunnable implements Runnable {

    /** HashMap to cache Responses */
    static HashMap<String,HttpResponse> cache = new HashMap<String, HttpResponse>();

    private Socket client;

    ProxyRunnable(Socket client) {
        this.client = client;
    }
    @Override
    public void run() {
        /* Fill in */;        
        handle();
    }

    private void handle() {
        Socket server = null;
        HttpRequest request = null;
        HttpResponse response = null;

        /* Process request. If there are any exceptions, then simply
         * return and end this request. This unfortunately means the
         * client will hang for a while, until it timeouts. */

        /* Read request */
        try {
            DataInputStream fromClient = new DataInputStream(client.getInputStream())/* Fill in */;
            request = new HttpRequest(fromClient)/* Fill in */;
        } catch (IOException e) {
            System.out.println("Error reading request from client: " + e);
            return;
        }
        /* Fill in */
        /* Serve the requested object from cache
         * if has been previously cached.
         */
        if (request.method.equals("GET") && cache.containsKey(request.URI)) {
            System.out.println("Using cached copy of " + request.URI);
            response = cache.get(request.URI);
            try {
                DataOutputStream toClient = new DataOutputStream(client.getOutputStream());/* Fill in */;
                /* Write response to client. First headers, then body */
                toClient.writeBytes(response.toString() + new String(response.body)/* Fill in */);
                client.close();
            } catch (IOException e) {
                System.out.println("Error writing response to client: " + e);
            }
        } else {
            /* Send request to server */
            try {
                /* Open socket and write request to socket */
                server = new Socket(request.getHost(), request.getPort())/* Fill in */;
                DataOutputStream toServer = new DataOutputStream(server.getOutputStream());

                /* Fill in */
                if(request.isPost) {
                    toServer.writeBytes(request.toString() + new String(request.body));
                } else {
                    toServer.writeBytes(request.toString());
                }
            } catch (UnknownHostException e) {
                System.out.println("Unknown host: " + request.getHost());
                System.out.println(e);
                return;
            } catch (IOException e) {
                System.out.println("Error writing request to server: " + e);
                return;
            }
            /* Read response and forward it to client */
            try {
                DataInputStream fromServer = new DataInputStream(server.getInputStream());/* Fill in */;
                response = new HttpResponse(fromServer);/* Fill in */;
                DataOutputStream toClient = new DataOutputStream(client.getOutputStream());/* Fill in */;
                /* Insert object into the cache */
                cache.put(request.URI, response);
                System.out.println("Cache entry added for URI: " + request.URI);
                /* Write response to client. First headers, then body */
                toClient.writeBytes(response.toString() + new String(response.body)/* Fill in */);
                /* Fill in */
                client.close();
                server.close();

                /* Fill in */
            } catch (IOException e) {
                System.out.println("Error writing response to client: " + e);
            }
        }
    }
}
