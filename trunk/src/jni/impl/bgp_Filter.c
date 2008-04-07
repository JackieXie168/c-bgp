// ==================================================================
// @(#)bgp_Filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/04/2006
// @lastdate 12/03/2008
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

#include <bgp/mrtd.h>
#include <bgp/peer.h>

#define CLASS_BGPFilter "be/ac/ucl/ingi/cbgp/bgp/Filter"
#define CONSTR_BGPFilter "(Lbe/ac/ucl/ingi/cbgp/CBGP;)V"

// -----[ cbgp_jni_new_bgp_Filter ]----------------------------------
/**
 * This function creates a new instance of the bgp.Filter object from
 * a CBGP filter.
 */
jobject cbgp_jni_new_bgp_Filter(JNIEnv * jEnv, jobject joCBGP,
				bgp_filter_t * pFilter)
{
  jobject joFilter;

  /* Java proxy object already existing ? */
  joFilter= jni_proxy_get(jEnv, pFilter);
  if (joFilter != NULL)
    return joFilter;

  /* Create new Filter object */
  if ((joFilter= cbgp_jni_new(jEnv, CLASS_BGPFilter, CONSTR_BGPFilter,
			      joCBGP)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joFilter, pFilter);

  return joFilter;
}

// -----[ getRules ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Filter
 * Method:    getRules
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Filter_getRules
  (JNIEnv * jEnv, jobject joFilter)
{
  bgp_filter_t * pFilter;
  SJNIContext sCtx;

  jni_lock(jEnv);

  pFilter= (bgp_filter_t *) jni_proxy_lookup(jEnv, joFilter);
  if (pFilter == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joFilter)*/;
  if ((sCtx.joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, sCtx.joVector);
}
