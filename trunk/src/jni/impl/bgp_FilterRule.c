// ==================================================================
// @(#)bgp_FilterRule.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/05/2008
// $Id: bgp_FilterRule.c,v 1.1 2008-06-11 15:21:52 bqu Exp $
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

#define CLASS_BGPFilterRule "be/ac/ucl/ingi/cbgp/bgp/FilterRule"
#define CONSTR_BGPFilterRule "(Lbe/ac/ucl/ingi/cbgp/CBGP;)V"

// -----[ cbgp_jni_new_bgp_FilterRule ]------------------------------
/**
 * This function creates a new instance of the bgp.FilterRule object
 * from a CBGP filter rule.
 */
jobject cbgp_jni_new_bgp_FilterRule(JNIEnv * jEnv, jobject joCBGP,
				    bgp_ft_rule_t * rule)
{
  jobject joRule;

  /* Java proxy object already existing ? */
  joRule= jni_proxy_get(jEnv, rule);
  if (joRule != NULL)
    return joRule;

  /* Create new Filter object */
  if ((joRule= cbgp_jni_new(jEnv, CLASS_BGPFilterRule, CONSTR_BGPFilterRule,
			    joCBGP)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joRule, rule);

  return joRule;
}
