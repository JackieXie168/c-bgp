// ==================================================================
// @(#)CBGP.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 29/06/2007
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
 * This is C-BGP's Java Native Interface (JNI). This class is only a
 * wrapper to C functions contained in the csim library (libcsim).
 */
public class CBGP
{

    // -----[ Console log levels ]-----------------------------------
    public static final int LOG_LEVEL_EVERYTHING= 0;
    public static final int LOG_LEVEL_DEBUG     = 1;
    public static final int LOG_LEVEL_INFO      = 2;
    public static final int LOG_LEVEL_WARNING   = 3;
    public static final int LOG_LEVEL_SEVERE    = 4;
    public static final int LOG_LEVEL_FATAL     = 5;
    public static final int LOG_LEVEL_MAX       = 255;

    /////////////////////////////////////////////////////////////////
    // C-BGP Java Native Interface Initialization/finalization
    /////////////////////////////////////////////////////////////////

    // -----[ init ]-------------------------------------------------
    /**
     * Inits the C-BGP JNI API. The filename of an optional log file
     * can be mentionned.
     *
     * @param filename the filename of an optional log file
     */
    public native synchronized void init(String filename)
    	throws CBGPException;

    // -----[ destroy ]----------------------------------------------
    /**
     * Finalizes the C-BGP JNI API.
     */
    public native synchronized void destroy();

    // -----[ consoleSetOutListener ]--------------------------------
    /**
     * Sets the listener for C-BGP's standard output stream.
     *
     * @param l a listener
     */
    public native synchronized void consoleSetOutListener(ConsoleEventListener l);

    // -----[ consoleSetErrListener ]--------------------------------
    /**
     * Sets the listener for C-BGP's standard error stream.
     *
     * @param l a listener
     */
    public native synchronized void consoleSetErrListener(ConsoleEventListener l);

    // -----[ consoleSetErrLevel ]-----------------------------------
    /**
     * Sets the current output log-level to i.
     *
     * @param i the new log-level
     */
    public native synchronized void consoleSetLevel(int i);


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
	Vector<Node> netGetNodes()
	throws CBGPException;
    // -----[ netGetNode ]-------------------------------------------
    public native synchronized
	Node netGetNode(String sNodeAddr)
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
    // Simulation queue management
    /////////////////////////////////////////////////////////////////

    // -----[ simRun ]-----------------------------------------------
    /**
     * Processes the queue of events until the queue is empty.
     */
    public native synchronized void simRun()
    	throws CBGPException;

    // -----[ simStep ]----------------------------------------------
    /**
     * Processes up to i events from the queue. If the queue contains
     * at least i events, i events will be processed. If the queue
     * contains less than i events, all the events will be processed.
     *
     * @param i the maximum number of events to process.
     */
    public native synchronized void simStep(int i)
    	throws CBGPException;

    // -----[ simClear ]---------------------------------------------
    /**
     * Clears the event queue.
     */
    public native synchronized void simClear()
    	throws CBGPException;

    // -----[ simGetEventCount ]-------------------------------------
    /**
     * Returns the number of events in the queue.
     *
     * @return the number of events
     */
    public native synchronized long simGetEventCount()
    	throws CBGPException;

    // -----[ simGetEvent ]------------------------------------------
    /**
     * Returns the event at position i.
     *
     * @param i the position of the event in the queue
     */
    public native synchronized Message simGetEvent(int i)
	throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Miscellaneous methods
    /////////////////////////////////////////////////////////////////

    // -----[ runCmd ]-----------------------------------------------
    /**
     * Runs a single C-BGP command. The command is expressed as a
     * single line of text.
     *
     * @param line a C-BGP command
     */
    public native synchronized void runCmd(String line)
    	throws CBGPException;

    // -----[ runScript ]--------------------------------------------
    /**
     * Runs a complete C-BGP script file whose name is filename.
     *
     * @param filename the name of a C-BGP script file.
     */
    public native synchronized void runScript(String filename)
		throws CBGPException;

    // -----[ cliGetPrompt ]----------------------------------------
    /**
     * Returns the current C-BGP CLI prompt.
     *
     * @return the current prompt.
     */
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

    // -----[ setBGPMsgListener ]------------------------------------
    public native void setBGPMsgListener(BGPMsgListener listener);


    /////////////////////////////////////////////////////////////////
    //
    // JNI library loading
    //
    /////////////////////////////////////////////////////////////////

    // -----[ JNI library loading ]----------------------------------
    static {
	try {
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

    
