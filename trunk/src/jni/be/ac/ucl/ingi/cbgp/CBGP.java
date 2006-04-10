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
    public native synchronized void init(String sFileLog);
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
    public native synchronized void netAddDomain(int iDomain)
	throws CBGPException;
    // -----[ netGetDomains ]----------------------------------------
    public native synchronized Vector netGetDomains()
	throws CBGPException;
    // -----[ netDomainGetNodes ]------------------------------------
    public native synchronized Vector netDomainGetNodes(int iNumber)
	throws CBGPException;
    // -----[ netDomainCompute ]-------------------------------------
    public native synchronized void netDomainCompute(int iNumber)
	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Nodes management
    /////////////////////////////////////////////////////////////////

    // -----[ netAddNode ]-------------------------------------------
    public native synchronized
	void netAddNode(String sAddr, int iDomain)
	throws CBGPException;
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
    // -----[ netNodeGetLinks ]--------------------------------------
    public native synchronized
	Vector netNodeGetLinks(String sAddr)
	throws CBGPException;
    // -----[ netNodeGetRT ]-----------------------------------------
    public native synchronized
	Vector netNodeGetRT(String sNodeAddr, String sPrefix)
	throws CBGPException;
    // -----[ netNodeRecordRoute ]-----------------------------------
    public native synchronized
	IPTrace netNodeRecordRoute(String sNodeAddr,
				   String sDstAddr)
	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Links management
    /////////////////////////////////////////////////////////////////

    // -----[ netAddLink ]-------------------------------------------
    public native synchronized
	void netAddLink(String sSrcAddr, String sDstAddr, int iWeight) 
	throws CBGPException;
    // -----[ netLinkWeight ]----------------------------------------
    public native synchronized
	void netLinkWeight(String sSrcAddr, String sDstAddr,
			   int iWeight)
	throws CBGPException;
    // -----[ netLinkUp ]--------------------------------------------
    public native synchronized
	void netLinkUp(String sSrcAddr, String sDstAddr, boolean bUp)
	throws CBGPException;

    
    /////////////////////////////////////////////////////////////////
    // BGP domains management
    /////////////////////////////////////////////////////////////////

    // -----[ bgpGetDomains ]----------------------------------------
    public native synchronized
	Vector bgpGetDomains()
	throws CBGPException;
    // -----[ bgpDomainGetRouters ]----------------------------------
    public native synchronized
	Vector bgpDomainGetRouters(int id)
	throws CBGPException;

    /////////////////////////////////////////////////////////////////
    // BGP router management
    /////////////////////////////////////////////////////////////////

    // -----[ bgpAddRouter ]-----------------------------------------
    public native synchronized
	int bgpAddRouter(String sName, String sAddr, int iASNumber)
	throws CBGPException;
    // -----[ bgpRouterAddNetwork ]----------------------------------
    public native synchronized
	void bgpRouterAddNetwork(String sRouterAddr, String sPrefix)
	throws CBGPException;
    // -----[ bgpRouterAddPeer ]-------------------------------------
    public native synchronized
	void bgpRouterAddPeer(String sRouterAddr, String sPeerAddr, 
			      int iASNumber)
	throws CBGPException;
    // -----[ bgpRouterPeerNextHopSelf ]-----------------------------
    public native synchronized
	void bgpRouterPeerNextHopSelf(String sRouterAddr,
				      String sPeerAddr)
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
    // -----[ bgpRouterPeerUp ]--------------------------------------
    public native synchronized
	void bgpRouterPeerUp(String sRouterAddr,
			     String sRouterPeer,
			     boolean bUp)
	throws CBGPException;
    // -----[ bgpRouterPeerRecv ]------------------------------------
    public native synchronized
	void bgpRouterPeerRecv(String sRouterAddr, 
			       String sRouterPeer,
			       String sMesg)
	    throws CBGPException;
    // -----[ bgpRouterRescan ]--------------------------------------
    public native void bgpRouterRescan(String net_addr)
	throws CBGPException;
    // -----[ bgpRouterGetPeers ]------------------------------------
    public native Vector bgpRouterGetPeers(String sRouterAddr)
	throws CBGPException;
    // -----[ bgpRouterGetRib ]--------------------------------------
    public native Vector bgpRouterGetRib(String sRouterAddr, String sPrefix)
	throws CBGPException;
    // -----[ bgpRouterGetAdjRib ]-----------------------------------
    public native Vector bgpRouterGetAdjRib(String sRouterAddr,
					    String sNeighborAddr,
					    String sPrefix,
					    boolean bIn)
	throws CBGPException;
    // -----[ bgpRouterLoadRib ]-------------------------------------
    public native void bgpRouterLoadRib(String sRouterAddr, String sFileName)
	throws CBGPException;
    // -----[ bgpDomainRescan ]--------------------------------------
    public native void bgpDomainRescan(int iASNumber)
	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Miscellaneous methods
    /////////////////////////////////////////////////////////////////

    // -----[ simRun ]-----------------------------------------------
    public native void simRun() throws CBGPException;
    // -----[ simStep ]-----------------------------------------------
    public native void simStep(int iNUmSteps) throws CBGPException;
    // -----[ runCmd ]-----------------------------------------------
    public native void runCmd(String line) throws CBGPException;
    // -----[ runScript ]--------------------------------------------
    public native void runScript(String sFileName)
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
		cbgp.netAddDomain(1);
		cbgp.netAddNode("0.1.0.1", 1);
		showNodes(cbgp.netDomainGetNodes(1));
		
		//Domain AS2
		System.out.println("Create domain 2");
		cbgp.netAddDomain(2);
		cbgp.netAddNode("0.2.0.1", 2);
		cbgp.netAddNode("0.2.0.2", 2);
		cbgp.netAddNode("0.2.0.3", 2);
		cbgp.netAddNode("0.2.0.4", 2);
		cbgp.netAddLink("0.2.0.1", "0.2.0.3", 20);
		cbgp.netAddLink("0.2.0.2", "0.2.0.3", 10);
		cbgp.netAddLink("0.2.0.3", "0.2.0.4", 10);
		showNodes(cbgp.netDomainGetNodes(2));

		// Intradomain routes
		System.out.println("Compute IGP routes...");
		cbgp.netDomainCompute(2);

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

		cbgp.bgpAddRouter("Router1_AS1", "0.1.0.1", 1);
		cbgp.bgpRouterAddNetwork("0.1.0.1", "0.1/16");
		cbgp.bgpRouterAddPeer("0.1.0.1", "0.2.0.1", 2);
		/*cbgp.bgpFilterInit("0.1.0.1", "0.2.0.1", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x05, "3");
		  //	cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		cbgp.bgpRouterAddPeer("0.1.0.1", "0.2.0.2", 2);
		/*cbgp.bgpFilterInit("0.1.0.1", "0.2.0.2", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x05, "3");
		  //cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		cbgp.bgpRouterAddPeer("0.1.0.1", "0.2.0.4", 2);
		/*cbgp.bgpFilterInit("0.1.0.1", "0.2.0.4", "out");
		  cbgp.bgpFilterMatchPrefixIn("0/0");
		  cbgp.bgpFilterAction(0x05, "3");
		  //cbgp.bgpFilterAction(0x01, "");
		  cbgp.bgpFilterFinalize();*/
		cbgp.bgpRouterPeerUp("0.1.0.1", "0.2.0.1", true);
		cbgp.bgpRouterPeerUp("0.1.0.1", "0.2.0.2", true);
		cbgp.bgpRouterPeerUp("0.1.0.1", "0.2.0.4", true);
								    
		//BGP in AS2
		cbgp.bgpAddRouter("Router1_AS2", "0.2.0.1", 2);
		cbgp.bgpRouterAddPeer("0.2.0.1", "0.1.0.1", 1);
		cbgp.bgpRouterAddPeer("0.2.0.1", "0.2.0.2", 2);
		cbgp.bgpRouterAddPeer("0.2.0.1", "0.2.0.3", 2);
		cbgp.bgpRouterAddPeer("0.2.0.1", "0.2.0.4", 2);
		cbgp.bgpRouterPeerNextHopSelf("0.2.0.1", "0.1.0.1");
		cbgp.bgpRouterPeerUp("0.2.0.1", "0.1.0.1", true);
		cbgp.bgpRouterPeerUp("0.2.0.1", "0.2.0.2", true);
		cbgp.bgpRouterPeerUp("0.2.0.1", "0.2.0.3", true);
		cbgp.bgpRouterPeerUp("0.2.0.1", "0.2.0.4", true);

		cbgp.bgpAddRouter("Router2_AS2", "0.2.0.2", 2);
		cbgp.bgpRouterAddPeer("0.2.0.2", "0.1.0.1", 1);
		cbgp.bgpRouterAddPeer("0.2.0.2", "0.2.0.1", 2);
		cbgp.bgpRouterAddPeer("0.2.0.2", "0.2.0.3", 2);
		cbgp.bgpRouterAddPeer("0.2.0.2", "0.2.0.4", 2);
		cbgp.bgpRouterPeerNextHopSelf("0.2.0.2", "0.1.0.1");
		cbgp.bgpRouterPeerUp("0.2.0.2", "0.1.0.1", true);
		cbgp.bgpRouterPeerUp("0.2.0.2", "0.2.0.1", true);
		cbgp.bgpRouterPeerUp("0.2.0.2", "0.2.0.3", true);
		cbgp.bgpRouterPeerUp("0.2.0.2", "0.2.0.4", true);

		cbgp.bgpAddRouter("Router3_AS2", "0.2.0.3", 2);
		cbgp.bgpRouterAddPeer("0.2.0.3", "0.2.0.1", 2);
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
		cbgp.bgpRouterAddPeer("0.2.0.3", "0.2.0.2", 2);
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
		cbgp.bgpRouterAddPeer("0.2.0.3", "0.2.0.4", 2);
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
		cbgp.bgpRouterPeerUp("0.2.0.3", "0.2.0.1", true);
		cbgp.bgpRouterPeerUp("0.2.0.3", "0.2.0.2", true);
		cbgp.bgpRouterPeerUp("0.2.0.3", "0.2.0.4", true);

		cbgp.bgpAddRouter("Router4_AS2", "0.2.0.4", 2);
		cbgp.bgpRouterAddPeer("0.2.0.4", "0.1.0.1", 1);
		cbgp.bgpRouterAddPeer("0.2.0.4", "0.2.0.1", 2);
		cbgp.bgpRouterAddPeer("0.2.0.4", "0.2.0.2", 2);
		cbgp.bgpRouterAddPeer("0.2.0.4", "0.2.0.3", 2);
		cbgp.bgpRouterPeerNextHopSelf("0.2.0.4", "0.1.0.1");
		cbgp.bgpRouterPeerUp("0.2.0.4", "0.1.0.1", true);
		cbgp.bgpRouterPeerUp("0.2.0.4", "0.2.0.1", true);
		cbgp.bgpRouterPeerUp("0.2.0.4", "0.2.0.2", true);
		cbgp.bgpRouterPeerUp("0.2.0.4", "0.2.0.3", true);

		cbgp.simRun();

		//Section added to demonstrate the possibility to change the 
		//IGP weight
		System.out.println("Links of 0.2.0.3 before weight change:");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 before weight change:");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 before weight change;");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		IPTrace trace= cbgp.netNodeRecordRoute("0.2.0.3", "0.1.0.1");
		System.out.println(trace);

		System.out.println("\n<Weight change : 0.2.0.3 <-> 0.2.0.1 from 20 to 5>\n");
		cbgp.netLinkWeight("0.2.0.3", "0.2.0.1", 5);

		//Update intradomain routes (recompute MST)
		cbgp.netDomainCompute(2);

		//Scan BGP routes that could have changed
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();
		//End of the added section 
 
		System.out.println("Links of 0.2.0.3 before first link down:");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 before first link down:");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 before first link down:");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
		System.out.println("\n\033[31;1m<<First link down: 0.2.0.1 <-> 0.2.0.3>>\033[0m\n");

		cbgp.netLinkUp("0.2.0.1", "0.2.0.3", false);

		//Update intradomain routes (recompute MST)
		cbgp.netDomainCompute(2);

		//Scan BGP routes that could change
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();
   

		System.out.println("Links of 0.2.0.3 after first link down:");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 after first link down:");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 after first link down:");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		System.out.println("\n\033[31;1m<<Second link down: 0.2.0.2 <-> 0.2.0.3>>\033[0m\n");

		cbgp.netLinkUp("0.2.0.2", "0.2.0.3", false);

		//Update intradomain routes (recompute MST)
		cbgp.netDomainCompute(2);

		//Scan BGP routes that could change
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();

		System.out.println("Links of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 after second link down:");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 after second link down:");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		System.out.println("\n\033[31;1m<<Third link down: 0.2.0.3 <-> 0.2.0.4>>\033[0m\n");

		cbgp.netLinkUp("0.2.0.4", "0.2.0.3", false);

		//Update intradomain routes (recompute MST)
		cbgp.netDomainCompute(2);

		//Scan BGP routes that could change
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();

		System.out.println("Links of 0.2.0.3 after third link down:");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 after third link down:");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 after third link down:");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
		System.out.println("RIB of 0.2.0.1 after third link down:");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.1", null), true);
		System.out.println("RIB In of 0.2.0.1 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.1", null, null, true), true);
 
		System.out.println("RIB of 0.2.0.2 after third link down:");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.2", null), true);
		System.out.println("RIB In of 0.2.0.2 after second link down:");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.2", null, null, true), true);

		for (Enumeration enumPeers= cbgp.bgpRouterGetPeers("0.2.0.2").elements();
		     enumPeers.hasMoreElements(); ) {
		    System.out.println(enumPeers.nextElement());
		}
 
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
	    System.exit(-1);
	}
    }

}

    
