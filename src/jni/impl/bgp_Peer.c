// ==================================================================
// @(#)bgp_Peer.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/04/2006
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
#include <jni/impl/bgp_Filter.h>
#include <jni/impl/bgp_Peer.h>

#include <bgp/mrtd.h>
#include <bgp/peer.h>

#define CLASS_BGPPeer "be/ac/ucl/ingi/cbgp/bgp/Peer"
#define CONSTR_BGPPeer "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;IB)V"

// -----[ cbgp_jni_new_bgp_Peer ]------------------------------------
/**
 * This function creates a new instance of the bgp.Peer object from a
 * CBGP peer.
 */
jobject cbgp_jni_new_bgp_Peer(JNIEnv * jEnv, jobject joCBGP,
			      SBGPPeer * pPeer)
{
  jobject joIPAddress;
  jobject joPeer;

  /* Convert peer attributes to Java objects */
  if ((joIPAddress= cbgp_jni_new_IPAddress(jEnv, pPeer->tAddr)) == NULL)
    return NULL;

  /* Create new BGPPeer object */
  if ((joPeer= cbgp_jni_new(jEnv, CLASS_BGPPeer, CONSTR_BGPPeer,
			    joCBGP,
		            joIPAddress,
		            (jint) pPeer->uRemoteAS,
		            (jbyte) pPeer->uFlags)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joPeer, pPeer);

  return joPeer;
}

// -----[ _proxy_finalize ]------------------------------------------
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer__1proxy_1finalize
(JNIEnv * jEnv, jobject joObject)
{
  //jint jiHashCode= jni_Object_hashCode(jEnv, joObject);
  //fprintf(stderr, "JNI::net_Link__proxy_finalize [key=%d]\n", jiHashCode);
}

// -----[ getRouterID ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getRouterID
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/IPAddress;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getRouterID
  (JNIEnv * jEnv, jobject joObject)
{
  SBGPPeer * pPeer;

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return NULL;

  return cbgp_jni_new_IPAddress(jEnv, pPeer->tRouterID);
}

// -----[ getSessionState ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getSessionState
 * Signature: ()B
 */
JNIEXPORT jbyte JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getSessionState
  (JNIEnv * jEnv, jobject joObject)
{
  SBGPPeer * pPeer;
  
  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return JNI_FALSE;

  return pPeer->uSessionState;
}

// -----[ openSession ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    openSession
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_openSession
  (JNIEnv * jEnv, jobject joObject)
{
  SBGPPeer * pPeer;
  
  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return;
 
  bgp_peer_open_session(pPeer);
}

// -----[ openSession ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    closeSession
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_closeSession
  (JNIEnv * jEnv, jobject joObject)
{
  SBGPPeer * pPeer;
  
  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return;

  bgp_peer_close_session(pPeer);
}

// -----[ recv ]-----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    recv
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_recv
  (JNIEnv * jEnv, jobject joPeer, jstring jsMesg)
{
  SBGPPeer * pPeer;
  const char * cMesg;
  SBGPMsg * pMsg;

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return;

  /* Build a message from the MRT-record */
  cMesg= (*jEnv)->GetStringUTFChars(jEnv, jsMesg, NULL);
  if ((pMsg= mrtd_msg_from_line(pPeer->pLocalRouter, pPeer,
				(char *) cMesg)) != NULL) {
    if (bgp_peer_handle_message(pPeer, pMsg) != 0)
      cbgp_jni_throw_CBGPException(jEnv, "could not handle message");
  } else {
    cbgp_jni_throw_CBGPException(jEnv, "could not understand MRT message");
  }
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsMesg, cMesg);
}

// -----[ getNextHopSelf ]-------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getNextHopSelf
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getNextHopSelf
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return JNI_FALSE;

  return bgp_peer_flag_get(pPeer, PEER_FLAG_NEXT_HOP_SELF)?JNI_TRUE:JNI_FALSE;
}

// -----[ setNextHopSelf ]-------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    setNextHopSelf
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_setNextHopSelf
  (JNIEnv * jEnv, jobject joPeer, jboolean state)
{
  SBGPPeer * pPeer;

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return;

  bgp_peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF,
		    (state==JNI_TRUE)?1:0);
}

// -----[ getInputfilter ]-------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getInputFilter
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Filter;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getInputFilter
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;
  SFilter * pFilter;

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return NULL;

  if ((pFilter= bgp_peer_in_filter_get(pPeer)) == NULL)
    return NULL;

  return cbgp_jni_new_bgp_Filter(jEnv, jni_proxy_get_CBGP(jEnv, joPeer),
				 pFilter);
}

// -----[ getOutputFilter ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getOutputFilter
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Filter;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getOutputFilter
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;
  SFilter * pFilter;

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return NULL;

  if ((pFilter= bgp_peer_out_filter_get(pPeer)) == NULL)
    return NULL;

  return cbgp_jni_new_bgp_Filter(jEnv, jni_proxy_get_CBGP(jEnv, joPeer),
				 pFilter);
}
