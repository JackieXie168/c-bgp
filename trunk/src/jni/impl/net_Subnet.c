// ==================================================================
// @(#)net_Subnet.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/10/2007
// @lastdate 19/10/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <net/subnet.h>

#define CLASS_Subnet "be/ac/ucl/ingi/cbgp/net/Subnet"
#define CONSTR_Subnet "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPPrefix;)V"

// -----[ cbgp_jni_new_net_Subnet ]----------------------------------
/**
 * This function creates a new instance of the Node object from a CBGP
 * node.
 */
jobject cbgp_jni_new_net_Subnet(JNIEnv * jEnv, jobject joCBGP,
				net_subnet_t * pSubnet)
{
  jobject joSubnet;
  jobject joPrefix;

  /* Java proxy object already existing ? */
  joSubnet= jni_proxy_get(jEnv, pSubnet);
  if (joSubnet != NULL)
    return joSubnet;

  /* Convert node attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, pSubnet->sPrefix);
  if (joPrefix == NULL)
    return NULL;

  /* Create new Subnet object */
  if ((joSubnet= cbgp_jni_new(jEnv, CLASS_Subnet, CONSTR_Subnet,
			      joCBGP,
			      joPrefix)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joSubnet, pSubnet);

  return joSubnet;
}
