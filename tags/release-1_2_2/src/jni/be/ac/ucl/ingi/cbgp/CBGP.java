// ==================================================================
// @(#)CBGP.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 20/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Vector;

import be.ac.ucl.ingi.cbgp.bgp.Peer;
import be.ac.ucl.ingi.cbgp.bgp.Router;
import be.ac.ucl.ingi.cbgp.net.*;

// -----[ CBGP ]-----------------------------------------------------
/**
 * This is the Java Native Interface to the C-BGP simulator. This
 * class is only a wrapper to C functions contained in the csim
 * library (libcsim.so).
 */
public class CBGP
{

    // -----[ Console log levels ]-----------------------------------
    public static int LOG_LEVEL_EVERYTHING= 0;
    public static int LOG_LEVEL_DEBUG     = 1;
    public static int LOG_LEVEL_INFO      = 2;
    public static int LOG_LEVEL_WARNING   = 3;
    public static int LOG_LEVEL_SEVERE    = 4;
    public static int LOG_LEVEL_FATAL     = 5;
    public static int LOG_LEVEL_MAX       = 255;

    /////////////////////////////////////////////////////////////////
    // C-BGP Java Native Interface Initialization/finalization
    /////////////////////////////////////////////////////////////////

    // -----[ init ]-------------------------------------------------
    public native synchronized void init(String sFileLog)
    	throws CBGPException;
    // -----[ destroy ]----------------------------------------------
    public native synchronized void destroy();
    // -----[ consoleSetOutListener ]--------------------------------
    public native synchronized void consoleSetOutListener(ConsoleEventListener l);
    // -----[ consoleSetErrListener ]--------------------------------
    public native synchronized void consoleSetErrListener(ConsoleEventListener l);
    // -----[ consoleSetErrLevel ]-----------------------------------
    public native synchronized void consoleSetLevel(int iLevel);


    /////////////////////////////////////////////////////////////////
    // IGP Domains Management
    /////////////////////////////////////////////////////////////////

    // -----[ netAddDomains ]----------------------------------------
    public native synchronized IGPDomain netAddDomain(int iDomain)
	throws CBGPException;
    // -----[ netGetDomains ]----------------------------------------
    public native synchronized Vector netGetDomains()
	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Nodes management
    /////////////////////////////////////////////////////////////////

    // -----[ netGetNodes ]------------------------------------------
    public native synchronized
	Vector netGetNodes()
	throws CBGPException;
    // -----[ netNodeRouteAdd ]--------------------------------------
    public native synchronized
	void netNodeRouteAdd(String sRouterAddr, String sPrefix,
			     String sNexthop, 
			     int iWeight)
		throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Links management
    /////////////////////////////////////////////////////////////////

    // -----[ netAddLink ]-------------------------------------------
    public native synchronized
	Link netAddLink(String sSrcAddr, String sDstAddr, int iWeight) 
	throws CBGPException;
    
    
    /////////////////////////////////////////////////////////////////
    // BGP domains management
    /////////////////////////////////////////////////////////////////

    // -----[ bgpGetDomains ]----------------------------------------
    public native synchronized
	Vector bgpGetDomains()
	throws CBGPException;

    /////////////////////////////////////////////////////////////////
    // BGP router management
    /////////////////////////////////////////////////////////////////

    // -----[ bgpAddRouter ]-----------------------------------------
    public native synchronized
	Router bgpAddRouter(String sName, String sAddr, int iASNumber)
	throws CBGPException;
    // -----[ bgpRouterPeerReflectorClient ]-------------------------
    public native synchronized
	void bgpRouterPeerReflectorClient(String sRouterAddr,
					  String sPeerAddr)
	throws CBGPException;
    // -----[ bgpRouterPeerVirtual ]---------------------------------
    public native synchronized
	void bgpRouterPeerVirtual(String sRouterAddr,
				  String sPeerAddr)
	throws CBGPException;
    // -----[ bgpRouterLoadRib ]-------------------------------------
    public native void bgpRouterLoadRib(String sRouterAddr, String sFileName)
	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Miscellaneous methods
    /////////////////////////////////////////////////////////////////

    // -----[ simRun ]-----------------------------------------------
    public native synchronized void simRun()
    	throws CBGPException;
    // -----[ simStep ]----------------------------------------------
    public native synchronized void simStep(int iNUmSteps)
    	throws CBGPException;
    // -----[ simClear ]---------------------------------------------
    public native synchronized void simClear()
    	throws CBGPException;
    // -----[ simGetCount ]------------------------------------------
    public native synchronized long simGetEventCount()
    	throws CBGPException;
    // -----[ runCmd ]-----------------------------------------------
    public native synchronized void runCmd(String line)
    	throws CBGPException;
    // -----[ runScript ]--------------------------------------------
    public native synchronized void runScript(String sFileName)
		throws CBGPException;
    // -----[ cliGetPrompt ]----------------------------------------
    public native synchronized String cliGetPrompt()
    	throws CBGPException;
    // -----[ cliGetHelp ]------------------------------------------
    // -----[ cliGetSyntax ]----------------------------------------
    // -----[ getVersion ]------------------------------------------
    public native synchronized String getVersion()
    	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Experimental features
    /////////////////////////////////////////////////////////////////

    // -----[ loadMRT ]----------------------------------------------
    public native ArrayList loadMRT(String sFileName);


    /* BQU: TO BE FIXED !!!
       Work done by Sebastien and never finished... :-( I think this
       part has to be completely rewritten since it lacks
       genericity. In addition, Sebastien's JNI implementation has no
       error control...

       public native int nodeInterfaceAdd(String net_addr_id,
       String net_addr_int, String mask);

       //Type = [in|out]
       public native int bgpFilterInit(String net_addr_router,
       String net_addr_neighbor, String type); 
       
       public native int bgpFilterMatchPrefixIn(String prefix);

       public native int bgpFilterMatchPrefixIs(String prefix);

       //types supported :
       //   0x01 permit
       //   0x02 deny
       //   0x03 set-localpref
       //   0x04 add-community
       //   0x05 path-prepend
       public native int bgpFilterAction(int type, 
       String value);

       public native void bgpFilterFinalize();
    */

    // -----[ showLinks ]--------------------------------------------
    private static void showLinks(Vector links)
    {
	System.out.println("========================================"+
			   "=======================================");
	System.out.println("\033[1mAddr\tDelay\tWeight\tStatus\033[0m");
	if (links != null) {
	    for (Enumeration linksEnum= links.elements();
		 linksEnum.hasMoreElements();) {
		Link link= (Link) linksEnum.nextElement();
		System.out.println(link);
	    }
	} else {
	    System.out.println("(null)");
	}
	System.out.println("========================================"+
			   "=======================================");
    }

    // -----[ showRoutes ]-------------------------------------------
    private static void showRoutes(Vector routes, boolean bBGP)
    {
	System.out.println("========================================"+
    	"=======================================");
	if (bBGP) {
	    System.out.println("   \033[1mPrefix\tNxt-hop\tLoc-prf\tMED\tPath\tOrigin\033[0m");
	} else {
	    System.out.println("   \033[1mPrefix\tNxt-hop\033[0m");
	}
	if (routes != null) {
	    for (Enumeration routesEnum= routes.elements();
		 routesEnum.hasMoreElements();) {
		Route route= (Route) routesEnum.nextElement();
		System.out.println(route);
	    }
	} else {
	    System.out.println("(null)");
	}
	System.out.println("========================================"+
			   "=======================================");
    }

    // -----[ showRoutes ]-------------------------------------------
    private static void showRoutes(ArrayList routes, boolean bBGP)
    {
	System.out.println("========================================"+
			   "=======================================");
	if (bBGP) {
	    System.out.println("   \033[1mPrefix\tNxt-hop\tLoc-prf\tMED\tPath\tOrigin\033[0m");
	} else {
	    System.out.println("   \033[1mPrefix\tNxt-hop\033[0m");
	}
	if (routes != null) {
	    for (int iIndex= 0; iIndex < routes.size(); iIndex++) {
		Route route= (Route) routes.get(iIndex);
		System.out.println(route);
	    }
	} else {
	    System.out.println("(null)");
	}
	System.out.println("========================================"+
			   "=======================================");
    }

    // -----[ showNodes ]--------------------------------------------
    private static void showNodes(Vector nodes)
    {
	System.out.println("========================================"+
			   "=======================================");
	System.out.println("\033[1mIP addr\033[0m");

	if (nodes != null) {
	    for (Enumeration nodesEnum= nodes.elements();
		 nodesEnum.hasMoreElements();) {
		Node node= (Node) nodesEnum.nextElement();
		System.out.println(node);
	    }
	} else {
	    System.out.println("(null)");
	}
	System.out.println("========================================"+
			   "=======================================");
    }

    public static void main(String[] args){
	CBGP cbgp= new CBGP();

	if (args.length > 0) {

	    showRoutes(cbgp.loadMRT(args[0]), true);

	} else {

	    try {

		System.out.println("Initializing C-BGP...");
		cbgp.init("/tmp/log_cbgp_jni");
	    
		//this example has been translated from 
		//http://cbgp.info.ucl.ac.be/downloads/valid-igp-link-up-down.cli
		System.out.println("*** \033[34;1mvalid-igp-link-up-down \033[0m***");
		
		//Domain AS1
		System.out.println("Create domain 1");
		IGPDomain domain1= cbgp.netAddDomain(1);
		domain1.addNode("0.1.0.1");
		showNodes(domain1.getNodes());
		
		//Domain AS2
		System.out.println("Create domain 2");
		IGPDomain domain2= cbgp.netAddDomain(2);
		domain2.addNode("0.2.0.1");
		domain2.addNode("0.2.0.2");
		Node node23= domain2.addNode("0.2.0.3");
		domain2.addNode("0.2.0.4");
		Link link21_23= cbgp.netAddLink("0.2.0.1", "0.2.0.3", 20);
		cbgp.netAddLink("0.2.0.2", "0.2.0.3", 10);
		Link link23_24= cbgp.netAddLink("0.2.0.3", "0.2.0.4", 10);
		showNodes(domain2.getNodes());

		// Intradomain routes
		System.out.println("Compute IGP routes...");
		domain1.compute();
		domain2.compute();

		//Interdomain links and routes
		cbgp.netAddLink("0.1.0.1", "0.2.0.1", 0);
		cbgp.netAddLink("0.1.0.1", "0.2.0.2", 0);
		cbgp.netAddLink("0.1.0.1", "0.2.0.4", 0);
		cbgp.netNodeRouteAdd("0.1.0.1", "0.2.0.1/32", "0.2.0.1", 0);
		cbgp.netNodeRouteAdd("0.1.0.1", "0.2.0.2/32", "0.2.0.2", 0);
		cbgp.netNodeRouteAdd("0.1.0.1", "0.2.0.4/32", "0.2.0.4", 0);
		cbgp.netNodeRouteAdd("0.2.0.1", "0.1.0.1/32", "0.1.0.1", 0);
		cbgp.netNodeRouteAdd("0.2.0.2", "0.1.0.1/32", "0.1.0.1", 0);
		cbgp.netNodeRouteAdd("0.2.0.4", "0.1.0.1/32", "0.1.0.1", 0);

		Router router= cbgp.bgpAddRouter("Router1_AS1", "0.1.0.1", 1);
		router.addNetwork("0.1/16");
		Peer peer= router.addPeer("0.2.0.1", 2);
		/*cbgp.bgpFilterInit("0.1.0.1", "0.2.0.1", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x05, "3");
		  //	cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		peer.openSession();
		peer= router.addPeer("0.2.0.2", 2);
		/*cbgp.bgpFilterInit("0.1.0.1", "0.2.0.2", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x05, "3");
		  //cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		peer.openSession();
		peer= router.addPeer("0.2.0.4", 2);
		/*cbgp.bgpFilterInit("0.1.0.1", "0.2.0.4", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x05, "3");
		  //cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		peer.openSession();
								    
		//BGP in AS2
		router= cbgp.bgpAddRouter("Router1_AS2", "0.2.0.1", 2);
		Router router21= router;
		peer= router.addPeer("0.1.0.1", 1);
		peer.setNextHopSelf(true);
		peer.openSession();
		peer= router.addPeer("0.2.0.2", 2);
		peer.openSession();
		peer= router.addPeer("0.2.0.3", 2);
		peer.openSession();
		peer= router.addPeer("0.2.0.4", 2);
		peer.openSession();

		router= cbgp.bgpAddRouter("Router2_AS2", "0.2.0.2", 2);
		Router router22= router;
		peer= router.addPeer("0.1.0.1", 1);
		peer.setNextHopSelf(true);
		peer.openSession();
		peer= router.addPeer("0.2.0.1", 2);
		peer.openSession();
		peer=router.addPeer("0.2.0.3", 2);
		peer.openSession();
		peer=router.addPeer("0.2.0.4", 2);
		peer.openSession();

		router= cbgp.bgpAddRouter("Router3_AS2", "0.2.0.3", 2);
		Router router23= router;
		peer= router.addPeer("0.2.0.1", 2);
		/*cbgp.bgpFilterInit("0.2.0.3", "0.2.0.1", "in");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x03, "1000");
		  cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		/*cbgp.bgpFilterInit("0.2.0.3", "0.2.0.1", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x03, "1000");
		  cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		peer.openSession();
		peer=router.addPeer("0.2.0.2", 2);
		/*cbgp.bgpFilterInit("0.2.0.3", "0.2.0.2", "in");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x03, "1000");
		  cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		/*cbgp.bgpFilterInit("0.2.0.3", "0.2.0.2", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x03, "1000");
		  cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		peer.openSession();
		peer= router.addPeer("0.2.0.4", 2);
		/*cbgp.bgpFilterInit("0.2.0.3", "0.2.0.4", "in");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x03, "1000");
		  cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();
		  cbgp.bgpFilterInit("0.2.0.3", "0.2.0.4", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x03, "1000");
		  cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		peer.openSession();

		router= cbgp.bgpAddRouter("Router4_AS2", "0.2.0.4", 2);
		peer=router.addPeer("0.1.0.1", 1);
		peer.setNextHopSelf(true);
		peer.openSession();
		peer= router.addPeer("0.2.0.1", 2);
		peer.openSession();
		peer= router.addPeer("0.2.0.2", 2);
		peer.openSession();
		peer= router.addPeer("0.2.0.3", 2);
		peer.openSession();

		cbgp.simRun();

		//Section added to demonstrate the possibility to change the 
		//IGP weight
		System.out.println("Links of 0.2.0.3 before weight change:");
		//For the moment on stdout
		showLinks(node23.getLinks());
		System.out.println("RT of 0.2.0.3 before weight change:");
		showRoutes(node23.getRT(null), false);
		System.out.println("RIB of 0.2.0.3 before weight change;");
		showRoutes(router23.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
//BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		/*IPTrace trace= cbgp.netNodeRecordRoute("0.2.0.3", "0.1.0.1");
		System.out.println(trace);*/

		System.out.println("\n<Weight change : 0.2.0.3 <-> 0.2.0.1 from 20 to 5>\n");
		link21_23.setWeight(5);

		//Update intradomain routes (recompute MST)
		domain2.compute();

		//Scan BGP routes that could have changed
//BQU		bgp_domain2.rescan();

		cbgp.simRun();
		//End of the added section 
 
		System.out.println("Links of 0.2.0.3 before first link down:");
		//For the moment on stdout
		showLinks(node23.getLinks());
		System.out.println("RT of 0.2.0.3 before first link down:");
		showRoutes(node23.getRT(null), false);
		System.out.println("RIB of 0.2.0.3 before first link down:");
		showRoutes(router23.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
//BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
		System.out.println("\n\033[31;1m<<First link down: 0.2.0.1 <-> 0.2.0.3>>\033[0m\n");

		link21_23.setState(false);

		//Update intradomain routes (recompute MST)
		domain2.compute();

		//Scan BGP routes that could change
//BQU		cbgp.bgpDomainRescan(2);

		cbgp.simRun();
   

		System.out.println("Links of 0.2.0.3 after first link down:");
		//For the moment on stdout
		showLinks(node23.getLinks());
		System.out.println("RT of 0.2.0.3 after first link down:");
		showRoutes(node23.getRT(null), false);
		System.out.println("RIB of 0.2.0.3 after first link down:");
		showRoutes(router23.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
//BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		System.out.println("\n\033[31;1m<<Second link down: 0.2.0.2 <-> 0.2.0.3>>\033[0m\n");

		link21_23.setState(false);

		//Update intradomain routes (recompute MST)
		domain2.compute();

		//Scan BGP routes that could change
//BQU		cbgp.bgpDomainRescan(2);

		cbgp.simRun();

		System.out.println("Links of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showLinks(node23.getLinks());
		System.out.println("RT of 0.2.0.3 after second link down:");
		showRoutes(node23.getRT(null), false);
		System.out.println("RIB of 0.2.0.3 after second link down:");
		showRoutes(router23.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
//BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		System.out.println("\n\033[31;1m<<Third link down: 0.2.0.3 <-> 0.2.0.4>>\033[0m\n");

		link23_24.setState(false);

		//Update intradomain routes (recompute MST)
		domain2.compute();

		//Scan BGP routes that could change
//BQU		cbgp.bgpDomainRescan(2);

		cbgp.simRun();

		System.out.println("Links of 0.2.0.3 after third link down:");
		//For the moment on stdout
		showLinks(node23.getLinks());
		System.out.println("RT of 0.2.0.3 after third link down:");
		showRoutes(node23.getRT(null), false);
		System.out.println("RIB of 0.2.0.3 after third link down:");
		showRoutes(router23.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
//BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
		System.out.println("RIB of 0.2.0.1 after third link down:");
		showRoutes(router21.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.1 after second link down:");
		//For the moment on stdout
// BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.1", null, null, true), true);
 
		System.out.println("RIB of 0.2.0.2 after third link down:");
		showRoutes(router22.getRIB(null), true);
		System.out.println("RIB In of 0.2.0.2 after second link down:");
		//For the moment on stdout
//BQU		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.2", null, null, true), true);

		/*for (Enumeration enumPeers= cbgp.bgpRouterGetPeers("0.2.0.2").elements();
		     enumPeers.hasMoreElements(); ) {
		    System.out.println(enumPeers.nextElement());
		}*/
 
		System.out.println("\n\033[32;1mDone\033[0m.\n");

		cbgp.destroy();

	    } catch (CBGPException e) {
		System.out.println("EXCEPTION: "+e.getMessage());
		e.printStackTrace();
	    }
	}
    }
    
    static {
	try {
	    //System.loadLibrary("pcre");
	    //System.loadLibrary("gds");
	    System.loadLibrary("csim");
	} catch (UnsatisfiedLinkError e) {
	    System.err.println("Could not load one of the libraries "+
			       "required by CBGP (reason: "+
			       e.getMessage()+")");
	    e.printStackTrace();
	    System.exit(-1);
	}
    }

}

    
