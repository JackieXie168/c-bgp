package be.ac.ucl.ingi.cbgp; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.bgp.Domain;
import be.ac.ucl.ingi.cbgp.bgp.Router;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPScriptException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Interface;
import be.ac.ucl.ingi.cbgp.net.Message;
import be.ac.ucl.ingi.cbgp.net.Node;
import be.ac.ucl.ingi.cbgp.net.Subnet;

// -----[ CBGP ]-----------------------------------------------------
/**
 * This is C-BGP's Java Native Interface (JNI). This class is only a
 * wrapper to C functions contained in the csim library (libcsim).
 * 
 * @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
 * @author Sebastien Tandel (standel@info.ucl.ac.be)
 * @date 27/10/2004
 * @lastdate 16/10/2007
 */
public class CBGP {

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
    public native synchronized void destroy()
	throws CBGPException;

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

    // -----[ netAddNode ]-------------------------------------------
    public native synchronized Node netAddNode(String addr, int domain)
    	throws CBGPException;
    
    // -----[ netAddSubnet ]-----------------------------------------
    public native synchronized Subnet netAddSubnet(String prefix, int type)
    	throws CBGPException;
    
    // -----[ netAddDomain ]-----------------------------------------
    /**
     * Add an IGP domain.
     *
     * @param i the domain's identifier
     * @return the newly created domain
     */
    public native synchronized IGPDomain netAddDomain(int iDomain)
		throws CBGPException;

    // -----[ netGetDomain ]-----------------------------------------
    /**
     * Get one IGP domain
     * 
     * @param iDomain is the domain identifier
     * @return the requested IGP domain if it exists, null if it does
     *         not exist.
     */
    public native synchronized IGPDomain netGetDomain(int iDomain)
    	throws CBGPException;
    
    // -----[ netGetDomains ]----------------------------------------
    /**
     * Get the list of all IGP domains.
     *
     * @return a Vector of IGPDomain's
     */
    public native synchronized Vector<IGPDomain> netGetDomains()
		throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Nodes management
    /////////////////////////////////////////////////////////////////

    // -----[ netGetNodes ]------------------------------------------
    /**
     * Get the list of all nodes.
     *
     * @return a Vector of Node's
     */
    public native synchronized Vector<Node> netGetNodes()
		throws CBGPException;

    // -----[ netGetNode ]-------------------------------------------
    /**
     * Get a specific node based on its identifier.
     *
     * @param sNodeAddr the node's identifier
     * @return the requested Node
     */
    public native synchronized Node netGetNode(String sNodeAddr)
		throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // Links management
    /////////////////////////////////////////////////////////////////

    // -----[ netAddLink ]-------------------------------------------
    /**
     * Add a new point-to-point link.
     *
     * @param sSrcAddr the link's head end
     * @param sDstAddr the link's tail end
     * @param iWeight the link's propagation delay
     * @return the newly created Link
     */
    public native synchronized Interface netAddLink(String sSrcAddr,
    	String sDstAddr, int iWeight) 
		throws CBGPException;
    
    /////////////////////////////////////////////////////////////////
    // BGP domains management
    /////////////////////////////////////////////////////////////////

    // -----[ bgpGetDomains ]----------------------------------------
    /**
     * Get the list of all BGP domains (ASes).
     *
     * @return a Vector of Domains
     */
    public native synchronized Vector<Domain> bgpGetDomains()
		throws CBGPException;


    /////////////////////////////////////////////////////////////////
    // BGP router management
    /////////////////////////////////////////////////////////////////

    // -----[ bgpAddRouter ]-----------------------------------------
    /**
     * Add a BGP router.
     *
     * @param sName the router's name
     * @param sAddr the router's identifier (must exist)
     * @param iASNumber the router's AS Number (will be created if it
     *                   does not already exists)
     * @return the newly created Router
     */
    public native synchronized
	Router bgpAddRouter(String sName, String sAddr, int iASNumber)
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
    	throws CBGPScriptException;

    // -----[ runScript ]--------------------------------------------
    /**
     * Runs a complete C-BGP script file whose name is filename.
     *
     * @param filename the name of a C-BGP script file.
     */
    public native synchronized void runScript(String filename)
		throws CBGPScriptException;

    // -----[ cliGetPrompt ]----------------------------------------
    /**
     * Returns the current C-BGP CLI prompt.
     *
     * @return the current prompt.
     */
    public native synchronized String cliGetPrompt()
    	throws CBGPException;

    // -----[ cliGetHelp ]------------------------------------------
    /* To be added */

    // -----[ cliGetSyntax ]----------------------------------------
    /* To be added */

    // -----[ getVersion ]------------------------------------------
    /**
     * Returns the C-BGP version. The version should be of the form
     * "x.y.z" where x,y,z are positive integer numbers. For
     * example, a possible version is '1.4.1'. The returned version
     * could also contain an additional suffix part separated by
     * a dash. For example, a possible version is '1.4.1-rc2'.
     *
     * @return a String containing the C-BGP version
     */
    public native synchronized String getVersion()
    	throws CBGPException;

    // -----[ getErrorMsg ]-----------------------------------------
    /**
     * Returns the last error message.
     *
     * @return a String containing the last error message
     */
    public native synchronized String getErrorMsg(int iErrorCode);

    
    /////////////////////////////////////////////////////////////////
    // Experimental features
    /////////////////////////////////////////////////////////////////

    // -----[ loadMRT ]----------------------------------------------
    /**
     * This is an experimental method. Use at your own risk.
     */
    public native Vector<be.ac.ucl.ingi.cbgp.bgp.Route> loadMRT(String sFileName);

    // -----[ setBGPMsgListener ]------------------------------------
    /**
     * This is an experimental method. Use at your own risk.
     */
    public native void setBGPMsgListener(BGPMsgListener listener);
    
    // -----[ netExport ]--------------------------------------------
    /**
     * Export the current topology and routing setup to a file in CLI
     * format.
     * 
     * This is an experimental method. Use at your own risk.
     */
    public native void netExport(String sFileName)
    	throws CBGPException;
    
    // -----[ netGetLinks ]------------------------------------------
    /**
     * This is an experimental method. Use at your own risk.
     */
    public native Vector<Interface> netGetLinks()
    	throws CBGPException;


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
