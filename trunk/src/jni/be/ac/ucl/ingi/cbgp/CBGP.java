// ==================================================================
// @(#)CBGP.java
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 18/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

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
  
    public native int netAddNode(String sAddr);
  
    public native int netAddLink(String sSrcAddr, String sDstAddr, int iWeight);

    public native int netLinkWeight(String sSrcAddr, String sDstAddr, int iWeight);

    public native int netLinkUp(String sSrcAddr, String sDstAddr, boolean bUp);

    public native int netNodeSpfPrefix(String sAddr, String sPrefix);

    public native int netNodeRouteAdd(String sRouterAddr, String sPrefix, 
				      String sNexthop, 
				      int iWeight);

    public native Vector netNodeGetLinks(String sAddr);

    public native Vector netNodeGetRT(String sNodeAddr, String sPrefix);

    public native IPTrace netNodeRecordRoute(String sNodeAddr,
					     String sDstAddr);

    public native int bgpAddRouter(String sName, String sAddr, int iASNumber);
  
    public native int bgpRouterAddNetwork(String sRouterAddr, 
					  String sPrefix);

    public native int bgpRouterAddPeer(String sRouterAddr, 
				       String sPeerAddr, 
				       int iASNumber);

    public native int bgpRouterPeerNextHopSelf(String sRouterAddr,
						   String sPeerAddr);

    public native int bgpRouterPeerVirtual(String sRouterAddr,
					   String sPeerAddr);

    public native int bgpRouterPeerUp(String sRouterAddr, 
					  String sRouterPeer,
					  boolean bUp);

    public native int bgpRouterPeerRecv(String sRouterAddr, 
					String sRouterPeer,
					String sMesg);
  
    public native int bgpRouterRescan(String net_addr);

    public native Vector bgpRouterGetPeers(String sRouterAddr);

    public native Vector bgpRouterGetRib(String sRouterAddr, String sPrefix);
  
    public native Vector bgpRouterGetAdjRib(String sRouterAddr,
					    String sNeighborAddr,
					    String sPrefix,
					    boolean bIn);

    public native int bgpRouterLoadRib(String sRouterAddr, String sFileName);

    public native int bgpDomainRescan(int iASNumber);

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

    public native int simRun();
  
    public native void print(String line); 

    public native int runCmd(String line);

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

    public static void main(String[] args){
	CBGP cbgp= new CBGP();

	cbgp.init("/tmp/log_cbgp_jni");

	//this example has been translated from 
	//http://cbgp.info.ucl.ac.be/downloads/valid-igp-link-up-down.cli
	cbgp.print("*** valid-igp-link-up-down ***\n\n");

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
	cbgp.print("Links of 0.2.0.3 before weight change:\n");
	//For the moment on stdout
	showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
	cbgp.print("RT of 0.2.0.3 before weight change:\n");
	showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
	cbgp.print("RIB of 0.2.0.3 before weight change;\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
	cbgp.print("RIB In of 0.2.0.3 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

	IPTrace trace= cbgp.netNodeRecordRoute("0.2.0.3", "0.1.0.1");
	System.out.println(trace);

	cbgp.print("\n<Weight change : 0.2.0.3 <-> 0.2.0.1 from 20 to 5>\n\n");
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
 
	cbgp.print("Links of 0.2.0.3 before first link down:\n");
	//For the moment on stdout
	showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
	cbgp.print("RT of 0.2.0.3 before first link down:\n");
	showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
	cbgp.print("RIB of 0.2.0.3 before first link down:\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
	cbgp.print("RIB In of 0.2.0.3 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
	cbgp.print("\n<<First link down: 0.2.0.1 <-> 0.2.0.3>>\n\n");

	cbgp.netLinkUp("0.2.0.1", "0.2.0.3", false);

	//Update intradomain routes (recompute MST)
	cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

	//Scan BGP routes that could change
	cbgp.bgpDomainRescan(2);

	cbgp.simRun();
   

	cbgp.print("Links of 0.2.0.3 after first link down:\n");
	//For the moment on stdout
	showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
	cbgp.print("RT of 0.2.0.3 after first link down:\n");
	showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
	cbgp.print("RIB of 0.2.0.3 after first link down:\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
	cbgp.print("RIB In of 0.2.0.3 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

	cbgp.print("\n<<Second link down: 0.2.0.2 <-> 0.2.0.3>>\n\n");

	cbgp.netLinkUp("0.2.0.2", "0.2.0.3", false);

	//Update intradomain routes (recompute MST)
	cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

	//Scan BGP routes that could change
	cbgp.bgpDomainRescan(2);

	cbgp.simRun();

	cbgp.print("Links of 0.2.0.3 after second link down:\n");
	//For the moment on stdout
	showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
	cbgp.print("RT of 0.2.0.3 after second link down:\n");
	showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
	cbgp.print("RIB of 0.2.0.3 after second link down:\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
	cbgp.print("RIB In of 0.2.0.3 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);

	cbgp.print("\n<<Third link down: 0.2.0.3 <-> 0.2.0.4>>\n\n");

	cbgp.netLinkUp("0.2.0.4", "0.2.0.3", false);

	//Update intradomain routes (recompute MST)
	cbgp.netNodeSpfPrefix("0.2.0.1", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.2", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.3", "0.2/16");
	cbgp.netNodeSpfPrefix("0.2.0.4", "0.2/16");

	//Scan BGP routes that could change
	cbgp.bgpDomainRescan(2);

	cbgp.simRun();

	cbgp.print("Links of 0.2.0.3 after third link down:\n");
	//For the moment on stdout
	showLinks(cbgp.netNodeGetLinks("0.2.0.3"));
	cbgp.print("RT of 0.2.0.3 after third link down:\n");
	showRoutes(cbgp.netNodeGetRT("0.2.0.3", null), false);
	cbgp.print("RIB of 0.2.0.3 after third link down:\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.3", null), true);
	cbgp.print("RIB In of 0.2.0.3 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.3", null, null, true), true);
    
	cbgp.print("RIB of 0.2.0.1 after third link down:\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.1", null), true);
	cbgp.print("RIB In of 0.2.0.1 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.1", null, null, true), true);
 
	cbgp.print("RIB of 0.2.0.2 after third link down:\n");
	showRoutes(cbgp.bgpRouterGetRib("0.2.0.2", null), true);
	cbgp.print("RIB In of 0.2.0.2 after second link down:\n");
	//For the moment on stdout
	showRoutes(cbgp.bgpRouterGetAdjRib("0.2.0.2", null, null, true), true);

	for (Enumeration enumPeers= cbgp.bgpRouterGetPeers("0.2.0.2").elements();
	     enumPeers.hasMoreElements(); ) {
	    System.out.println(enumPeers.nextElement());
	}
 
	cbgp.print("Done.\n");

	cbgp.destroy();
    }

    static {
	System.loadLibrary("csim");
    }
}

