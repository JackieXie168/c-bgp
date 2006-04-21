// ==================================================================
// @(#)jni_interface.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 21/04/2006
// ==================================================================
// TODO :
//   cannot be used with Walton [ to be fixed by STA ]
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/cli_ctx.h>
#include <libgds/gds.h>
#include <libgds/log.h>
#include <libgds/str_util.h>

/*** jni_md.h ***
 * Include machine-dependent typedefs (is this portable ?)
 * This header file is located in linux/jni_md.h under Linux while it
 * is located in solaris/jni_md.h under Solaris. I haven't found a
 * specification for this. So we might get portability problems at a
 * point...
 */
#include <jni_md.h>

#include <jni.h>
#include <string.h>
#include <assert.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_CBGP.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>
#include <jni/impl/bgp_Domain.h>
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_Router.h>
#include <jni/impl/net_IGPDomain.h>
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>

#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>
#include <net/prefix.h>
#include <net/igp.h>
#include <net/routing.h>

#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/domain.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/filter.h>
#include <bgp/filter_t.h>
#include <bgp/filter_registry.h>
#include <bgp/rexford.h>
#include <bgp/route_map.h>

#include <sim/simulator.h>

#include <cli/common.h>

// -----[ SConsoleCtx ]----------------------------------------------
/**
 * Context structure used for console callbacks
 * (ConsoleEventListener).
 *
 * IMPORTANT NOTE: the joListener field must contain a global
 * reference to the Java listener !!! Use NewGlobalRef to create a
 * global reference from a local reference.
 */
typedef struct {
  JavaVM * jVM;
  jobject joCBGP;
  jobject joListener;
} SConsoleCtx;

static SConsoleCtx sConsoleOutCtx;
static SConsoleCtx sConsoleErrCtx;

static jobject joGlobalCBGP= NULL;


/////////////////////////////////////////////////////////////////////
// C-BGP Java Native Interface Initialization/finalization
/////////////////////////////////////////////////////////////////////

// -----[ init ]-----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_init
  (JNIEnv * jEnv, jobject joCBGP, jstring file_log)
{
  //const char * cFileLog;

  if (joGlobalCBGP != NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "CBGP class already initialized");
    return;
  }
  joGlobalCBGP= joCBGP;

  // Initialize GDS library
  gds_init(0);
  //gds_init(GDS_OPTION_MEMORY_DEBUG);

  // Initialize console listeners contexts
  sConsoleOutCtx.jVM= NULL;
  sConsoleErrCtx.jVM= NULL;
  sConsoleOutCtx.joListener= NULL;
  sConsoleErrCtx.joListener= NULL;

  log_set_level(pLogDebug, LOG_LEVEL_WARNING);

  /*
  cFileLog = (*env)->GetStringUTFChars(env, file_log, JNI_FALSE);
  //log_set_level(pMainLog, LOG_LEVEL_WARNING);
  log_set_level(pMainLog, LOG_LEVEL_EVERYTHING);
  if (strcmp(cFileLog, ""))
    log_set_file(pMainLog, (char *)cFileLog);
  else
    log_set_stream(pMainLog, stderr);
  (*env)->ReleaseStringUTFChars(env, file_log, cFileLog);
  */

  // Hash init code commented in order to allow parameter setup
  // through he command-line/script
  //_comm_hash_init();
  //_path_hash_init();

  _network_create();
  _bgp_domain_init();
  _igp_domain_init();
  _ft_registry_init();
  _filter_path_regex_init();
  _route_map_init();
  _rexford_init();
  simulator_init(); 

  fprintf(stdout, "C-BGP library started.\n");
}

// -----[ destroy ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_destroy
  (JNIEnv * jEnv, jobject joCBGP)
{
  // Delete global references to console listeners
  if (sConsoleOutCtx.joListener != NULL) {
    (*jEnv)->DeleteGlobalRef(jEnv, sConsoleOutCtx.joListener);
    sConsoleOutCtx.joListener= NULL;
  }
  if (sConsoleErrCtx.joListener != NULL) {
    (*jEnv)->DeleteGlobalRef(jEnv, sConsoleErrCtx.joListener);
    sConsoleErrCtx.joListener= NULL;
  }

  // Free simulator data structures...
  simulator_done();

  joGlobalCBGP= NULL;

  fprintf(stdout, "C-BGP library stopped.\n");
}

// -----[ _console_listener ]----------------------------------------
static void _console_listener(void * pContext, char * pcBuffer)
{
  SConsoleCtx * pCtx= (SConsoleCtx *) pContext;
  JavaVM * jVM= pCtx->jVM;
  JNIEnv * jEnv= NULL;
  void * pTmp= &jEnv; // Need this unless GCC will complain about the
		      // typecast (void **) &jEnv
  jobject joEvent= NULL;
  
  // Attach the current thread to the Java VM (and get a pointer to
  // the JNI environment). This is required since the calling thread
  // might be different from the one that initialized the related
  // console listener object.
  if ((*jVM)->AttachCurrentThread(jVM, pTmp, NULL) != 0) {
    fprintf(stderr, "ERROR: AttachCurrentThread failed in JNI::_console_listener\n");
    return;
  }

  // Create new ConsoleEvent object.
  if ((joEvent= cbgp_jni_new_ConsoleEvent(jEnv, pcBuffer)) == NULL)
    return;

  // Call the listener "eventFired" method.
  cbgp_jni_call_void(jEnv, pCtx->joListener,
		     "eventFired",
		     "(Lbe/ac/ucl/ingi/cbgp/ConsoleEvent;)V",
		     joEvent);
}

// -----[ consoleSetOutListener ]------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetOutListener
 * Signature: (Lbe/ac/ucl/ingi/cbgp/ConsoleEventListener;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetOutListener
  (JNIEnv * jEnv, jobject joCBGP, jobject joListener)
{
  fprintf(stderr, "console_set_out_listener[%p]\n", jEnv);
  
  log_destroy(&pLogOut);
  sConsoleOutCtx.joCBGP= joCBGP;

  // Remove global reference to former listener (if applicable)
  if (sConsoleOutCtx.joListener != NULL)
    (*jEnv)->DeleteGlobalRef(jEnv, sConsoleOutCtx.joListener);

  // Get reference to Java Virtual Machine (required by the JNI callback)
  if (sConsoleOutCtx.jVM == NULL)
    if ((*jEnv)->GetJavaVM(jEnv, &sConsoleOutCtx.jVM) != 0) {
      cbgp_jni_throw_CBGPException(jEnv, "could not get reference to Java VM");
      return;
    }

  // Add a global reference to the listener object (returns NULL if
  // system has run out of memory)
  sConsoleOutCtx.joListener= (*jEnv)->NewGlobalRef(jEnv, joListener);
  if (sConsoleOutCtx.joListener == NULL) {
    fprintf(stderr, "ERROR: NewGlobalRef returned NULL in consoleSetOutListener");
    return;
  }

  pLogOut= log_create_callback(_console_listener, &sConsoleOutCtx);
}

// -----[ consoleSetErrListener ]------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetErrListener
 * Signature: (Lbe/ac/ucl/ingi/cbgp/ConsoleEventListener;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetErrListener
  (JNIEnv * jEnv, jobject joCBGP, jobject joListener)
{
  fprintf(stderr, "console_set_err_listener[%p]\n", jEnv);
  log_destroy(&pLogErr);
  sConsoleErrCtx.joCBGP= joCBGP;

  // Remove global reference to former listener (if applicable)
  if (sConsoleErrCtx.joListener != NULL)
    (*jEnv)->DeleteGlobalRef(jEnv, sConsoleErrCtx.joListener);

  // Get reference to Java Virtual Machine (required by the JNI callback)
  if (sConsoleErrCtx.jVM == NULL)
    if ((*jEnv)->GetJavaVM(jEnv, &sConsoleErrCtx.jVM) != 0) {
      cbgp_jni_throw_CBGPException(jEnv, "could not get reference to Java VM");
      return;
    }

  // Add a global reference to the listener object (returns NULL if
  // system has run out of memory)
  sConsoleErrCtx.joListener= (*jEnv)->NewGlobalRef(jEnv, joListener);
  if (sConsoleErrCtx.joListener == NULL) {
    fprintf(stderr, "ERROR: NewGlobalRef returned NULL in consoleSetErrListener");
    return;
  }

  pLogErr= log_create_callback(_console_listener, &sConsoleErrCtx);
}

// -----[ consoleSetLevel ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetLevel
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetLevel
  (JNIEnv * env, jobject joCBGP, jint iLevel)
{
  if ((iLevel < 0) || (iLevel > 255)) {
    cbgp_jni_throw_CBGPException(env, "invalid log-level");
    return;
  }

  //log_set_level(pMainLog, (uint8_t) iLevel);
}





/////////////////////////////////////////////////////////////////////
// IGP Domains Management
/////////////////////////////////////////////////////////////////////

// -----[ netAddDomain ]---------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netAddDomain
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/IGPDomain;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddDomain
(JNIEnv * jEnv, jobject joCBGP, jint iDomain)
{
  SIGPDomain * pDomain= NULL;

  if ((iDomain < 0) || (iDomain > 65535)) {
    cbgp_jni_throw_CBGPException(jEnv, "invalid domain number");
    return NULL;
  }

  if (exists_igp_domain((uint16_t) iDomain)) {
    cbgp_jni_throw_CBGPException(jEnv, "domain already exists");
    return NULL;
  }

  pDomain= igp_domain_create((uint16_t) iDomain, DOMAIN_IGP);
  register_igp_domain(pDomain);

  return cbgp_jni_new_net_IGPDomain(jEnv, joCBGP, pDomain);
}

// -----[ _netGetDomains ]-------------------------------------------
static int _netGetDomains(SIGPDomain * pDomain, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joDomain;

  // Create IGPDomain instance
  joDomain= cbgp_jni_new_net_IGPDomain(pCtx->jEnv,
				       pCtx->joCBGP,
				       pDomain);
  if (joDomain == NULL)
    return -1;

  // Add to Vector
  if (cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joDomain) != 0)
    return -1;
  
  return 0;
}

// -----[ netGetDomains ]--------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetDomains
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetDomains
  (JNIEnv * jEnv, jobject joCBGP)
{
  SJNIContext sCtx;
  jobject joVector;

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return NULL;

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;
  sCtx.joCBGP= joCBGP;

  if (igp_domain_for_each(_netGetDomains, &sCtx) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not get list of domains");
    return NULL;
  }

  return joVector;
}


//////////////////////////////////////////////////////////////////////
// Nodes management
//////////////////////////////////////////////////////////////////////

// -----[ netAddNode ]-----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netAddNode
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddNode
(JNIEnv * jEnv, jobject joCBGP, jstring jsAddr, jint iDomain)
{
  SIGPDomain * pDomain;
  SNetNode * pNode; 
  net_addr_t tNetAddr;

  // Check that the domain is valid
  pDomain= cbgp_jni_net_domain_from_int(jEnv, iDomain);
  if (pDomain == NULL)
    return NULL;

  if (ip_jstring_to_address(jEnv, jsAddr, &tNetAddr) != 0)
    return NULL;
  if ((pNode= node_create(tNetAddr)) == NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "node could not be created");
    return NULL;
  }

  if (network_add_node(pNode) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "node already exists");
    return NULL;
  }

  igp_domain_add_router(pDomain, pNode);

  return cbgp_jni_new_net_Node(jEnv, joCBGP, pNode);
}

// -----[ netGetNodes ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetNodes
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetNodes
  (JNIEnv * env, jobject joCBGP)
{
  SEnumerator * pEnum= trie_get_enum(network_get()->pNodes);
  SNetNode * pNode;
  jobject joVector;
  jobject joNode;

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
    return NULL;

  while (enum_has_next(pEnum)) {
    pNode= *(SNetNode **) enum_get_next(pEnum);

    // Create Node object
    if ((joNode= cbgp_jni_new_net_Node(env, joCBGP, pNode)) == NULL) {
      joVector= NULL;
      break;
    }

    // Add to Vector
    cbgp_jni_Vector_add(env, joVector, joNode);
  }
  enum_destroy(&pEnum);

  return joVector;
}

// -----[ nodeInterfaceAdd ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeInterfaceAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_nodeInterfaceAdd
  (JNIEnv * env, jobject obj, jstring net_addr_id, jstring net_addr_int, jstring mask)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetInterface * pInterface; 
  net_addr_t iNetAddrId, iNetAddrInt;
  SPrefix * pMask = MALLOC(sizeof(SPrefix));
 
  if (ip_jstring_to_address(env, net_addr_id, &iNetAddrId) != 0)
     return -1;
  if ((pNode= network_find_node(pNetwork, iNetAddrId)) != NULL)
    return -1;
  if (ip_jstring_to_address(env, net_addr_int, &iNetAddrInt) != 0)
    return -1;
  if (ip_jstring_to_prefix(env, mask, pMask) != 0)
    return -1;
  
  pInterface= node_interface_create();
  pInterface->tAddr= iNetAddrInt;
  pInterface->tMask= pMask;

  return node_interface_add(pNode, pInterface);
}*/

// -----[ netNodeRouteAdd ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeRouteAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netNodeRouteAdd
  (JNIEnv * env, jobject obj, jstring jsAddr, jstring jsPrefix,
   jstring jsNexthop, jint jiWeight)
{
  SNetNode * pNode;
  SPrefix sPrefix;
  net_addr_t tNextHop;

  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return;
  if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
    return;
  if (ip_jstring_to_address(env, jsNexthop, &tNextHop) != 0)
    return;

  if (node_rt_add_route(pNode, sPrefix, tNextHop, tNextHop,
			jiWeight, NET_ROUTE_STATIC) != 0) {
    cbgp_jni_throw_CBGPException(env, "could not add route");
    return;
  }
}


/////////////////////////////////////////////////////////////////////
// Links management
/////////////////////////////////////////////////////////////////////

// -----[ netAddLink ]-----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeLinkAdd
 * Signature: (II)I
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddLink
  (JNIEnv * jEnv, jobject joCBGP, jstring jsSrcAddr, jstring jsDstAddr,
   jint jiWeight)
{
  SNetNode * pNodeSrc, * pNodeDst;

  if ((pNodeSrc= cbgp_jni_net_node_from_string(jEnv, jsSrcAddr)) == NULL)
    return NULL;
  if ((pNodeDst= cbgp_jni_net_node_from_string(jEnv, jsDstAddr)) == NULL)
    return NULL;

  if (node_add_link_to_router(pNodeSrc, pNodeDst, jiWeight, 1) < 0) {
    cbgp_jni_throw_CBGPException(jEnv, "link already exists");
    return NULL;
  }

  return cbgp_jni_new_net_Link(jEnv, joCBGP, NULL);
}


////////////////////////////////b/////////////////////////////////////
// BGP domains management
/////////////////////////////////////////////////////////////////////

// -----[ _bgpGetDomains ]-------------------------------------------
static int _bgpGetDomains(SBGPDomain * pDomain, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joDomain;

  // Create BGPDomain instance
  joDomain= cbgp_jni_new_bgp_Domain(pCtx->jEnv,
				    pCtx->joCBGP,
				    pDomain);

  if (joDomain == NULL)
    return -1;

  // Add to Vector
  if (cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joDomain) != 0)
    return -1;
  
  return 0;
}

// -----[ bgpGetDomains ]--------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpGetDomains
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpGetDomains
  (JNIEnv * jEnv, jobject joCBGP)
{
  SJNIContext sCtx;
  jobject joVector= NULL;

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return NULL;

  sCtx.jEnv= jEnv;
  sCtx.joCBGP= joCBGP;
  sCtx.joVector= joVector;

  if (bgp_domains_for_each(_bgpGetDomains, &sCtx) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not get list of domains");
    return NULL;
  }

  return joVector;
}


/////////////////////////////////////////////////////////////////////
// BGP router management
/////////////////////////////////////////////////////////////////////

// -----[ bgpAddRouter ]---------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpAddRouter
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Lbe/ac/ucl/ingi/cbgp/bgp/Router;
 *
 * Note: 'net_addr' must be non-NULL. 'name' may be NULL.
 * Returns: -1 on error and 0 on success.
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpAddRouter
  (JNIEnv * jEnv, jobject joCBGP, jstring jsName, jstring jsAddr,
   jint jiASNumber)
{
  SNetNode * pNode;
  SBGPRouter * pRouter;
  //const char * pcName;

  /* Try to find related router. */
  if ((pNode= cbgp_jni_net_node_from_string(jEnv, jsAddr)) == NULL)
    return NULL;

  /* Create BGP router, and register. */
  pRouter= bgp_router_create(jiASNumber, pNode);
  if (node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter, 
			     (FNetNodeHandlerDestroy) bgp_router_destroy,
			     bgp_router_handle_message)) {
    bgp_router_destroy(&pRouter);
    cbgp_jni_throw_CBGPException(jEnv, "Node already supports BGP");
    return NULL;
  }

  /*
  if (jsName != NULL) {
    pcName = (*env)->GetStringUTFChars(env, jsName, NULL);
    bgp_router_set_name(pRouter, str_create((char *) pcName));
    (*env)->ReleaseStringUTFChars(env, jsName, pcName);
    }*/

  return cbgp_jni_new_bgp_Router(jEnv, joCBGP, pRouter);
}

// -----[ bgpRouterPeerReflectorClient ]-----------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterPeerReflectorClient
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerReflectorClient
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  bgp_peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);
}


// -----[ bgpRouterPeerVirtual ]-------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterPeerVirtual
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerVirtual
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
  bgp_peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);
}

// -----[ bgpRouterPeerUp ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterNeighborUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerUp
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr, jboolean bUp)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  if (bUp == JNI_TRUE) {
    if (bgp_peer_open_session(pPeer) != 0)
      cbgp_jni_throw_CBGPException(env, "could not open session");
  } else {
    if (bgp_peer_close_session(pPeer) != 0)
      cbgp_jni_throw_CBGPException(env, "could not close session");
  }
}

// -----[ bgpRouterRescan ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterRescan
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterRescan
  (JNIEnv * env, jobject obj, jstring jsAddr)
{
  SBGPRouter * pRouter;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsAddr)) == NULL)
    return;
 
  if (bgp_router_scan_rib(pRouter) != 0)
    cbgp_jni_throw_CBGPException(env, "could not rescan router");
}

// -----[ bgpRouterGetAdjRib ]---------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterShowRibIn
 * Signature: (Ljava/lang/String;)V
 */
//JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterGetAdjRib
//  (JNIEnv * env, jobject obj, jstring jsRouterAddr,
//   jstring jsPeerAddr, jstring jsPrefix, jboolean bIn)
//{
//  SBGPRouter * pRouter;
//  jobject joVector;
//  jobject joRoute;
//  SPrefix sPrefix;
//  net_addr_t tPeerAddr;
//  int iIndex;
//  SPeer * pPeer= NULL;
//  SRoute * pRoute;
//
//  /* Get the router instance */
//  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
//    return NULL;
//
//  /* Create a Vector object that will hold the BGP routes to be
//     returned */
//  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
//    return NULL;
//
//  /* Convert the peer address */
//  if (jsPeerAddr != NULL) {
//    if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
//      return NULL;
//    if ((pPeer= bgp_router_find_peer(pRouter, tPeerAddr)) == NULL)
//      return NULL;
//  }
//
//  /* Convert prefix */
//  if (jsPrefix == NULL) {
//    
//    /* Add routes into the Vector object */
//    if (pPeer == NULL) {
//      
//      for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
//	pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
//	
//	/* For each route in the RIB, create a new BGPRoute object and add
//	   it to the Vector object */
//	SJNIContext sCtx;
//	sCtx.pRouter= pRouter;
//	sCtx.joVector= joVector;
//	sCtx.jEnv= env;
//	if (bIn == JNI_TRUE) {
//	  if (rib_for_each(pPeer->pAdjRIBIn, cbgp_jni_get_rib_route, &sCtx) != 0)
//	    return NULL;
//	} else {
//	  if (rib_for_each(pPeer->pAdjRIBOut, cbgp_jni_get_rib_route, &sCtx) != 0)
//	    return NULL;
//	}
//	
//      }
//      
//    } else {
//      
//      /* For each route in the RIB, create a new BGPRoute object and add
//	 it to the Vector object */
//      SJNIContext sCtx;
//      sCtx.pRouter= pRouter;
//      sCtx.joVector= joVector;
//      sCtx.jEnv= env;
//      if (bIn == JNI_TRUE) {
//	if (rib_for_each(pPeer->pAdjRIBIn, cbgp_jni_get_rib_route, &sCtx) != 0)
//	  return NULL;
//      } else {
//	if (rib_for_each(pPeer->pAdjRIBOut, cbgp_jni_get_rib_route, &sCtx) != 0)
//	  return NULL;
//      }
//
//    }
//    
//  } else {
//
//    /* Convert the prefix */
//    if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
//      return NULL;
//    
//    if (pPeer == NULL) {
//      
//      for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
//	pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
//
//	pRoute= NULL;
//#ifndef __EXPERIMENTAL_WALTON__ 
//	if (sPrefix.uMaskLen == 32) {
//	  if (bIn == JNI_TRUE) {
//	    pRoute= rib_find_best(pPeer->pAdjRIBIn, sPrefix);
//	  } else {
//	    pRoute= rib_find_best(pPeer->pAdjRIBOut, sPrefix);
//	  }
//	} else {
//	  if (bIn == JNI_TRUE) {
//	    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
//	  } else {
//	    pRoute= rib_find_exact(pPeer->pAdjRIBOut, sPrefix);
//	  }
//	}
//#endif
//	
//	if (pRoute != NULL) {
//	  if ((joRoute= cbgp_jni_new_BGPRoute(env, pRoute)) == NULL)
//	    return NULL;
//	  cbgp_jni_Vector_add(env, joVector, joRoute);
//	}
//      }
//
//    } else {
//
//      pRoute= NULL;
//#ifndef __EXPERIMENTAL_WALTON__ 
//      if (sPrefix.uMaskLen == 32) {
//	if (bIn == JNI_TRUE) {
//	  pRoute= rib_find_best(pPeer->pAdjRIBIn, sPrefix);
//	} else {
//	  pRoute= rib_find_best(pPeer->pAdjRIBOut, sPrefix);
//	}
//      } else {
//	if (bIn == JNI_TRUE) {
//	  pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
//	} else {
//	  pRoute= rib_find_exact(pPeer->pAdjRIBOut, sPrefix);
//	}
//      }
//#endif
//      
//      if (pRoute != NULL) {
//	if ((joRoute= cbgp_jni_new_BGPRoute(env, pRoute)) == NULL)
//	  return NULL;
//	cbgp_jni_Vector_add(env, joVector, joRoute);
//      }
//
//    }
//
//  }
//
//  return joVector;
//}

// -----[ bgpRouterLoadRib ]-----------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterLoadRib
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterLoadRib
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsFileName)
{
  SBGPRouter * pRouter;
  const char * cFileName;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return;

  cFileName= (*env)->GetStringUTFChars(env, jsFileName, NULL);
  if (bgp_router_load_rib((char *) cFileName, pRouter) != 0)
    cbgp_jni_throw_CBGPException(env, "could not load RIB");
  (*env)->ReleaseStringUTFChars(env, jsFileName, cFileName);
}

// -----[ bgpDomainRescan ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpDomainRescan
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpDomainRescan
  (JNIEnv * env, jobject obj, jint jiASNumber)
{
  if (!exists_bgp_domain((uint32_t) jiASNumber)) {
    cbgp_jni_throw_CBGPException(env, "domain does not exist");
    return;
  }

  if (bgp_domain_rescan(get_bgp_domain((uint32_t) jiASNumber)) != 0)
    cbgp_jni_throw_CBGPException(env, "could not rescan domain");
}

// -----[ bgpFilterInit ]--------------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpFilterInit
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterInit
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr, 
   jstring type)
{
  SBGPRouter * pRouter;
  SPeer * pPeer;
  net_addr_t tPeerAddr;

  int ret =0;
  const jbyte * cType;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return -1;
  if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
    return -1;
  
  if ((pPeer= bgp_router_find_peer(pRouter, tPeerAddr)) == NULL)
    return -1;

  cType = (*env)->GetStringUTFChars(env, type, NULL);
  if (strcmp(cType, FILTER_IN) == 0) { 
    pFilter = filter_create();
    peer_set_in_filter(pPeer, pFilter);
  } else if (strcmp(cType, FILTER_OUT) == 0) {
    pFilter = filter_create();
    peer_set_out_filter(pPeer, pFilter);
  } else 
    ret = 1;

  (*env)->ReleaseStringUTFChars(env, type, cType);
  return ret;

}
*/

// -----[ bgpFilterMatchPrefixIn ]-----------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterMatchPrefix
 * Signature: (Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterMatchPrefixIn
  (JNIEnv * env, jobject obj, jstring prefix)
{
  const jbyte * cPrefix;
  char * pcEndPtr;
  SPrefix Prefix;

  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", cPrefix);
  }
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  pMatcher = filter_match_prefix_in(Prefix);
  return 0;
}
*/

/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterMatchPrefix
 * Signature: (Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterMatchPrefixIs
  (JNIEnv * env, jobject obj, jstring prefix)
{
  const jbyte * cPrefix;
  char * pcEndPtr;
  SPrefix Prefix;

  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", cPrefix);
  }
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  pMatcher = filter_match_prefix_equals(Prefix);
  return 0;
}
*/

/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterAction
 * Signature: (ILjava/lang/String;)I
 *
 */
/* BQU: TO BE FIXED!!!

#define ACTION_PERMIT	    0x01
#define ACTION_DENY	    0x02
#define ACTION_LOCPREF	    0x03
#define ACTION_ADD_COMM	    0x04
#define ACTION_PATH_PREPEND 0x05
JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterAction
  (JNIEnv * env, jobject obj, jint type, jstring value)
{

  const jbyte * cValue;
  uint32_t iValue;

  switch (type) {
    case ACTION_PERMIT:
      pAction = filter_action_accept();
      break;
    case ACTION_DENY:
      pAction = filter_action_deny();
      break;
    case ACTION_LOCPREF:
      cValue = (*env)->GetStringUTFChars(env, value, NULL);
      iValue = atoi(cValue);
      (*env)->ReleaseStringUTFChars(env, value, cValue);
      pAction = filter_action_pref_set(iValue);
      break;
    case ACTION_ADD_COMM:
      cValue = (*env)->GetStringUTFChars(env, value, NULL);
      iValue = atoi(cValue);
      (*env)->ReleaseStringUTFChars(env, value, cValue);
      pAction = filter_action_comm_append(iValue);
      break;
    case ACTION_PATH_PREPEND:
      cValue = (*env)->GetStringUTFChars(env, value, NULL);
      iValue = atoi(cValue);
      (*env)->ReleaseStringUTFChars(env, value, cValue);
      pAction = filter_action_path_prepend(iValue);
      break;
    default:
      return 1;
  }
  filter_add_rule(pFilter, pMatcher, pAction);
  return 0;
}
*/

/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterFinalize
 * Signature: ()V
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterFinalize
  (JNIEnv *env , jobject obj)
{
  pFilter= NULL;
  pMatcher= NULL;
  pAction= NULL;
}
*/

/////////////////////////////////////////////////////////////////////
// Miscellaneous methods
/////////////////////////////////////////////////////////////////////

// -----[ simRun ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simRun
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simRun
  (JNIEnv * jEnv, jobject joCBGP)
{
  if (simulator_run() != 0)
    cbgp_jni_throw_CBGPException(jEnv, "simulation error");
}

// -----[ simStep ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simStep
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simStep
  (JNIEnv * jEnv, jobject joCBGP, jint jiNumSteps)
{
  if (simulator_step(jiNumSteps) != 0)
    cbgp_jni_throw_CBGPException(jEnv, "simulation error");
}

// -----[ simClear ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simClear
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simClear
  (JNIEnv * jEnv, jobject joCBGP)
{
  simulator_clear();
}

// -----[ simGetEventCount ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simGetEventCount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simGetEventCount
  (JNIEnv * jEnv, jobject joCBGP)
{
  return (jlong) simulator_get_num_events();
}

// -----[ runCmd ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runCmd
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runCmd
  (JNIEnv * env, jobject obj, jstring jsCommand)
{
  const char * cCommand;

  if (jsCommand == NULL)
    return;

  cCommand= (*env)->GetStringUTFChars(env, jsCommand, NULL);
  if (cli_execute_line(cli_get(), (char *) cCommand) != 0)
    cbgp_jni_throw_CBGPException(env, "could not execute command");
  (*env)->ReleaseStringUTFChars(env, jsCommand, cCommand);
}

// -----[ runScript ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runScript
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runScript
  (JNIEnv * env, jobject obj, jstring jsFileName)
{
  const char * pcFileName;
  FILE * pInCli;

  pcFileName= (*env)->GetStringUTFChars(env, jsFileName, NULL);

  if ((pInCli= fopen(pcFileName, "r")) != NULL) {
    if (cli_execute_file(cli_get(), pInCli) != CLI_SUCCESS) {
      cbgp_jni_throw_CBGPException(env, "could not execute script");
    }
    fclose(pInCli);
  } else {
    cbgp_jni_throw_CBGPException(env, "could not open file");
  }

  (*env)->ReleaseStringUTFChars(env, jsFileName, pcFileName);
}

// -----[ cliGetPrompt ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    cliGetPrompt
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_cliGetPrompt
  (JNIEnv *jEnv, jobject joCBGP)
{
  char * pcPrompt= cli_context_to_string(cli_get()->pExecContext, "cbgp");

  return cbgp_jni_new_String(jEnv, pcPrompt);
}

// -----[ getVersion ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    getVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_getVersion
  (JNIEnv * jEnv, jobject joCBGP)
{
  return NULL;
}

// -----[ cbgp_jni_routes_list_function ]----------------------------
int cbgp_jni_routes_list_function(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joRoute;

  if ((joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, *(SRoute **) pItem)) == NULL)
    return -1;

  return cbgp_jni_ArrayList_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ loadMRT ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    loadMRT
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_loadMRT
  (JNIEnv *env , jobject obj, jstring jsFileName)
{
  jobject joArrayList= NULL;
  SRoutes * pRoutes;
  const char * pcFileName;
  SJNIContext sCtx;

  pcFileName= (*env)->GetStringUTFChars(env, jsFileName, NULL);
  if ((pRoutes= mrtd_load_routes(pcFileName, 0, NULL)) != NULL) {
    joArrayList= cbgp_jni_new_ArrayList(env);
    sCtx.jEnv= env;
    sCtx.joVector= joArrayList;
    if (routes_list_for_each(pRoutes, cbgp_jni_routes_list_function, &sCtx) != 0)
      joArrayList= NULL;
  }
  (*env)->ReleaseStringUTFChars(env, jsFileName, pcFileName);

  // TODO: clean whole list !!! (routes_list are only references...)
  routes_list_destroy(&pRoutes);

  return joArrayList;
}
