// ==================================================================
// @(#)net_Link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 29/06/2007
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

  /* Java proxy object already existing ? */
  joLink= jni_proxy_get(jEnv, pLink);
  if (joLink != NULL)
    return joLink;

  /* Convert link attributes to Java objects */
  jobject obj_IPAddress=
    cbgp_jni_new_IPAddress(jEnv, net_link_get_address(pLink));

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
  jboolean jbState;

  jni_lock(jEnv);
  
  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jbState= net_link_get_state(pLink, NET_LINK_FLAG_UP)?JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jbState);
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

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock2(jEnv);
    
  net_link_set_state(pLink, NET_LINK_FLAG_UP, (bState == JNI_TRUE)?1:0);

  jni_unlock(jEnv);
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
  jlong jlResult;

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, 0);

  jlResult= (jlong) net_link_get_weight(pLink, 0);

  return_jni_unlock(jEnv, jlResult);
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

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock2(jEnv);

  net_link_set_weight(pLink, 0, (uint32_t) jlWeight);

  jni_unlock(jEnv);
}

