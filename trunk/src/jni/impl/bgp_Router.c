// ==================================================================
// @(#)bgp_Router.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// @lastdate 11/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni_md.h>
#include <jni.h>
#include <jni/exceptions.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_Router.h>
#include <jni/impl/net_Node.h>

#include <bgp/as.h>
#include <bgp/route.h>
#include <bgp/route-input.h>

#define CLASS_BGPRouter "be/ac/ucl/ingi/cbgp/bgp/Router"
#define CONSTR_BGPRouter "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                         "Lbe/ac/ucl/ingi/cbgp/net/Node;" \
                         "SLbe/ac/ucl/ingi/cbgp/IPAddress;)V"

// -----[ cbgp_jni_new_bgp_Router ]-----------------------------------
/**
 * This function creates a new instance of the bgp.Router object from a
 * BGP router.
 */
jobject cbgp_jni_new_bgp_Router(JNIEnv * jEnv, jobject joCBGP,
				bgp_router_t * pRouter)
{
  jobject joNode;
  jobject joRouterID;
  jobject joRouter;

  /* Java proxy object already existing ? */
  joRouter= jni_proxy_get(jEnv, pRouter);
  if (joRouter != NULL)
    return joRouter;

  /* Get underlying node */
  joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, pRouter->pNode);
  if (joNode == NULL)
    return NULL;

  /* Convert router attributes to Java objects */
  if ((joRouterID= cbgp_jni_new_IPAddress(jEnv, pRouter->tRouterID)) == NULL)
    return NULL;

  /* Create new BGPRouter object */
  if ((joRouter= cbgp_jni_new(jEnv, CLASS_BGPRouter, CONSTR_BGPRouter,
			      joCBGP,
			      joNode,
			      (jshort) pRouter->uASN,
			      joRouterID)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joRouter, pRouter);

  return joRouter;
}

// -----[ isRouteReflector ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    isRouteReflector
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_isRouteReflector
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * pRouter;

  jni_lock(jEnv);

  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  return_jni_unlock(jEnv, (pRouter->iRouteReflector != 0)?JNI_TRUE:JNI_FALSE);
}

// -----[ addNetwork ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    addNetwork
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPPrefix;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_addNetwork
  (JNIEnv * jEnv, jobject joRouter, jstring jsPrefix)
{
  bgp_router_t * pRouter;
  SPrefix sPrefix;
  jobject joPrefix;
  int iResult;

  jni_lock(jEnv);

  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return_jni_unlock(jEnv, NULL);

  iResult= bgp_router_add_network(pRouter, sPrefix);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not add network (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  joPrefix= cbgp_jni_new_IPPrefix(jEnv, sPrefix);

  return_jni_unlock(jEnv, joPrefix);
}

// -----[ delNetwork ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    delNetwork
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_delNetwork
  (JNIEnv * jEnv, jobject joRouter, jstring jsPrefix)
{
  bgp_router_t * pRouter;
  SPrefix sPrefix;

  jni_lock(jEnv);

  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return_jni_unlock2(jEnv);

  if (bgp_router_del_network(pRouter, sPrefix) != 0) {
    throw_CBGPException(jEnv, "coud not remove network");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}

// -----[ addPeer ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    addPeer
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_addPeer
  (JNIEnv * jEnv, jobject joRouter, jstring jsPeerAddr, jint jiASNumber)
{
  bgp_router_t * pRouter;
  bgp_peer_t * pPeer;
  net_addr_t tPeerAddr;
  jobject joPeer;

  jni_lock(jEnv);

  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_address(jEnv, jsPeerAddr, &tPeerAddr) != 0)
    return_jni_unlock(jEnv, NULL);

  if (bgp_router_add_peer(pRouter, jiASNumber, tPeerAddr, &pPeer) != 0) {
    throw_CBGPException(jEnv, "could not add peer");
    return_jni_unlock(jEnv, NULL);
  }

  joPeer= cbgp_jni_new_bgp_Peer(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joRouter)*/,
				pPeer);

  return_jni_unlock(jEnv, joPeer);
}

// -----[ _cbgp_jni_get_peer ]----------------------------------------
static int _cbgp_jni_get_peer(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joPeer;

  if ((joPeer= cbgp_jni_new_bgp_Peer(pCtx->jEnv,
				     pCtx->joCBGP,
				     *((bgp_peer_t **) pItem))) == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joPeer);
}

// -----[ getPeers ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    getPeers
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getPeers
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * pRouter;
  jobject joVector;
  SJNIContext sCtx;

  jni_lock(jEnv);

  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joRouter)*/;
  sCtx.jEnv= jEnv;
  if (bgp_router_peers_for_each(pRouter, _cbgp_jni_get_peer, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_rib_route ]----------------------------------
static int _cbgp_jni_get_rib_route(uint32_t uKey, uint8_t uKeyLen,
				   void * pItem, void * pContext)
{
   SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joRoute;

  joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, (bgp_route_t *) pItem, NULL);
  if (joRoute == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ getRIB ]---------------------------------------------------
/**
 * Class    : bgp.Router
 * Method   : getRIB
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 *
 * This function creates an array of BGPRoute objects corresponding
 * with the content of the given router's RIB.
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getRIB
  (JNIEnv * jEnv, jobject joRouter, jstring jsDest)
{
  bgp_router_t * pRouter;
  jobject joVector;
  bgp_route_t * pRoute;
  SJNIContext sCtx;
  SNetDest sDest;

  jni_lock(jEnv);

  /* Get the router instance */
  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination specifier (*|address|prefix) */
  if (jsDest != NULL) {
    if (ip_jstring_to_dest(jEnv, jsDest, &sDest) < 0)
      return_jni_unlock(jEnv, NULL);
  } else
    sDest.tType= NET_ADDR_ANY;

  /* Prepare the JNI Context environment */
  sCtx.pRouter= pRouter;
  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;

  switch (sDest.tType) {
  case NET_DEST_ANY:

    /* For each route in the RIB, create a new BGPRoute object and add
       it to the Vector object */
    if (rib_for_each(pRouter->pLocRIB, _cbgp_jni_get_rib_route, &sCtx) != 0)
      return_jni_unlock(jEnv, NULL);
    break;

  case NET_DEST_ADDRESS:
#ifndef __EXPERIMENTAL_WALTON__ 
    sDest.uDest.sPrefix.tNetwork= sDest.uDest.tAddr;
    sDest.uDest.sPrefix.uMaskLen= 32;
    pRoute= rib_find_best(pRouter->pLocRIB, sDest.uDest.sPrefix);
    if (pRoute != NULL)
      if (_cbgp_jni_get_rib_route(sDest.uDest.sPrefix.tNetwork,
				  32, pRoute, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
#endif

    break;

  case NET_DEST_PREFIX:
#ifndef __EXPERIMENTAL_WALTON__ 
    pRoute= rib_find_exact(pRouter->pLocRIB, sDest.uDest.sPrefix);
    if (pRoute != NULL)
      if (_cbgp_jni_get_rib_route(sDest.uDest.sPrefix.tNetwork,
				  sDest.uDest.sPrefix.uMaskLen,
				  pRoute, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
#endif
    break;

  default:
    abort();
  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_adj_rib_routes ]-----------------------------
static int _cbgp_jni_get_adj_rib_routes(bgp_peer_t * pPeer, SNetDest * pDest,
					int iIn, SJNIContext * pCtx)
{
  bgp_route_t * pRoute;
  bgp_rib_dir_t dir= (iIn?RIB_IN:RIB_OUT);

  switch (pDest->tType) {
  case NET_DEST_ANY:
    return rib_for_each(pPeer->pAdjRIB[dir],
			_cbgp_jni_get_rib_route, pCtx);

  case NET_DEST_ADDRESS:
    pDest->uDest.sPrefix.tNetwork= pDest->uDest.tAddr;
    pDest->uDest.sPrefix.uMaskLen= 32;

    pRoute= rib_find_best(pPeer->pAdjRIB[dir],
			  pDest->uDest.sPrefix);
    if (pRoute != NULL)
      return _cbgp_jni_get_rib_route(pDest->uDest.sPrefix.tNetwork,
				     32, pRoute, pCtx);
    break;
    
  case NET_DEST_PREFIX:
    pRoute= rib_find_exact(pPeer->pAdjRIB[dir],
			   pDest->uDest.sPrefix);
    if (pRoute != NULL)
      return _cbgp_jni_get_rib_route(pDest->uDest.sPrefix.tNetwork,
				     pDest->uDest.sPrefix.uMaskLen,
				     pRoute, pCtx);
    break;

  default:
    abort();
  }

  return 0;
}

// -----[ getAdjRIB ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    getAdjRIB
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getAdjRIB
  (JNIEnv * jEnv, jobject joRouter, jstring jsPeerAddr,
   jstring jsDest, jboolean bIn)
{
  bgp_router_t * pRouter;
  jobject joVector;
  net_addr_t tPeerAddr;
  unsigned int uIndex;
  bgp_peer_t * pPeer= NULL;
  SNetDest sDest;
  SJNIContext sCtx;

  jni_lock(jEnv);

  /* Get the router instance */
  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create a Vector object that will hold the BGP routes to be returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the peer address */
  if (jsPeerAddr != NULL) {
    if (ip_jstring_to_address(jEnv, jsPeerAddr, &tPeerAddr) != 0)
      return_jni_unlock(jEnv, NULL);
    if ((pPeer= bgp_router_find_peer(pRouter, tPeerAddr)) == NULL) {
      throw_CBGPException(jEnv, "unknown peer");
      return_jni_unlock(jEnv, NULL);
    }
  }

  /* Convert the destination specifier (*|address|prefix) */
  if (jsDest != NULL) {
    if (ip_jstring_to_dest(jEnv, jsDest, &sDest) < 0)
      return_jni_unlock(jEnv, NULL);
  } else
    sDest.tType= NET_ADDR_ANY;

  /* Prepare the JNI Context environment */
  sCtx.pRouter= pRouter;
  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;

  if (pPeer == NULL) {
    
    for (uIndex= 0; uIndex < ptr_array_length(pRouter->pPeers); uIndex++) {
      pPeer= (bgp_peer_t *) pRouter->pPeers->data[uIndex];
      if (_cbgp_jni_get_adj_rib_routes(pPeer, &sDest,
				       (bIn==JNI_TRUE), &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
    }

  } else {

    if (_cbgp_jni_get_adj_rib_routes(pPeer, &sDest,
				     (bIn==JNI_TRUE), &sCtx) < 0)
      return_jni_unlock(jEnv, NULL);

  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ _getNetworks ]---------------------------------------------
static int _getNetworks(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joNetwork;
  bgp_route_t * pRoute= *((bgp_route_t **) pItem);

  if ((joNetwork= cbgp_jni_new_IPPrefix(pCtx->jEnv,
					pRoute->sPrefix)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joNetwork);
}

// -----[ getNetworks ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    getNetworks
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getNetworks
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * pRouter;
  jobject joVector;
  SJNIContext sCtx;

  jni_lock(jEnv);

  /* Get the router instance */
  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joRouter)*/;
  
  bgp_router_networks_for_each(pRouter, _getNetworks, &sCtx);

  return_jni_unlock(jEnv, joVector);
}

// -----[ loadRib ]--------------------------------------------------
/*
 * Class:     bgp.Router
 * Method:    loadRib
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_loadRib
(JNIEnv * jEnv, jobject joRouter, jstring jsFileName, jboolean jbForce)
{
  char * cFileName;
  bgp_router_t * pRouter;
  uint8_t tFormat= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t tOptions= 0;

  jni_lock(jEnv);

  /* Get the router instance */
  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock2(jEnv);

  if (jbForce == JNI_TRUE) {
    tOptions|= BGP_ROUTER_LOAD_OPTIONS_FORCE;
    tOptions|= BGP_ROUTER_LOAD_OPTIONS_AUTOCONF;
  }

  tOptions|= BGP_ROUTER_LOAD_OPTIONS_SUMMARY;

  cFileName= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  if (bgp_router_load_rib(pRouter, (char *) cFileName,
			  tFormat, tOptions) != 0)
    throw_CBGPException(jEnv, "could not load RIB");
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, cFileName);

  jni_unlock(jEnv);
}

// -----[ rescan ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    rescan
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_rescan
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * pRouter;

  jni_lock(jEnv);

  /* Get the router instance */
  pRouter= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return_jni_unlock2(jEnv);

  if (bgp_router_scan_rib(pRouter) != 0)
    throw_CBGPException(jEnv, "could not rescan router");

  jni_unlock(jEnv);
}
