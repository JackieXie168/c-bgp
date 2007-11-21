// ==================================================================
// @(#)bgp_Peer.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/04/2006
// @lastdate 05/10/2007
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
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;I)V"

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

  /* Java proxy object already existing ? */
  joPeer= jni_proxy_get(jEnv, pPeer);
  if (joPeer != NULL)
    return joPeer;

  /* Convert peer attributes to Java objects */
  if ((joIPAddress= cbgp_jni_new_IPAddress(jEnv, pPeer->tAddr)) == NULL)
    return NULL;

  /* Create new BGPPeer object */
  if ((joPeer= cbgp_jni_new(jEnv, CLASS_BGPPeer, CONSTR_BGPPeer,
			    joCBGP,
		            joIPAddress,
		            (jint) pPeer->uRemoteAS)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joPeer, pPeer);

  return joPeer;
}

// -----[ getRouterID ]----------------------------------------------
/**
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
  
  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  return_jni_unlock(jEnv, pPeer->uSessionState);
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

  jni_lock(jEnv);
  
  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);
 
  bgp_peer_open_session(pPeer);

  jni_unlock(jEnv);
}

// -----[ closeSession ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    closeSession
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_closeSession
  (JNIEnv * jEnv, jobject joObject)
{
  SBGPPeer * pPeer;

  jni_lock(jEnv);
  
  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joObject);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_close_session(pPeer);

  jni_unlock(jEnv);
}

// -----[ recv ]-----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    recv
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_recv
  (JNIEnv * jEnv, jobject joPeer, jstring jsMesg)
{
  SBGPPeer * pPeer;
  const char * cMesg;
  SBGPMsg * pMsg;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  /* Check that the peer is virtual */
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    cbgp_jni_throw_CBGPException(jEnv, "only virtual peers can do that");
    return_jni_unlock2(jEnv);
  }

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

  jni_unlock(jEnv);
}

// -----[ isInternal ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    isInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isInternal
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;
  jboolean jResult;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (pPeer->uRemoteAS == pPeer->pLocalRouter->uNumber)?JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}

// -----[ is ReflectorClient ]---------------------------------------
/**
 * Class:     bgp.Peer
 * Method:    isReflectorClient
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isReflectorClient
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;
  jboolean jResult;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}									       

// -----[ setReflectorClient ]---------------------------------------
/*
 * Class:     bgp.Peer
 * Method:    setReflectorClient
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_setReflectorClient
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  pPeer->pLocalRouter->iRouteReflector= 1;
  bgp_peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);

  jni_unlock(jEnv);
}

// -----[ isVirtual ]------------------------------------------------
/**
 * Class:     bgp.Peer
 * Method:    isVirtual
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isVirtual
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;
  jboolean jResult;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}

// -----[ setVirtual ]-----------------------------------------------
/*
 * Class:     bgp.Peer
 * Method:    setVirtual
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_setVirtual
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
  bgp_peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);

  jni_unlock(jEnv);
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
  jboolean jbNextHopSelf;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jbNextHopSelf= bgp_peer_flag_get(pPeer, PEER_FLAG_NEXT_HOP_SELF)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jbNextHopSelf);
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

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF,
		    (state==JNI_TRUE)?1:0);

  jni_unlock(jEnv);
}

// -----[ hasSoftRestart ]-------------------------------------------
/**
 * Class:     bgp.Peer
 * Method:    hasSoftRestart
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_hasSoftRestart
  (JNIEnv * jEnv, jobject joPeer)
{
  SBGPPeer * pPeer;
  jboolean jResult;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
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
  jobject joFilter;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((pFilter= bgp_peer_in_filter_get(pPeer)) == NULL)
    return_jni_unlock(jEnv, NULL);

  joFilter= cbgp_jni_new_bgp_Filter(jEnv, jni_proxy_get_CBGP(jEnv, joPeer),
				    pFilter);

  return_jni_unlock(jEnv, joFilter);
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
  jobject joFilter;

  jni_lock(jEnv);

  pPeer= (SBGPPeer *) jni_proxy_lookup(jEnv, joPeer);
  if (pPeer == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((pFilter= bgp_peer_out_filter_get(pPeer)) == NULL)
    return_jni_unlock(jEnv, NULL);

  joFilter= cbgp_jni_new_bgp_Filter(jEnv, jni_proxy_get_CBGP(jEnv, joPeer),
				    pFilter);

  return_jni_unlock(jEnv, joFilter);
}
