// ==================================================================
// @(#)CBGP.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 08/03/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Vector;

// -----[ CBGP ]-----------------------------------------------------
/**
 * This is the Java Native Interface to the C-BGP simulator. This
 * class is only a wrapper to C functions contained in the csim
 * library (libcsim.so).
 */
public class CBGP
{
    public native void init(String sFileLog);

    public native void destroy();
  
    public native void netAddNode(String sAddr)
	throws CBGPException;
  
    public native void netAddLink(String sSrcAddr, String sDstAddr, int iWeight) 
	throws CBGPException;

    public native void netLinkWeight(String sSrcAddr, String sDstAddr, int iWeight)
	throws CBGPException;

    public native void netLinkUp(String sSrcAddr, String sDstAddr, boolean bUp)
	throws CBGPException;

    public native void netNodeSpfPrefix(String sAddr, String sPrefix)
	throws CBGPException;

    public native void netNodeRouteAdd(String sRouterAddr, String sPrefix,
				       String sNexthop, 
				       int iWeight)
	throws CBGPException;

    public native Vector netNodeGetLinks(String sAddr)
	throws CBGPException;

    public native Vector netNodeGetRT(String sNodeAddr, String sPrefix)
	throws CBGPException;

    public native IPTrace netNodeRecordRoute(String sNodeAddr,
					     String sDstAddr)
	throws CBGPException;

    public native int bgpAddRouter(String sName, String sAddr, int iASNumber)
	throws CBGPException;
  
    public native void bgpRouterAddNetwork(String sRouterAddr, 
					   String sPrefix)
	throws CBGPException;

    public native void bgpRouterAddPeer(String sRouterAddr, 
					String sPeerAddr, 
					int iASNumber)
	throws CBGPException;

    public native void bgpRouterPeerNextHopSelf(String sRouterAddr,
					       String sPeerAddr)
	throws CBGPException;

    public native void bgpRouterPeerReflectorClient(String sRouterAddr,
						   String sPeerAddr)
	throws CBGPException;

    public native void bgpRouterPeerVirtual(String sRouterAddr,
					    String sPeerAddr)
	throws CBGPException;

    public native void bgpRouterPeerUp(String sRouterAddr,
				       String sRouterPeer,
				       boolean bUp)
	throws CBGPException;

    public native void bgpRouterPeerRecv(String sRouterAddr, 
					 String sRouterPeer,
					 String sMesg)
	throws CBGPException;
  
    public native void bgpRouterRescan(String net_addr)
	throws CBGPException;

    public native Vector bgpRouterGetPeers(String sRouterAddr)
	throws CBGPException;

    public native Vector bgpRouterGetRib(String sRouterAddr, String sPrefix)
	throws CBGPException;
  
    public native Vector bgpRouterGetAdjRib(String sRouterAddr,
					    String sNeighborAddr,
					    String sPrefix,
					    boolean bIn)
	throws CBGPException;

    public native void bgpRouterLoadRib(String sRouterAddr, String sFileName)
	throws CBGPException;

    public native void bgpDomainRescan(int iASNumber)
	throws CBGPException;

    public native ArrayList loadMRT(String sFileName);

    /* BQU: TO BE FIXED !!!

    public native int nodeInterfaceAdd(String net_addr_id, String net_addr_int, 
      String mask);

    //Type = [in|out]
    public native int bgpFilterInit(String net_addr_router, String net_addr_neighbor,
				    String type); 
      
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

    public native void simRun() throws CBGPException;
  
    public native void runCmd(String line) throws CBGPException;

    // -----[ showLinks ]--------------------------------------------
    private static void showLinks(Vector links)
    {
	System.out.println("========================================"+
			   "=======================================");
	System.out.println("Addr\tDelay\tWeight\tStatus");
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
	    System.out.println("   Prefix\tNxt-hop\tLoc-prf\tMED\tPath\tOrigin");
	} else {
	    System.out.println("   Prefix\tNxt-hop");
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
	    System.out.println("   Prefix\tNxt-hop\tLoc-prf\tMED\tPath\tOrigin");
	} else {
	    System.out.println("   Prefix\tNxt-hop");
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

    public static void main(String[] args){
	CBGP cbgp= new CBGP();

	if (args.length > 0) {

	    showRoutes(cbgp.loadMRT(args[0]), true);

	} else {

	    try {

		cbgp.init("/tmp/log_cbgp_jni");
	    
		//this example has been translated from 
		//http://cbgp.info.ucl.ac.be/downloads/valid-igp-link-up-down.cli
		System.out.println("*** \033[34;1mvalid-igp-link-up-down \033[0m***\n\n");
		
		//Domain AS1
		cbgp.netAddNode("0.1.0.1");
		
		//Domain AS2
		cbgp.netAddNode("0.2.0.1");
		cbgp.netAddNode("0.2.0.2");
		cbgp.netAddNode("0.2.0.3");
		cbgp.netAddNode("0.2.0.4");
		
		cbgp.netAddLink("0.2.0.1", "0.2.0.3", 20); // -> it was initially 5 but 
		//we also demonstrate the igp 
		//change weight in this example
		cbgp.netAddLink("0.2.0.2", "0.2.0.3", 10);
		cbgp.netAddLink("0.2.0.4", "0.2.0.3", 10);
	    
		cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

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
		System.out.println("Links of 0.2.0.3 before weight change:\n");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 before weight change:\n");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 before weight change;\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		IPTrace trace= cbgp.netNodeRecordRoute("0.2.0.3", "0.1.0.1");
		System.out.println(trace);

		System.out.println("\n<Weight change : 0.2.0.3 <-> 0.2.0.1 from 20 to 5>\n\n");
		cbgp.netLinkWeight("0.2.0.3", "0.2.0.1", 5);

		//Update intradomain routes (recompute MST)
		cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

		//Scan BGP routes that could have changed
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();
		//End of the added section 
 
		System.out.println("Links of 0.2.0.3 before first link down:\n");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 before first link down:\n");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 before first link down:\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
		System.out.println("\n\033[31;1m<<First link down: 0.2.0.1 <-> 0.2.0.3>>\033[0m\n\n");

		cbgp.netLinkUp("0.2.0.1", "0.2.0.3", false);

		//Update intradomain routes (recompute MST)
		cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

		//Scan BGP routes that could change
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();
   

		System.out.println("Links of 0.2.0.3 after first link down:\n");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 after first link down:\n");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 after first link down:\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		System.out.println("\n\033[31;1m<<Second link down: 0.2.0.2 <-> 0.2.0.3>>\033[0m\n\n");

		cbgp.netLinkUp("0.2.0.2", "0.2.0.3", false);

		//Update intradomain routes (recompute MST)
		cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

		//Scan BGP routes that could change
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();

		System.out.println("Links of 0.2.0.3 after second link down:\n");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 after second link down:\n");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 after second link down:\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

		System.out.println("\n\033[31;1m<<Third link down: 0.2.0.3 <-> 0.2.0.4>>\033[0m\n\n");

		cbgp.netLinkUp("0.2.0.4", "0.2.0.3", false);

		//Update intradomain routes (recompute MST)
		cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
		cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

		//Scan BGP routes that could change
		cbgp.bgpDomainRescan(2);

		cbgp.simRun();

		System.out.println("Links of 0.2.0.3 after third link down:\n");
		//For the moment on stdout
		showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
		System.out.println("RT of 0.2.0.3 after third link down:\n");
		showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
		System.out.println("RIB of 0.2.0.3 after third link down:\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
		System.out.println("RIB In of 0.2.0.3 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
		System.out.println("RIB of 0.2.0.1 after third link down:\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.1", null), true);
		System.out.println("RIB In of 0.2.0.1 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.1", null, null, true), true);
 
		System.out.println("RIB of 0.2.0.2 after third link down:\n");
		showRoutes(cbgp.bgpRouterGetRib("0.2.0.2", null), true);
		System.out.println("RIB In of 0.2.0.2 after second link down:\n");
		//For the moment on stdout
		showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.2", null, null, true), true);

		for (Enumeration enumPeers= cbgp.bgpRouterGetPeers("0.2.0.2").elements();
		     enumPeers.hasMoreElements(); ) {
		    System.out.println(enumPeers.nextElement());
		}
 
		System.out.println("\n\033[32;1mDone\033[0m.\n");

		cbgp.destroy();

	    } catch (CBGPException e) {
		System.out.println(e.getMessage());
		e.printStackTrace();
	    }

	}
    }
    
    static {
	try {
	    System.loadLibrary("pcre");
	    System.loadLibrary("gds");
	    System.loadLibrary("csim");
	} catch (UnsatisfiedLinkError e) {
	    System.err.println("Could not load one of the libraries "+
			       "required by CBGP (reason: "+
			       e.getMessage()+")");
	    System.exit(-1);
	}
    }

}

    