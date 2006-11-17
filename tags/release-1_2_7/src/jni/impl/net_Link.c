// ==================================================================
// @(#)net_Link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
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
#include <jni/impl/net_Link.h>

#include <net/link.h>

#define CLASS_Link "be/ac/ucl/ingi/cbgp/net/Link"
#define CONSTR_Link "(Lbe/ac/ucl/ingi/cbgp/CBGP;"\
                    "Lbe/ac/ucl/ingi/cbgp/IPAddress;J)V"

// -----[ cbgp_jni_new_net_Link ]------------------------------------
/**
 * This function creates a new instance of the Link object from a CBGP
 * link.
 */
jobject cbgp_jni_new_net_Link(JNIEnv * jEnv, jobject joCBGP,
			      SNetLink * pLink)
{
  jobject joLink;

  /* Convert link attributes to Java objects */
  jobject obj_IPAddress= cbgp_jni_new_IPAddress(jEnv, link_get_address(pLink));

  /* Check that the conversion was successful */
  if (obj_IPAddress == NULL)
    return NULL;

  /* Create new Link object */
  if ((joLink= cbgp_jni_new(jEnv, CLASS_Link, CONSTR_Link,
			    joCBGP,
			    obj_IPAddress,
			    (jlong) pLink->tDelay)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joLink, pLink);
  return joLink;
}

// -----[ _proxy_finalize ]------------------------------------------
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link__1proxy_1finalize
(JNIEnv * jEnv, jobject joObject)
{
  //jint jiHashCode= jni_Object_hashCode(jEnv, joObject);
  //fprintf(stderr, "JNI::net_Link__proxy_finalize [key=%d]\n", jiHashCode);
}

// -----[ getState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getState
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getState
  (JNIEnv * jEnv, jobject joLink)
{
  SNetLink * pLink;
  
  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return JNI_FALSE;

  return link_get_state(pLink, NET_LINK_FLAG_UP)?JNI_TRUE:JNI_FALSE;
}

// -----[ setState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setState
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setState
  (JNIEnv * jEnv, jobject joLink, jboolean bState)
{
  SNetLink * pLink;

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return;
    
  link_set_state(pLink, NET_LINK_FLAG_UP, (bState == JNI_TRUE)?1:0);
}

// -----[ getWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getWeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getWeight
  (JNIEnv * jEnv, jobject joLink)
{
  SNetLink * pLink;

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return 0;

  return (jlong) pLink->uIGPweight;
}

// -----[ setWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setWeight
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setWeight
  (JNIEnv * jEnv, jobject joLink, jlong jlWeight)
{
  SNetLink * pLink;

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return;

  link_set_igp_weight(pLink, (uint32_t) jlWeight);
}

