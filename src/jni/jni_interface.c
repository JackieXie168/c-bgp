// ==================================================================
// @(#)jni_interface.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 29/06/2007
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
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/listener.h>
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
#include <api.h>

//#define JNI_LOG 1

// -----[ jni_log_enter ]--------------------------------------------
void jni_log_enter(char * pFunction)
{
#ifdef JNI_LOG
  fprintf(stderr, "debug: enter %s\n", pFunction);
#endif
}

// -----[ jni_log_leave ]--------------------------------------------
void jni_log_leave(char * pFunction)
{
#ifdef JNI_LOG
  fprintf(stderr, "debug: leave %s\n", pFunction);
#endif
}

// -----[ CBGP listeners ]-------------------------------------------
static SJNIListener sConsoleOutListener;
static SJNIListener sConsoleErrListener;
static SJNIListener sBGPListener;

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
  if (joGlobalCBGP != NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "CBGP class already initialized");
    return;
  }

  // Create global reference that can be garbage-collected (weak)
  joGlobalCBGP= (*jEnv)->NewWeakGlobalRef(jEnv, joCBGP);

  // Initialize console listeners contexts
  jni_listener_init(&sConsoleOutListener);
  jni_listener_init(&sConsoleErrListener);
  jni_listener_init(&sBGPListener);

  // Initialize C-BGP library
  libcbgp_init();
  libcbgp_set_debug_level(LOG_LEVEL_WARNING);

  fprintf(stdout, "C-BGP library started.\n");
  fflush(stdout);

  jni_init_lock(joGlobalCBGP);
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
  jni_lock(jEnv);

  // Free simulator data structures...
  libcbgp_done();

  // Delete global references to console listeners
  jni_listener_unset(&sConsoleOutListener, jEnv);
  jni_listener_unset(&sConsoleErrListener, jEnv);
  jni_listener_unset(&sBGPListener, jEnv);

  fprintf(stdout, "C-BGP library stopped.\n");
  fflush(stdout);

  jni_unlock(jEnv);

  joGlobalCBGP= NULL;
}

// -----[ _console_listener ]----------------------------------------
static void _console_listener(void * pContext, char * pcBuffer)
{
  SJNIListener * pListener= (SJNIListener *) pContext;
  JavaVM * jVM= pListener->jVM;
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
  cbgp_jni_call_void(jEnv, pListener->joListener,
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
  jni_lock(jEnv);

  log_destroy(&pLogOut);
  jni_listener_set(&sConsoleOutListener, jEnv, joListener);
  pLogOut= log_create_callback(_console_listener, &sConsoleOutListener);

  jni_unlock(jEnv);
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
  jni_lock(jEnv);
  
  log_destroy(&pLogErr);
  jni_listener_set(&sConsoleErrListener, jEnv, joListener);
  pLogErr= log_create_callback(_console_listener, &sConsoleErrListener);

  jni_unlock(jEnv);
}

// -----[ consoleSetLevel ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetLevel
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetLevel
  (JNIEnv * jEnv, jobject joCBGP, jint iLevel)
{
  jni_lock(jEnv);
  
  if ((iLevel < 0) || (iLevel > 255)) {
    cbgp_jni_throw_CBGPException(jEnv, "invalid log-level");
    return_jni_unlock2(jEnv);
  }

  //log_set_level(pMainLog, (uint8_t) iLevel);

  jni_unlock(jEnv);
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
  jobject joDomain;

  jni_lock(jEnv);

  if ((iDomain < 0) || (iDomain > 65535)) {
    cbgp_jni_throw_CBGPException(jEnv, "invalid domain number");
    return_jni_unlock(jEnv, NULL);
  }

  if (exists_igp_domain((uint16_t) iDomain)) {
    cbgp_jni_throw_CBGPException(jEnv, "domain already exists");
    return_jni_unlock(jEnv, NULL);
  }

  pDomain= igp_domain_create((uint16_t) iDomain, DOMAIN_IGP);
  register_igp_domain(pDomain);

  joDomain= cbgp_jni_new_net_IGPDomain(jEnv, joCBGP, pDomain);

  return_jni_unlock(jEnv, joDomain);
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

  jni_lock(jEnv);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;
  sCtx.joCBGP= joCBGP;

  if (igp_domain_for_each(_netGetDomains, &sCtx) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not get list of domains");
    return_jni_unlock(jEnv, NULL);
  }

  return_jni_unlock(jEnv, joVector);
}


//////////////////////////////////////////////////////////////////////
//
// Nodes management
//
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
  jobject joNode;

  jni_lock(jEnv);

  // Check that the domain is valid
  pDomain= cbgp_jni_net_domain_from_int(jEnv, iDomain);
  if (pDomain == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_address(jEnv, jsAddr, &tNetAddr) != 0)
    return_jni_unlock(jEnv, NULL);
  if ((pNode= node_create(tNetAddr)) == NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "node could not be created");
    return_jni_unlock(jEnv, NULL);
  }

  if (network_add_node(pNode) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "node already exists");
    return_jni_unlock(jEnv, NULL);
  }

  igp_domain_add_router(pDomain, pNode);

  joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, pNode);
  return_jni_unlock(jEnv, joNode);
}

// -----[ netGetNodes ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetNodes
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetNodes
  (JNIEnv * jEnv, jobject joCBGP)
{
  SEnumerator * pEnum;
  SNetNode * pNode;
  jobject joVector;
  jobject joNode;

  jni_lock(jEnv);

  pEnum= trie_get_enum(network_get()->pNodes);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  while (enum_has_next(pEnum)) {
    pNode= *(SNetNode **) enum_get_next(pEnum);

    // Create Node object
    if ((joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, pNode)) == NULL) {
      joVector= NULL;
      break;
    }

    // Add to Vector
    cbgp_jni_Vector_add(jEnv, joVector, joNode);
  }
  enum_destroy(&pEnum);

  return_jni_unlock(jEnv, joVector);
}

// -----[ netGetNode ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetNode
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetNode
(JNIEnv * jEnv, jobject joCBGP, jstring jsAddr)
{
  SNetNode * pNode;
  jobject joNode;

  jni_lock(jEnv);

  // Find SNetNode object
  if ((pNode= cbgp_jni_net_node_from_string(jEnv, jsAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  // Create Node object
  if ((joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, pNode)) == NULL)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joNode);
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
  (JNIEnv * jEnv, jobject obj, jstring jsAddr, jstring jsPrefix,
   jstring jsNexthop, jint jiWeight)
{
  SNetNode * pNode;
  SPrefix sPrefix;
  net_addr_t tNextHop;

  jni_lock(jEnv);

  if ((pNode= cbgp_jni_net_node_from_string(jEnv, jsAddr)) == NULL)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_address(jEnv, jsNexthop, &tNextHop) != 0)
    return_jni_unlock2(jEnv);

  if (node_rt_add_route(pNode, sPrefix, tNextHop, tNextHop,
			jiWeight, NET_ROUTE_STATIC) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not add route");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
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
  jobject joLink;

  jni_lock(jEnv);

  if ((pNodeSrc= cbgp_jni_net_node_from_string(jEnv, jsSrcAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((pNodeDst= cbgp_jni_net_node_from_string(jEnv, jsDstAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  if (node_add_link_ptp(pNodeSrc, pNodeDst, jiWeight, 0, 1, 1) < 0) {
    cbgp_jni_throw_CBGPException(jEnv, "link already exists");
    return_jni_unlock(jEnv, NULL);
  }

  joLink= cbgp_jni_new_net_Link(jEnv, joCBGP, NULL);

  return_jni_unlock(jEnv, joLink);
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

  jni_lock(jEnv);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joCBGP= joCBGP;
  sCtx.joVector= joVector;

  if (bgp_domains_for_each(_bgpGetDomains, &sCtx) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not get list of domains");
    return_jni_unlock(jEnv, NULL);
  }

  return_jni_unlock(jEnv, joVector);
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
  jobject joRouter;

  jni_lock(jEnv);

  /* Try to find related router. */
  if ((pNode= cbgp_jni_net_node_from_string(jEnv, jsAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create BGP router, and register. */
  pRouter= bgp_router_create(jiASNumber, pNode);
  if (node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter, 
			     (FNetNodeHandlerDestroy) bgp_router_destroy,
			     bgp_router_handle_message)) {
    bgp_router_destroy(&pRouter);
    cbgp_jni_throw_CBGPException(jEnv, "Node already supports BGP");
    return_jni_unlock(jEnv, NULL);
  }

  /*
  if (jsName != NULL) {
    pcName = (*env)->GetStringUTFChars(env, jsName, NULL);
    bgp_router_set_name(pRouter, str_create((char *) pcName));
    (*env)->ReleaseStringUTFChars(env, jsName, pcName);
    }*/

  joRouter= cbgp_jni_new_bgp_Router(jEnv, joCBGP, pRouter);

  return_jni_unlock(jEnv, joRouter);
}

// -----[ bgpRouterPeerReflectorClient ]-----------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterPeerReflectorClient
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerReflectorClient
  (JNIEnv * jEnv, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  jni_lock(jEnv);

  pPeer= cbgp_jni_bgp_peer_from_string(jEnv, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);

  jni_unlock(jEnv);
}


// -----[ bgpRouterPeerVirtual ]-------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterPeerVirtual
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerVirtual
  (JNIEnv * jEnv, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  jni_lock(jEnv);

  pPeer= cbgp_jni_bgp_peer_from_string(jEnv, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
  bgp_peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);

  jni_unlock(jEnv);
}

// -----[ bgpRouterPeerUp ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterNeighborUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerUp
  (JNIEnv * jEnv, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr,
   jboolean bUp)
{
  SBGPPeer * pPeer;

  jni_lock(jEnv);

  pPeer= cbgp_jni_bgp_peer_from_string(jEnv, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  if (bUp == JNI_TRUE) {
    if (bgp_peer_open_session(pPeer) != 0)
      cbgp_jni_throw_CBGPException(jEnv, "could not open session");
  } else {
    if (bgp_peer_close_session(pPeer) != 0)
      cbgp_jni_throw_CBGPException(jEnv, "could not close session");
  }

  jni_unlock(jEnv);
}

// -----[ bgpRouterRescan ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterRescan
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterRescan
  (JNIEnv * jEnv, jobject obj, jstring jsAddr)
{
  SBGPRouter * pRouter;

  jni_lock(jEnv);

  if ((pRouter= cbgp_jni_bgp_router_from_string(jEnv, jsAddr)) == NULL)
    return_jni_unlock2(jEnv);
 
  if (bgp_router_scan_rib(pRouter) != 0)
    cbgp_jni_throw_CBGPException(jEnv, "could not rescan router");

  jni_unlock(jEnv);
}

// -----[ bgpRouterLoadRib ]-----------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterLoadRib
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterLoadRib
  (JNIEnv * jEnv, jobject obj, jstring jsRouterAddr, jstring jsFileName)
{
  char * cFileName;
  SBGPRouter * pRouter;
  uint8_t tFormat= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t tOptions= 0;

  jni_lock(jEnv);

  if ((pRouter= cbgp_jni_bgp_router_from_string(jEnv, jsRouterAddr)) == NULL)
    return_jni_unlock2(jEnv);

  cFileName = (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  if (bgp_router_load_rib(pRouter, (char *) cFileName,
			  tFormat, tOptions) != 0)
    cbgp_jni_throw_CBGPException(jEnv, "could not load RIB");
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, cFileName);

  jni_unlock(jEnv);
}

// -----[ bgpDomainRescan ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpDomainRescan
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpDomainRescan
  (JNIEnv * jEnv, jobject obj, jint jiASNumber)
{
  jni_lock(jEnv);
  
  if (!exists_bgp_domain((uint32_t) jiASNumber)) {
    cbgp_jni_throw_CBGPException(jEnv, "domain does not exist");
    return_jni_unlock2(jEnv);
  }

  if (bgp_domain_rescan(get_bgp_domain((uint32_t) jiASNumber)) != 0)
    cbgp_jni_throw_CBGPException(jEnv, "could not rescan domain");

  jni_unlock(jEnv);
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
  jni_lock(jEnv);

  if (simulator_run(network_get_simulator()) != 0)
    cbgp_jni_throw_CBGPException(jEnv, "simulation error");

  jni_unlock(jEnv);
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
  jni_lock(jEnv);

  if (simulator_step(network_get_simulator(), jiNumSteps) != 0)
    cbgp_jni_throw_CBGPException(jEnv, "simulation error");

  jni_unlock(jEnv);
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
  jni_lock(jEnv);

  //simulator_clear(network_get_simulator());

  jni_unlock(jEnv);
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
  jlong jlResult;

  jni_lock(jEnv);

  jlResult= (jlong) simulator_get_num_events(network_get_simulator());

  return_jni_unlock(jEnv, jlResult); 
}

// -----[ simGetEvent ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simGetEvent
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/Message
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simGetEvent
(JNIEnv * jEnv, jobject joCBGP, jint i)
{
  SNetSendContext * pCtx;
  SNetMessage * pMsg;
  jobject joMessage;

  jni_lock(jEnv);

  // Note: we assume that only network events (SNetSendContext) are
  // stored in the queue.
  pCtx= (SNetSendContext*) simulator_get_event(network_get_simulator(),
					       (unsigned int) i);
  pMsg= pCtx->pMessage;
  

  // Create new Message instance.
  if ((joMessage= cbgp_jni_new_BGPMessage(jEnv, pMsg)) == NULL)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joMessage);
}

// -----[ runCmd ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runCmd
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runCmd
  (JNIEnv * jEnv, jobject obj, jstring jsCommand)
{
  char * cCommand;

  if (jsCommand == NULL)
    return;

  jni_lock(jEnv);

  cCommand= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsCommand, NULL);
  if (libcbgp_exec_cmd(cCommand) != CLI_SUCCESS)
    cbgp_jni_throw_CBGPException(jEnv, "could not execute command");
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsCommand, cCommand);

  jni_unlock(jEnv);
}

// -----[ runScript ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runScript
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runScript
  (JNIEnv * jEnv, jobject obj, jstring jsFileName)
{
  char * pcFileName;

  pcFileName= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);

  jni_lock(jEnv);

  if (libcbgp_exec_file(pcFileName) != CLI_SUCCESS)
    cbgp_jni_throw_CBGPException(jEnv, "could not execute script");
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, pcFileName);

  jni_unlock(jEnv);
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
  char * pcPrompt;
  jstring jsPrompt;

  jni_lock(jEnv);

  pcPrompt= cli_context_to_string(cli_get()->pCtx, "cbgp");
  jsPrompt= cbgp_jni_new_String(jEnv, pcPrompt);

  return_jni_unlock(jEnv, jsPrompt);
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
  return cbgp_jni_new_String(jEnv, PACKAGE_VERSION);
}

// -----[ _cbgp_jni_routes_list_function ]----------------------------
static int _cbgp_jni_routes_list_function(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joRoute;

  if ((joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, *(SRoute **) pItem)) == NULL)
    return -1;

  // TODO: if there is a large number of objects, we should remove the
  // reference to the route as soon as it is added to the Vector
  // object. If we keep a large number of references to Java
  // instances, the performance will quickly decrease! This probably
  // apply to other places in the JNI code where a large number of
  // objects is returned in a single collection...
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

  pcFileName= (char *) (*env)->GetStringUTFChars(env, jsFileName, NULL);
  if ((pRoutes= bgp_routes_load_list(pcFileName, BGP_ROUTES_INPUT_MRT_ASC)) != NULL) {
    joArrayList= cbgp_jni_new_ArrayList(env);
    sCtx.jEnv= env;
    sCtx.joVector= joArrayList;
    if (routes_list_for_each(pRoutes, _cbgp_jni_routes_list_function, &sCtx) != 0)
      joArrayList= NULL;
  }
  (*env)->ReleaseStringUTFChars(env, jsFileName, pcFileName);

  // TODO: clean whole list !!! (routes_list are only references...)
  routes_list_destroy(&pRoutes);

  return joArrayList;
}

// -----[ _bgp_msg_listener ]----------------------------------------
static void _bgp_msg_listener(SNetMessage * pMessage, void * pContext)
{
  SJNIListener * pListener= (SJNIListener *) pContext;
  JavaVM * jVM= pListener->jVM;
  JNIEnv * jEnv= NULL;
  void * pTmp= &jEnv; // Need this unless GCC will complain about the
		      // typecast (void **) &jEnv
  jobject joMessage= NULL;

  // Attach the current thread to the Java VM (and get a pointer to
  // the JNI environment). This is required since the calling thread
  // might be different from the one that initialized the related
  // console listener object.
  if ((*jVM)->AttachCurrentThread(jVM, pTmp, NULL) != 0) {
    fprintf(stderr, "ERROR: AttachCurrentThread failed in JNI::_console_listener\n");
    return;
  }

  // Create new Message instance.
  if ((joMessage= cbgp_jni_new_BGPMessage(jEnv, pMessage)) == NULL)
    return;

  // Call the listener's method.
  cbgp_jni_call_void(jEnv, pListener->joListener,
		     "handleMessage",
		     "(Lbe/ac/ucl/ingi/cbgp/net/Message;)V",
		     joMessage);
}

// -----[ setBGPMsgListener ]----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    setBGPMsgListener
 * Signature: (Lbe/ac/ucl/ingi/cbgp/BGPMsgListener;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_setBGPMsgListener
  (JNIEnv * jEnv , jobject joCBGP, jobject joListener)
{
  jni_lock(jEnv);

  jni_listener_set(&sBGPListener, jEnv, joListener);
  bgp_router_set_msg_listener(_bgp_msg_listener, &sBGPListener);

  jni_unlock(jEnv);
}
