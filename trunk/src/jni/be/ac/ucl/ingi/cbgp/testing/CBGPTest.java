// ==================================================================
// @(#)CBGPTest.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 20/06/2007
// ==================================================================
package be.ac.ucl.ingi.cbgp.testing;

import java.util.Enumeration;
import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPTrace;
import be.ac.ucl.ingi.cbgp.IPTraceElement;
import be.ac.ucl.ingi.cbgp.Route;
import be.ac.ucl.ingi.cbgp.bgp.Peer;
import be.ac.ucl.ingi.cbgp.bgp.Router;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPScriptException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Interface;
import be.ac.ucl.ingi.cbgp.net.Node;

// -----[ CBGPTest ]-------------------------------------------------
public class CBGPTest
{

    // -----[ main ]-------------------------------------------------
    @SuppressWarnings("unchecked")
	public static void main(String[] args){
	CBGP cbgp= new CBGP();
	
	try {
        cbgp.init("/tmp/cbgp_jni.log");
        cbgp.consoleSetLevel(CBGP.LOG_LEVEL_EVERYTHING);

        /* Simon Balon's experiment: problem with JNI interface (iBGP route-reflection
         * seems not work). Same experiment designed using the CLI works perfectly... */
        IGPDomain domain1 = cbgp.netAddDomain(1);
        IGPDomain domain2 = cbgp.netAddDomain(2);

        domain1.addNode("1.0.0.1");
        Node node12= domain1.addNode("1.0.0.2");
        domain1.addNode("1.0.0.3");

        Node node21= domain2.addNode("2.0.0.1");
        /*Node node22= */domain2.addNode("2.0.0.2");

        cbgp.netAddLink("1.0.0.1", "1.0.0.3", 1);
        cbgp.netAddLink("1.0.0.1", "1.0.0.2", 1);

        cbgp.netAddLink("2.0.0.1", "1.0.0.2", 1);
        cbgp.netAddLink("2.0.0.1", "2.0.0.2", 1).setWeight(1);

        Vector<be.ac.ucl.ingi.cbgp.net.Node> nodes = domain1.getNodes();
        for (be.ac.ucl.ingi.cbgp.net.Node currentNode : nodes) {
            Vector<be.ac.ucl.ingi.cbgp.net.Interface> links = currentNode.getLinks();
            for (be.ac.ucl.ingi.cbgp.net.Interface currentLink : links) {
                currentLink.setWeight(1);
            }
        }

        node12.addRoute("2.0.0.1/32", "2.0.0.1", 0);
        node21.addRoute("1.0.0.2/32", "1.0.0.2", 0);

        domain1.compute();
        domain2.compute();

        Router router11 = cbgp.bgpAddRouter("1.0.0.1", "1.0.0.1", 1);
        Router router12 = cbgp.bgpAddRouter("1.0.0.2", "1.0.0.2", 1);
        Router router13 = cbgp.bgpAddRouter("1.0.0.3", "1.0.0.3", 1);

        Router router21 = cbgp.bgpAddRouter("2.0.0.1", "2.0.0.1", 2);

        Peer peer1112 = router11.addPeer("1.0.0.2", 1);
        peer1112.setReflectorClient();
        peer1112.openSession();
        Peer peer1113 = router11.addPeer("1.0.0.3", 1);
        peer1113.setReflectorClient();
        peer1113.openSession();

        Peer peer1211 = router12.addPeer("1.0.0.1", 1);
        peer1211.openSession();

        Peer peer1311 = router13.addPeer("1.0.0.1", 1);
        peer1311.openSession();

        Peer peer1221 = router12.addPeer("2.0.0.1", 2);
        peer1221.setNextHopSelf(true);
        peer1221.openSession();

        Peer peer2112 = router21.addPeer("1.0.0.2", 1);
        peer2112.openSession();
        Peer peer2122 = router21.addPeer("2.0.0.2", 2);
        peer2122.setVirtual();
        peer2122.openSession();
        
        cbgp.simRun();
        
        cbgp.runCmd("bgp router 1.0.0.1 show info");
        cbgp.runCmd("bgp router 1.0.0.3 show info");
        cbgp.runCmd("net node 2.0.0.1 show rt *");
        cbgp.runCmd("net node 2.0.0.1 show links");
        cbgp.runCmd("bgp router 2.0.0.1 show peers");
        
        router21.addNetwork("2.0/16");
        
        peer2122.recv("BGP4|1104543314|A|2.0.0.1|2|12.0.48.0/20|10578 1742|IGP|2.0.0.2|200|0|10578:800 10578:840 11537:950|NAG||");

        cbgp.simRun();

        System.out.println("Peers R11: "+router11.getPeers());
        System.out.println("Peers R12: "+router12.getPeers());
        System.out.println("Peers R13: "+router13.getPeers());
        System.out.println("Peers R21: "+router21.getPeers());
        
        System.out.println("");
        Vector<be.ac.ucl.ingi.cbgp.bgp.Route> routes = router11.getAdjRIB("1.0.0.2", "2.0/16", true);
        System.out.println("adj rib in of 1.0.0.1 from 1.0.0.2 for prefix 2.0/16 :");
        for (Route currentRoute : routes) {
            System.out.println(currentRoute.toString());
        }

        System.out.println("");
        routes = router11.getAdjRIB("1.0.0.3", "2.0/16", false);
        System.out.println("adj rib out of 1.0.0.1 to 1.0.0.3 for prefix 2.0/16 :");
        for (Route currentRoute : routes) {
            System.out.println(currentRoute.toString());
        }
        System.out.println("");

        System.out.println("RIB of 1.0.0.3 for prefix 2.0/16");
        System.out.println(router13.getAdjRIB("1.0.0.1", "2.0/16", true));
        System.out.println("");

    } catch (Exception e) {
        System.out.println("Exception in test ");
         e.printStackTrace();
    }
    
    System.exit(0);
	
	try {
		cbgp.init("/tmp/log_cbgp_jni");
		cbgp.runScript("/Users/bqu/Desktop/EMANICS_Summer_School_2007/exercises/abilene.cli");
	    Node node= cbgp.netGetNode("198.32.12.41");
	    IPTrace trace= node.traceRoute("198.32.12.121");
	    System.out.println(trace);
	    cbgp.destroy();
	} catch (CBGPScriptException e) {
		e.printStackTrace();
		System.exit(-1);
	} catch (CBGPException e) {
	    e.printStackTrace();
	    System.exit(-1);
	}
	System.exit(0);
	
	if (args.length > 0) {

	    showRoutes(cbgp.loadMRT(args[0]), true);

	} else {

	    try {

		// This example has been translated from a CLI script
		// valid-igp-link-up-down.cli

		System.out.println("Initializing C-BGP...");
		cbgp.init("/tmp/log_cbgp_jni");
	    
		showTitle("valid-igp-link-up-down");
		
		//Domain AS1
		System.out.println("Create domain 1");
		IGPDomain domain1= cbgp.netAddDomain(1);
		Node node11= domain1.addNode("0.1.0.1");
		showNodes(domain1.getNodes());
		
		//Domain AS2
		System.out.println("Create domain 2");
		IGPDomain domain2= cbgp.netAddDomain(2);
		Node node21= domain2.addNode("0.2.0.1");
		Node node22= domain2.addNode("0.2.0.2");
		Node node23= domain2.addNode("0.2.0.3");
		Node node24= domain2.addNode("0.2.0.4");
		Interface link21_23= cbgp.netAddLink("0.2.0.1", "0.2.0.3", 20);
		cbgp.netAddLink("0.2.0.2", "0.2.0.3", 10);
		Interface link23_24= cbgp.netAddLink("0.2.0.3", "0.2.0.4", 10);
		showNodes(domain2.getNodes());

		// Intradomain routes
		System.out.println("Compute IGP routes...");
		domain1.compute();
		domain2.compute();

		//Interdomain links and routes
		cbgp.netAddLink("0.1.0.1", "0.2.0.1", 0);
		cbgp.netAddLink("0.1.0.1", "0.2.0.2", 0);
		cbgp.netAddLink("0.1.0.1", "0.2.0.4", 0);
		node11.addRoute("0.2.0.1/32", "0.2.0.1", 0);
		node11.addRoute("0.2.0.2/32", "0.2.0.2", 0);
		node11.addRoute("0.2.0.4/32", "0.2.0.4", 0);
		node21.addRoute("0.1.0.1/32", "0.1.0.1", 0);
		node22.addRoute("0.1.0.1/32", "0.1.0.1", 0);
		node24.addRoute("0.1.0.1/32", "0.1.0.1", 0);

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

    // -----[ showTitle ]--------------------------------------------
    private static void showTitle(String msg)
    {
	System.out.println("*** \033[34;1m"+msg+"\033[0m***");
    }

    // -----[ showHL ]-----------------------------------------------
    private static void showHL(String msg)
    {
	System.out.println("\033[1m"+msg+"\033[0m");
    }

    // -----[ showSep ]----------------------------------------------
    private static void showSep()
    {
	System.out.println("========================================"+
			   "=======================================");
    }

    // -----[ showLinks ]--------------------------------------------
    private static void showLinks(Vector links)
    {
	showSep();
	showHL("Addr\tDelay\tWeight\tStatus");
	if (links != null) {
	    for (Enumeration linksEnum= links.elements();
		 linksEnum.hasMoreElements();) {
		Interface link= (Interface) linksEnum.nextElement();
		System.out.println(link);
	    }
	} else {
	    System.out.println("(null)");
	}
	showSep();
    }

    // -----[ showRoutes ]-------------------------------------------
    private static void showRoutes(Vector routes, boolean bBGP)
    {
	showSep();
	if (bBGP) {
	    showHL("   Prefix\tNxt-hop\tLoc-prf\tMED\tPath\tOrigin");
	} else {
	    showHL("   Prefix\tNxt-hop");
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
	showSep();
    }

    // -----[ showNodes ]--------------------------------------------
    private static void showNodes(Vector nodes)
    {
	showSep();
	showHL("IP addr");
	if (nodes != null) {
	    for (Enumeration nodesEnum= nodes.elements();
		 nodesEnum.hasMoreElements();) {
		Node node= (Node) nodesEnum.nextElement();
		System.out.println(node);
	    }
	} else {
	    System.out.println("(null)");
	}
	showSep();
    }

}

