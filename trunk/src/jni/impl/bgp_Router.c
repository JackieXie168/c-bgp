// ==================================================================
// @(#)bgp_Router.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/04/2006
// @lastdate 25/04/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_Router.h>

#include <bgp/as.h>
#include <bgp/route.h>

#define CLASS_BGPRouter "be/ac/ucl/ingi/cbgp/bgp/Router"
#define CONSTR_BGPRouter "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                         "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
                         "SLbe/ac/ucl/ingi/cbgp/IPAddress;" \
                         "Ljava/lang/String;)V"

// -----[ cbgp_jni_new_bgp_Router ]-----------------------------------
/**
 * This function creates a new instance of the bgp.Router object from a
 * BGP router.
 */
jobject cbgp_jni_new_bgp_Router(JNIEnv * jEnv, jobject joCBGP,
				SBGPRouter * pRouter)
{
  jobject joIPAddress;
  jobject joRouterID;
  jobject joRouter;

  /* Convert router attributes to Java objects */
  if ((joIPAddress= cbgp_jni_new_IPAddress(jEnv, pRouter->pNode->tAddr)) == NULL)
    return NULL;

  /* Convert router attributes to Java objects */
  if ((joRouterID= cbgp_jni_new_IPAddress(jEnv, pRouter->tRouterID)) == NULL)
    return NULL;

  /* Create new BGPRouter object */
  if ((joRouter= cbgp_jni_new(jEnv, CLASS_BGPRouter, CONSTR_BGPRouter,
			      joCBGP,
			      joIPAddress,
			      (jshort) pRouter->uNumber,
			      joRouterID,
			      NULL)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joRouter, pRouter);

  return joRouter;
}

// -----[ _proxy_finalize ]------------------------------------------
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router__1proxy_1finalize
(JNIEnv * jEnv, jobject joObject)
{
  //jint jiHashCode= jni_Object_hashCode(jEnv, joObject);
  //fprintf(stderr, "JNI::net_Link__proxy_finalize [key=%d]\n", jiHashCode);
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
  SBGPRouter * pRouter;
  SPrefix sPrefix;

  pRouter= (SBGPRouter *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return NULL;

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return NULL;

  if (bgp_router_add_network(pRouter, sPrefix) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "coud not add network");
    return NULL;
  }

  return cbgp_jni_new_IPPrefix(jEnv, sPrefix);
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
  SBGPRouter * pRouter;
  SPrefix sPrefix;

  pRouter= (SBGPRouter *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return;

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return;

  if (bgp_router_del_network(pRouter, sPrefix) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "coud not remove network");
    return;
  }
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
  SBGPRouter * pRouter;
  SBGPPeer * pPeer;
  net_addr_t tPeerAddr;

  pRouter= (SBGPRouter *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return NULL;

  if (ip_jstring_to_address(jEnv, jsPeerAddr, &tPeerAddr) != 0)
    return NULL;

  pPeer= bgp_router_add_peer(pRouter, jiASNumber, tPeerAddr, 0);
  if (pPeer == NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "could not add peer");
    return NULL;
  }

  return cbgp_jni_new_bgp_Peer(jEnv, jni_proxy_get_CBGP(jEnv, joRouter),
			       pPeer);
}

// -----[ _cbgp_jni_get_peer ]----------------------------------------
static int _cbgp_jni_get_peer(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joPeer;

  if ((joPeer= cbgp_jni_new_bgp_Peer(pCtx->jEnv,
				     pCtx->joCBGP,
				     *((SPeer **) pItem))) == NULL)
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
  SBGPRouter * pRouter;
  jobject joVector;
  SJNIContext sCtx;

  pRouter= (SBGPRouter *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return NULL;

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return NULL;

  sCtx.joVector= joVector;
  sCtx.joCBGP= jni_proxy_get_CBGP(jEnv, joRouter);
  sCtx.jEnv= jEnv;
  if (bgp_router_peers_for_each(pRouter, _cbgp_jni_get_peer, &sCtx) != 0)
    return NULL;

  return joVector;
}

// -----[ _cbgp_jni_get_rib_route ]----------------------------------
static int _cbgp_jni_get_rib_route(uint32_t uKey, uint8_t uKeyLen,
				   void * pItem, void * pContext)
{
   SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joRoute;

  if ((joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, (SRoute *) pItem)) == NULL)
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
  (JNIEnv * jEnv, jobject joRouter, jstring jsPrefix)
{
  SBGPRouter * pRouter;
  jobject joVector;
  SPrefix sPrefix;
  SRoute * pRoute;
  jobject joRoute;

  /* Get the router instance */
  pRouter= (SBGPRouter *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return NULL;

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return NULL;

  if (jsPrefix == NULL) {

    /* For each route in the RIB, create a new BGPRoute object and add
       it to the Vector object */
    SJNIContext sCtx;
    sCtx.pRouter= pRouter;
    sCtx.joVector= joVector;
    sCtx.jEnv= jEnv;
    if (rib_for_each(pRouter->pLocRIB, _cbgp_jni_get_rib_route, &sCtx) != 0)
      return NULL;

  } else {

    /* Convert the prefix */
    if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
      return NULL;

    pRoute= NULL;
#ifndef __EXPERIMENTAL_WALTON__ 
    if (sPrefix.uMaskLen == 32)
      pRoute= rib_find_best(pRouter->pLocRIB, sPrefix);
    else
      pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif

    if (pRoute != NULL) {
      if ((joRoute= cbgp_jni_new_BGPRoute(jEnv, pRoute)) == NULL)
	return NULL;
      cbgp_jni_Vector_add(jEnv, joVector, joRoute);
    }

  }

  return joVector;
}

// -----[ _getNetworks ]---------------------------------------------
static int _getNetworks(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joNetwork;
  SRoute * pRoute= *((SRoute **) pItem);

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
  SBGPRouter * pRouter;
  jobject joVector;
  SJNIContext sCtx;

  /* Get the router instance */
  pRouter= (SBGPRouter *) jni_proxy_lookup(jEnv, joRouter);
  if (pRouter == NULL)
    return NULL;

  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return NULL;

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;
  sCtx.joCBGP= jni_proxy_get_CBGP(jEnv, joRouter);
  
  bgp_router_networks_for_each(pRouter, _getNetworks, &sCtx);

  return joVector;
}
