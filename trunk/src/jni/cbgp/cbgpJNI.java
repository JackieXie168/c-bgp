// ==================================================================
// @(#)cbgpJNI.java
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2003
// @lastdate 30/10/2004
// ==================================================================

//package jni.cbgp; 

class cbgpJNI
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
    cbgpJNI CBGP = new cbgpJNI();

    CBGP.init("/home/standel/infonet/log_cbgp_jni");

    //this example has been translated from 
    //http://cbgp.info.ucl.ac.be/downloads/valid-igp-link-up-down.cli
    CBGP.simPrint("*** valid-igp-link-up-down ***\n\n");

    //Domain AS1
    CBGP.nodeAdd("0.1.0.1");
   
    //Domain AS2
    CBGP.nodeAdd("0.2.0.1");
    CBGP.nodeAdd("0.2.0.2");
    CBGP.nodeAdd("0.2.0.3");
    CBGP.nodeAdd("0.2.0.4");

    CBGP.nodeLinkAdd("0.2.0.1", "0.2.0.3", 20); // -> it was initially 5 but 
						//we also demonstrate the igp 
						//change weight in this example
    CBGP.nodeLinkAdd("0.2.0.2", "0.2.0.3", 10);
    CBGP.nodeLinkAdd("0.2.0.4", "0.2.0.3", 10);

    CBGP.nodeSpfPrefix("0.2.0.1", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.2", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.3", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Interdomain links and routes
    CBGP.nodeLinkAdd("0.1.0.1", "0.2.0.1", 0);
    CBGP.nodeLinkAdd("0.1.0.1", "0.2.0.2", 0);
    CBGP.nodeLinkAdd("0.1.0.1", "0.2.0.4", 0);
    CBGP.nodeRouteAdd("0.1.0.1", "0.2.0.1/32", "0.2.0.1", 0);
    CBGP.nodeRouteAdd("0.1.0.1", "0.2.0.2/32", "0.2.0.2", 0);
    CBGP.nodeRouteAdd("0.1.0.1", "0.2.0.4/32", "0.2.0.4", 0);
    CBGP.nodeRouteAdd("0.2.0.1", "0.1.0.1/32", "0.1.0.1", 0);
    CBGP.nodeRouteAdd("0.2.0.2", "0.1.0.1/32", "0.1.0.1", 0);
    CBGP.nodeRouteAdd("0.2.0.4", "0.1.0.1/32", "0.1.0.1", 0);

    CBGP.bgpRouterAdd("Router1_AS1", "0.1.0.1", 1);
    CBGP.bgpRouterNetworkAdd("0.1.0.1", "0.1/16");
   CBGP.bgpRouterNeighborAdd("0.1.0.1", "0.2.0.1", 2);
       CBGP.bgpFilterInit("0.1.0.1", "0.2.0.1", "out");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x05, "3");
//	CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
    CBGP.bgpRouterNeighborAdd("0.1.0.1", "0.2.0.2", 2);
       CBGP.bgpFilterInit("0.1.0.1", "0.2.0.2", "out");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x05, "3");
	//CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
    CBGP.bgpRouterNeighborAdd("0.1.0.1", "0.2.0.4", 2);
       CBGP.bgpFilterInit("0.1.0.1", "0.2.0.4", "out");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x05, "3");
	//CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
    CBGP.bgpRouterNeighborUp("0.1.0.1", "0.2.0.1");
    CBGP.bgpRouterNeighborUp("0.1.0.1", "0.2.0.2");
    CBGP.bgpRouterNeighborUp("0.1.0.1", "0.2.0.4");
								    
    //BGP in AS2
    CBGP.bgpRouterAdd("Router1_AS2", "0.2.0.1", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.1", "0.1.0.1", 1);
    CBGP.bgpRouterNeighborAdd("0.2.0.1", "0.2.0.2", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.1", "0.2.0.3", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.1", "0.2.0.4", 2);
    CBGP.bgpRouterNeighborNextHopSelf("0.2.0.1", "0.1.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.1", "0.1.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.1", "0.2.0.2");
    CBGP.bgpRouterNeighborUp("0.2.0.1", "0.2.0.3");
    CBGP.bgpRouterNeighborUp("0.2.0.1", "0.2.0.4");

    CBGP.bgpRouterAdd("Router2_AS2", "0.2.0.2", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.2", "0.1.0.1", 1);
    CBGP.bgpRouterNeighborAdd("0.2.0.2", "0.2.0.1", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.2", "0.2.0.3", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.2", "0.2.0.4", 2);
    CBGP.bgpRouterNeighborNextHopSelf("0.2.0.2", "0.1.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.2", "0.1.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.2", "0.2.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.2", "0.2.0.3");
    CBGP.bgpRouterNeighborUp("0.2.0.2", "0.2.0.4");

    CBGP.bgpRouterAdd("Router3_AS2", "0.2.0.3", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.3", "0.2.0.1", 2);
      CBGP.bgpFilterInit("0.2.0.3", "0.2.0.1", "in");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x03, "1000");
	CBGP.bgpFilterAction(0x01, "");
       CBGP.bgpFilterInit("0.2.0.3", "0.2.0.1", "out");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x03, "1000");
	CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
     CBGP.bgpFilterFinalize();
    CBGP.bgpRouterNeighborAdd("0.2.0.3", "0.2.0.2", 2);
      CBGP.bgpFilterInit("0.2.0.3", "0.2.0.2", "in");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x03, "1000");
	CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
      CBGP.bgpFilterInit("0.2.0.3", "0.2.0.2", "out");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x03, "1000");
	CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
    CBGP.bgpRouterNeighborAdd("0.2.0.3", "0.2.0.4", 2);
      CBGP.bgpFilterInit("0.2.0.3", "0.2.0.4", "in");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x03, "1000");
	CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
      CBGP.bgpFilterInit("0.2.0.3", "0.2.0.4", "out");
	CBGP.bgpFilterMatchPrefixIn("0/0");
	CBGP.bgpFilterAction(0x03, "1000");
	CBGP.bgpFilterAction(0x01, "");
      CBGP.bgpFilterFinalize();
    CBGP.bgpRouterNeighborUp("0.2.0.3", "0.2.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.3", "0.2.0.2");
    CBGP.bgpRouterNeighborUp("0.2.0.3", "0.2.0.4");

    CBGP.bgpRouterAdd("Router4_AS2", "0.2.0.4", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.4", "0.1.0.1", 1);
    CBGP.bgpRouterNeighborAdd("0.2.0.4", "0.2.0.1", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.4", "0.2.0.2", 2);
    CBGP.bgpRouterNeighborAdd("0.2.0.4", "0.2.0.3", 2);
    CBGP.bgpRouterNeighborNextHopSelf("0.2.0.4", "0.1.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.4", "0.1.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.4", "0.2.0.1");
    CBGP.bgpRouterNeighborUp("0.2.0.4", "0.2.0.2");
    CBGP.bgpRouterNeighborUp("0.2.0.4", "0.2.0.3");

    CBGP.simRun();

    String Rib;
    String RT;
    //Section added to demonstrate the possibility to change the 
    //IGP weight
    CBGP.simPrint("Links of 0.2.0.3 before weight change:\n");
    CBGP.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    CBGP.nodeShowLinks("0.2.0.3");
    CBGP.simPrint("RT of 0.2.0.3 before weight change:\n");
    RT = CBGP.nodeShowRT("0.2.0.3", "");
    CBGP.simPrint(RT);
    CBGP.simPrint("RIB of 0.2.0.3 before weight change;\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.3");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.3");

    CBGP.simPrint("\n<Weight change : 0.2.0.3 <-> 0.2.0.1 from 20 to 5>\n\n");

    CBGP.nodeLinkWeight("0.2.0.3", "0.2.0.1", 5);

    //Update intradomain routes (recompute MST)
    CBGP.nodeSpfPrefix("0.2.0.1", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.2", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.3", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    CBGP.bgpRouterRescan("0.2.0.3");

    CBGP.simRun();
    //End of the added section 
 
    CBGP.simPrint("Links of 0.2.0.3 before first link down:\n");
    CBGP.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    CBGP.nodeShowLinks("0.2.0.3");
    CBGP.simPrint("RT of 0.2.0.3 before first link down:\n");
    RT = CBGP.nodeShowRT("0.2.0.3", "");
    CBGP.simPrint(RT);
    CBGP.simPrint("RIB of 0.2.0.3 before first link down:\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.3");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.3");
    
    CBGP.simPrint("\n<<First link down: 0.2.0.1 <-> 0.2.0.3>>\n\n");

    CBGP.nodeLinkDown("0.2.0.1", "0.2.0.3");

    //Update intradomain routes (recompute MST)
    CBGP.nodeSpfPrefix("0.2.0.1", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.2", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.3", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    CBGP.bgpRouterRescan("0.2.0.3");

    CBGP.simRun();
   

    CBGP.simPrint("Links of 0.2.0.3 after first link down:\n");
    CBGP.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    CBGP.nodeShowLinks("0.2.0.3");
    CBGP.simPrint("RT of 0.2.0.3 after first link down:\n");
    RT = CBGP.nodeShowRT("0.2.0.3", "");
    CBGP.simPrint(RT);
    CBGP.simPrint("RIB of 0.2.0.3 after first link down:\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.3");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.3");

    CBGP.simPrint("\n<<Second link down: 0.2.0.2 <-> 0.2.0.3>>\n\n");

    CBGP.nodeLinkDown("0.2.0.2", "0.2.0.3");

    //Update intradomain routes (recompute MST)
    CBGP.nodeSpfPrefix("0.2.0.1", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.2", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.3", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    CBGP.bgpRouterRescan("0.2.0.3");

    CBGP.simRun();

    CBGP.simPrint("Links of 0.2.0.3 after second link down:\n");
    CBGP.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    CBGP.nodeShowLinks("0.2.0.3");
    CBGP.simPrint("RT of 0.2.0.3 after second link down:\n");
    RT = CBGP.nodeShowRT("0.2.0.3", "");
    CBGP.simPrint(RT);
    CBGP.simPrint("RIB of 0.2.0.3 after second link down:\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.3");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.3");

    CBGP.simPrint("\n<<Third link down: 0.2.0.3 <-> 0.2.0.4>>\n\n");

    CBGP.nodeLinkDown("0.2.0.4", "0.2.0.3");

    //Update intradomain routes (recompute MST)
    CBGP.nodeSpfPrefix("0.2.0.1", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.2", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.3", "0.2/16");
    CBGP.nodeSpfPrefix("0.2.0.4", "0.2/16");

    //Scan BGP routes that could change
    CBGP.bgpRouterRescan("0.2.0.3");

    CBGP.simRun();

    CBGP.simPrint("Links of 0.2.0.3 after third link down:\n");
    CBGP.simPrint("Addr\t\tDelay\tWeight\tStatus\n");
    //For the moment on stdout
    CBGP.nodeShowLinks("0.2.0.3");
    CBGP.simPrint("RT of 0.2.0.3 after third link down:\n");
    RT = CBGP.nodeShowRT("0.2.0.3", "");
    CBGP.simPrint(RT);
    CBGP.simPrint("RIB of 0.2.0.3 after third link down:\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.3");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.3 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.3");
    
    CBGP.simPrint("RIB of 0.2.0.1 after third link down:\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.1");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.1 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.1");
 
    CBGP.simPrint("RIB of 0.2.0.2 after third link down:\n");
    CBGP.simPrint("prefix\t\tnxt-hop\tloc-pref\tMED\tpath\torigin\n");
    Rib = CBGP.bgpRouterShowRib("0.2.0.2");
    CBGP.simPrint(Rib);
    CBGP.simPrint("RIB In of 0.2.0.2 after second link down:\n");
    //For the moment on stdout
    CBGP.bgpRouterShowRibIn("0.2.0.2");
 
    CBGP.simPrint("\nDone.\n");

  CBGP.runCmd("bgp router 0.1.0.1 peer 0.2.0.2 filter out show");
    CBGP.finalize();
  }

  static {
    System.loadLibrary("csim");
  }
}

