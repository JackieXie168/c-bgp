// ==================================================================
// @(#)CBGP.java
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2003
// @lastdate 07/12/2004
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ CBGP ]-----------------------------------------------------
/**
 * This is the Java Native Interface to the C-BGP simulator. This
 * class is only a wrapper to C functions contained in the csim
 * library (libcsim.so).
 */
public class CBGP
{
  public native void init(String file_log);

  public native void finalize();
  
  public native int nodeAdd(String net_addr);
  
  public native int nodeLinkAdd(String net_addr_src, String net_addr_dst, 
				int Weight);

  public native int nodeLinkWeight(String net_addr1, String net_addr2, 
				    int Weight);

  public native int nodeLinkUp(String net_addr1, String net_addr2);

  public native int nodeLinkDown(String net_addr1, String net_addr2);

  public native int nodeRouteAdd(String net_addr, String prefix, 
				  String net_addr_next_hop, 
				  int Weight);

  public native int nodeSpfPrefix(String net_addr, String prefix);

  public native int nodeInterfaceAdd(String net_addr_id, String net_addr_int, 
				      String mask);

  public native void nodeShowLinks(String net_addr);

  public native String nodeShowRT(String net_addr, String prefix);

  public native int bgpRouterAdd(String name, String net_addr_router, 
				  int ASid);
  
  public native int bgpRouterNetworkAdd(String net_addr_router, 
					String prefix_network);

  public native int bgpRouterNeighborAdd(String net_addr_router, 
					  String net_addr_neighbor, 
					  int ASIdPeer);

  public native void bgpRouterNeighborNextHopSelf(String net_addr_router, 
						  String net_addr_neighbor);

  public native int bgpRouterNeighborUp(String net_addr_router, 
					String net_addr_neighbor);
  
  public native int bgpRouterRescan(String net_addr);

  public native String bgpRouterShowRib(String net_addr_router);
  
  public native void bgpRouterShowRibIn(String net_addr_router);

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

  public native int simRun();
  
  public native void simPrint(String line); 

  public native void runCmd(String line);

  public static void main(String[] args){
      CBGP cbgp= new CBGP();

      cbgp.init("/tmp/log_cbgp_jni");

      //this example has been translated from 
      //http://cbgp.info.ucl.ac.be/downloads/valid-igp-link-up-down.cli
      cbgp.simPrint("*** valid-igp-link-up-down ***\n\n");

    //Domain AS1
    cbgp.nodeAdd("0.1.0.1");
   
    //Domain AS2
    cbgp.nodeAdd("0.2.0.1");
    cbgp.nodeAdd("0.2.0.2");
    cbgp.nodeAdd("0.2.0.3");
    cbgp.nodeAdd("0.2.0.4");

    cbgp.nodeLinkAdd("0.2.0.1", "0.2.0.3", 20); // -> it was initially 5 but 
						//we also demonstrate the igp 
						//change weight in this example
    cbgp.nodeLinkAdd("0.2.0.2", "0.2.0.3", 10);
    cbgp.nodeLinkAdd("0.2.0.4", "0.2.0.3", 10);

    cbgp.nodeSpfPrefix("0.2.0.1", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.2", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.3", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Interdomain links and routes
    cbgp.nodeLinkAdd("0.1.0.1", "0.2.0.1", 0);
    cbgp.nodeLinkAdd("0.1.0.1", "0.2.0.2", 0);
    cbgp.nodeLinkAdd("0.1.0.1", "0.2.0.4", 0);
    cbgp.nodeRouteAdd("0.1.0.1", "0.2.0.1/32", "0.2.0.1", 0);
    cbgp.nodeRouteAdd("0.1.0.1", "0.2.0.2/32", "0.2.0.2", 0);
    cbgp.nodeRouteAdd("0.1.0.1", "0.2.0.4/32", "0.2.0.4", 0);
    cbgp.nodeRouteAdd("0.2.0.1", "0.1.0.1/32", "0.1.0.1", 0);
    cbgp.nodeRouteAdd("0.2.0.2", "0.1.0.1/32", "0.1.0.1", 0);
    cbgp.nodeRouteAdd("0.2.0.4", "0.1.0.1/32", "0.1.0.1", 0);

    cbgp.bgpRouterAdd("Router1_AS1", "0.1.0.1", 1);
    cbgp.bgpRouterNetworkAdd("0.1.0.1", "0.1/16");
   cbgp.bgpRouterNeighborAdd("0.1.0.1", "0.2.0.1", 2);
       cbgp.bgpFilterInit("0.1.0.1", "0.2.0.1", "out");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x05, "3");
//	cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
    cbgp.bgpRouterNeighborAdd("0.1.0.1", "0.2.0.2", 2);
       cbgp.bgpFilterInit("0.1.0.1", "0.2.0.2", "out");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x05, "3");
	//cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
    cbgp.bgpRouterNeighborAdd("0.1.0.1", "0.2.0.4", 2);
       cbgp.bgpFilterInit("0.1.0.1", "0.2.0.4", "out");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x05, "3");
	//cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
    cbgp.bgpRouterNeighborUp("0.1.0.1", "0.2.0.1");
    cbgp.bgpRouterNeighborUp("0.1.0.1", "0.2.0.2");
    cbgp.bgpRouterNeighborUp("0.1.0.1", "0.2.0.4");
								    
    //BGP in AS2
    cbgp.bgpRouterAdd("Router1_AS2", "0.2.0.1", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.1", "0.1.0.1", 1);
    cbgp.bgpRouterNeighborAdd("0.2.0.1", "0.2.0.2", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.1", "0.2.0.3", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.1", "0.2.0.4", 2);
    cbgp.bgpRouterNeighborNextHopSelf("0.2.0.1", "0.1.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.1", "0.1.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.1", "0.2.0.2");
    cbgp.bgpRouterNeighborUp("0.2.0.1", "0.2.0.3");
    cbgp.bgpRouterNeighborUp("0.2.0.1", "0.2.0.4");

    cbgp.bgpRouterAdd("Router2_AS2", "0.2.0.2", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.2", "0.1.0.1", 1);
    cbgp.bgpRouterNeighborAdd("0.2.0.2", "0.2.0.1", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.2", "0.2.0.3", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.2", "0.2.0.4", 2);
    cbgp.bgpRouterNeighborNextHopSelf("0.2.0.2", "0.1.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.2", "0.1.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.2", "0.2.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.2", "0.2.0.3");
    cbgp.bgpRouterNeighborUp("0.2.0.2", "0.2.0.4");

    cbgp.bgpRouterAdd("Router3_AS2", "0.2.0.3", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.3", "0.2.0.1", 2);
      cbgp.bgpFilterInit("0.2.0.3", "0.2.0.1", "in");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x03, "1000");
	cbgp.bgpFilterAction(0x01, "");
       cbgp.bgpFilterInit("0.2.0.3", "0.2.0.1", "out");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x03, "1000");
	cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
     cbgp.bgpFilterFinalize();
    cbgp.bgpRouterNeighborAdd("0.2.0.3", "0.2.0.2", 2);
      cbgp.bgpFilterInit("0.2.0.3", "0.2.0.2", "in");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x03, "1000");
	cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
      cbgp.bgpFilterInit("0.2.0.3", "0.2.0.2", "out");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x03, "1000");
	cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
    cbgp.bgpRouterNeighborAdd("0.2.0.3", "0.2.0.4", 2);
      cbgp.bgpFilterInit("0.2.0.3", "0.2.0.4", "in");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x03, "1000");
	cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
      cbgp.bgpFilterInit("0.2.0.3", "0.2.0.4", "out");
	cbgp.bgpFilterMatchPrefixIn("0/0");
	cbgp.bgpFilterAction(0x03, "1000");
	cbgp.bgpFilterAction(0x01, "");
      cbgp.bgpFilterFinalize();
    cbgp.bgpRouterNeighborUp("0.2.0.3", "0.2.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.3", "0.2.0.2");
    cbgp.bgpRouterNeighborUp("0.2.0.3", "0.2.0.4");

    cbgp.bgpRouterAdd("Router4_AS2", "0.2.0.4", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.4", "0.1.0.1", 1);
    cbgp.bgpRouterNeighborAdd("0.2.0.4", "0.2.0.1", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.4", "0.2.0.2", 2);
    cbgp.bgpRouterNeighborAdd("0.2.0.4", "0.2.0.3", 2);
    cbgp.bgpRouterNeighborNextHopSelf("0.2.0.4", "0.1.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.4", "0.1.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.4", "0.2.0.1");
    cbgp.bgpRouterNeighborUp("0.2.0.4", "0.2.0.2");
    cbgp.bgpRouterNeighborUp("0.2.0.4", "0.2.0.3");

    cbgp.simRun();

    String Rib;
    String RT;
    //Section added to demonstrate the possibility to change the 
    //IGP weight
    cbgp.simPrint("Links of 0.2.0.3 before weight change:\n");
    cbgp.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    cbgp.nodeShowLinks("0.2.0.3");
    cbgp.simPrint("RT of 0.2.0.3 before weight change:\n");
    RT = cbgp.nodeShowRT("0.2.0.3", "");
    cbgp.simPrint(RT);
    cbgp.simPrint("RIB of 0.2.0.3 before weight change;\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.3");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.3");

    cbgp.simPrint("\n<Weight change : 0.2.0.3 <-> 0.2.0.1 from 20 to 5>\n\n");

    cbgp.nodeLinkWeight("0.2.0.3", "0.2.0.1", 5);

    //Update intradomain routes (recompute MST)
    cbgp.nodeSpfPrefix("0.2.0.1", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.2", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.3", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    cbgp.bgpRouterRescan("0.2.0.3");

    cbgp.simRun();
    //End of the added section 
 
    cbgp.simPrint("Links of 0.2.0.3 before first link down:\n");
    cbgp.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    cbgp.nodeShowLinks("0.2.0.3");
    cbgp.simPrint("RT of 0.2.0.3 before first link down:\n");
    RT = cbgp.nodeShowRT("0.2.0.3", "");
    cbgp.simPrint(RT);
    cbgp.simPrint("RIB of 0.2.0.3 before first link down:\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.3");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.3");
    
    cbgp.simPrint("\n<<First link down: 0.2.0.1 <-> 0.2.0.3>>\n\n");

    cbgp.nodeLinkDown("0.2.0.1", "0.2.0.3");

    //Update intradomain routes (recompute MST)
    cbgp.nodeSpfPrefix("0.2.0.1", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.2", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.3", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    cbgp.bgpRouterRescan("0.2.0.3");

    cbgp.simRun();
   

    cbgp.simPrint("Links of 0.2.0.3 after first link down:\n");
    cbgp.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    cbgp.nodeShowLinks("0.2.0.3");
    cbgp.simPrint("RT of 0.2.0.3 after first link down:\n");
    RT = cbgp.nodeShowRT("0.2.0.3", "");
    cbgp.simPrint(RT);
    cbgp.simPrint("RIB of 0.2.0.3 after first link down:\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.3");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.3");

    cbgp.simPrint("\n<<Second link down: 0.2.0.2 <-> 0.2.0.3>>\n\n");

    cbgp.nodeLinkDown("0.2.0.2", "0.2.0.3");

    //Update intradomain routes (recompute MST)
    cbgp.nodeSpfPrefix("0.2.0.1", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.2", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.3", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    cbgp.bgpRouterRescan("0.2.0.3");

    cbgp.simRun();

    cbgp.simPrint("Links of 0.2.0.3 after second link down:\n");
    cbgp.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    cbgp.nodeShowLinks("0.2.0.3");
    cbgp.simPrint("RT of 0.2.0.3 after second link down:\n");
    RT = cbgp.nodeShowRT("0.2.0.3", "");
    cbgp.simPrint(RT);
    cbgp.simPrint("RIB of 0.2.0.3 after second link down:\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.3");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.3");

    cbgp.simPrint("\n<<Third link down: 0.2.0.3 <-> 0.2.0.4>>\n\n");

    cbgp.nodeLinkDown("0.2.0.4", "0.2.0.3");

    //Update intradomain routes (recompute MST)
    cbgp.nodeSpfPrefix("0.2.0.1", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.2", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.3", "0.2/16");
    cbgp.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    cbgp.bgpRouterRescan("0.2.0.3");

    cbgp.simRun();

    cbgp.simPrint("Links of 0.2.0.3 after third link down:\n");
    cbgp.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    cbgp.nodeShowLinks("0.2.0.3");
    cbgp.simPrint("RT of 0.2.0.3 after third link down:\n");
    RT = cbgp.nodeShowRT("0.2.0.3", "");
    cbgp.simPrint(RT);
    cbgp.simPrint("RIB of 0.2.0.3 after third link down:\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.3");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.3");
    
    cbgp.simPrint("RIB of 0.2.0.1 after third link down:\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.1");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.1 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.1");
 
    cbgp.simPrint("RIB of 0.2.0.2 after third link down:\n");
    cbgp.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = cbgp.bgpRouterShowRib("0.2.0.2");
    cbgp.simPrint(Rib);
    cbgp.simPrint("RIB In of 0.2.0.2 after second link down:\n");
    //For the moment on stdout
    cbgp.bgpRouterShowRibIn("0.2.0.2");
 
    cbgp.simPrint("\nDone.\n");

  cbgp.runCmd("bgp router 0.1.0.1 peer 0.2.0.2 filter out show");
    cbgp.finalize();
  }

  static {
    System.loadLibrary("csim");
  }
}

