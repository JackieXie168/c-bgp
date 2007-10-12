// ==================================================================
// @(#)jni_interface.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 12/10/2007
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

// -----[ _jni_log_enter ]-------------------------------------------
#ifdef JNI_LOG
static inline void _jni_log_enter(const char * pcFunction)
{
  fprintf(stderr, "debug: enter %s\n", pcFunction);
}
#endif /* JNI_LOG */

// -----[ _jni_log_leave ]-------------------------------------------
#ifdef JNI_LOG
static inline void _jni_log_leave(const char * pcFunction)
{
  fprintf(stderr, "debug: leave %s\n", pcFunction);
}
#endif /* JNI_LOG */

#ifdef JNI_LOG
#define JNI_LOG_ENTER _jni_log_enter(__func__)
#else
#define JNI_LOG_ENTER
#endif /* JNI_LOG */

#ifdef JNI_LOG
#define JNI_LOG_LEAVE _jni_log_leave(__func__)
#else
#define JNI_LOG_LEAVE
#endif /* JNI_LOG */

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
  JNI_LOG_ENTER;
  jni_lock(jEnv);

  // Test if a C-BGP JNI session has already been created.
  if (joGlobalCBGP != NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "CBGP class already initialized");
    JNI_LOG_LEAVE;
    return_jni_unlock2(jEnv);
  }

  // Create global reference
  joGlobalCBGP= (*jEnv)->NewGlobalRef(jEnv, joCBGP);

  // Initialize C-BGP library (but not the GDS library)
  libcbgp_init2();
  _jni_proxies_init();

  // Initialize console listeners contexts
  jni_listener_init(&sConsoleOutListener);
  jni_listener_init(&sConsoleErrListener);
  jni_listener_init(&sBGPListener);

  fprintf(stdout, "C-BGP JNI library started.\n");
  fflush(stdout);

  jni_unlock(jEnv);
  JNI_LOG_LEAVE;
}

// -----[ destroy ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_destroy
  (JNIEnv * jEnv, jobject joCBGP)
{
  JNI_LOG_ENTER;
  jni_lock(jEnv);

  // Test if a C-BGP JNI session has already been created.
  if (joGlobalCBGP == NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "CBGP class already initialized");
    JNI_LOG_LEAVE;
    return_jni_unlock2(jEnv);
  }

  // Delete global references to console listeners
  bgp_router_set_msg_listener(NULL, NULL);
  log_destroy(&pLogOut);
  log_destroy(&pLogErr);
  pLogOut= log_create_stream(stdout);
  pLogErr= log_create_stream(stderr);
  jni_listener_unset(&sConsoleOutListener, jEnv);
  jni_listener_unset(&sConsoleErrListener, jEnv);
  jni_listener_unset(&sBGPListener, jEnv);

  // Invalidates all the C->Java mappings
  _jni_proxies_invalidate();

  // Free simulator data structures (but do not terminate GDS).
  libcbgp_done2();

  // Remove global reference
  (*jEnv)->DeleteGlobalRef(jEnv, joGlobalCBGP);
  joGlobalCBGP= NULL;

  fprintf(stdout, "C-BGP JNI library stopped.\n");
  fflush(stdout);

  jni_unlock(jEnv);
  JNI_LOG_LEAVE;
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

  libcbgp_set_debug_level(iLevel);
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
  SNetLink * pLink;
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

  pLink= node_find_link_ptp(pNodeSrc, pNodeDst->tAddr);
  joLink= cbgp_jni_new_net_Link(jEnv, joCBGP, pLink);

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


/////////////////////////////////////////////////////////////////////
// Simulation queue management
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


/////////////////////////////////////////////////////////////////////
// Miscellaneous methods
/////////////////////////////////////////////////////////////////////

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

// -----[ getErrorMsg ]----------------------------------------------
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_getErrorMsg
  (JNIEnv * jEnv, jobject joCBGP, jint iErrorCode)
{
  return cbgp_jni_net_error_str(jEnv, iErrorCode);
}


/////////////////////////////////////////////////////////////////////
// Experimental features
/////////////////////////////////////////////////////////////////////

// -----[ _cbgp_jni_routes_list_function ]----------------------------
static int _cbgp_jni_routes_list_function(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joRoute;
  int iResult;

  if ((joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, *(SRoute **) pItem)) == NULL)
    return -1;

  // Add the route object to the list
  iResult= cbgp_jni_ArrayList_add(pCtx->jEnv, pCtx->joVector, joRoute);

  // if there is a large number of objects, we should remove the
  // reference to the route as soon as it is added to the Vector
  // object. If we keep a large number of references to Java
  // instances, the performance will quickly decrease!
  (*(pCtx->jEnv))->DeleteLocalRef(pCtx->jEnv, joRoute);

  return iResult;
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


/////////////////////////////////////////////////////////////////////
//
// JNI Library Load/UnLoad
//
/////////////////////////////////////////////////////////////////////

/**
 * The VM calls JNI_OnLoad when the native library is loaded (for
 * example, through System.loadLibrary). JNI_OnLoad must return the
 * JNI version needed by the native library. In order to use any of
 * the new JNI functions, a native library must export a JNI_OnLoad
 * function that returns JNI_VERSION_1_2. If the native library does
 * not export a JNI_OnLoad function, the VM assumes that the library
 * only requires JNI version JNI_VERSION_1_1. If the VM does not
 * recognize the version number returned by JNI_OnLoad, the native
 * library cannot be loaded.
 */

// -----[ JNI_OnLoad ]-----------------------------------------------
/**
 * This function is called when the JNI library is loaded.
 */
jint JNI_OnLoad(JavaVM * jVM, void *reserved)
{
  JNIEnv * jEnv= NULL;

  fprintf(stdout, "C-BGP JNI library loaded.\n");
  fflush(stdout);

#ifdef __MEMORY_DEBUG__
  gds_init(GDS_OPTION_MEMORY_DEBUG);
#else
  gds_init(0);
#endif

  // Get JNI environment
  if ((*jVM)->GetEnv(jVM, (void **) &jEnv, JNI_VERSION_1_2) != JNI_OK)
    abort();

  // Init locking system
  _jni_lock_init(jEnv);

  return JNI_VERSION_1_2;
}

// -----[ JNI_OnUnload ]---------------------------------------------
/**
 * This function is called when the JNI library is unloaded.
 */
void JNI_OnUnload(JavaVM * jVM, void *reserved)
{
  JNIEnv * jEnv= NULL;

  fprintf(stdout, "C-BGP JNI library unloaded.\n");
  fflush(stdout);

  // Get JNI environment
  if ((*jVM)->GetEnv(jVM, (void **) &jEnv, JNI_VERSION_1_2) != JNI_OK)
    abort();

  // Terminate C<->Java mappings and locking system
  _jni_proxies_destroy();
  _jni_lock_destroy(jEnv);

  gds_destroy();
}


