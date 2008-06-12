// ==================================================================
// @(#)net_IPTrace.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/09/2007
// $Id: net_IPTrace.c,v 1.1 2008-06-12 09:29:10 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <jni/impl/net_IPTrace.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/net_Subnet.h>

#define BASE "be/ac/ucl/ingi/cbgp/";

#define CLASS_IPTrace "be/ac/ucl/ingi/cbgp/IPTrace"
#define CONSTR_IPTrace "(Lbe/ac/ucl/ingi/cbgp/net/Node;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;I)V"
#define METHOD_IPTrace_append "(Lbe/ac/ucl/ingi/cbgp/IPTraceElement;)V"

#define CLASS_IPTraceElement "be/ac/ucl/ingi/cbgp/IPTraceElement"
#define CLASS_IPTraceElementNode "be/ac/ucl/ingi/cbgp/IPTraceNode"
#define CONSTR_IPTraceElementNode "(Lbe/ac/ucl/ingi/cbgp/net/Node;" \
  "Lbe/ac/ucl/ingi/cbgp/net/Interface;" \
  "Lbe/ac/ucl/ingi/cbgp/net/Interface;)V"

#define CLASS_IPTraceElementSubnet "be/ac/ucl/ingi/cbgp/IPTraceSubnet"
#define CONSTR_IPTraceElementSubnet "(Lbe/ac/ucl/ingi/cbgp/net/Subnet;)V"

// -----[ _new_IPTraceElementNode ]----------------------------------
static inline jobject _new_IPTraceElementNode(JNIEnv * jEnv,
					      net_node_t * node,
					      net_iface_t * iif,
					      net_iface_t * oif)
{
  jobject joNode= NULL, joInIface= NULL, joOutIface= NULL;

  if (node != NULL) {
    joNode= cbgp_jni_new_net_Node(jEnv, NULL, node);
    if (joNode == NULL)
      return NULL;
    if (iif != NULL) {
      joInIface= cbgp_jni_new_net_Interface(jEnv, NULL, iif);
      if (joInIface == NULL)
	return NULL;
    }
    if (oif != NULL) {
      joOutIface= cbgp_jni_new_net_Interface(jEnv, NULL, oif);
      if (joOutIface == NULL)
	return NULL;
    }
  }

  return cbgp_jni_new(jEnv, CLASS_IPTraceElementNode,
		      CONSTR_IPTraceElementNode, joNode,
		      joInIface, joOutIface);
}

// -----[ _new_IPTraceElementSubnet ]--------------------------------
static inline jobject _new_IPTraceElementSubnet(JNIEnv * jEnv,
						net_subnet_t * subnet)
{
  jobject joSubnet= cbgp_jni_new_net_Subnet(jEnv, NULL, subnet);
  if (joSubnet == NULL)
    return NULL;

  return cbgp_jni_new(jEnv, CLASS_IPTraceElementSubnet,
		      CONSTR_IPTraceElementSubnet, joSubnet);
}

// -----[ _new_IPTraceElement ]--------------------------------------
static inline jobject _new_IPTraceElement(JNIEnv * jEnv,
					  ip_trace_item_t * item,
					  int * hop_count)
{
  jobject element= NULL;
  jclass class;
  jfieldID field;
  int hop_count_val= *hop_count;

  switch (item->elt.type) {
  case NODE:
    element= _new_IPTraceElementNode(jEnv,
				     item->elt.node,
				     item->iif,
				     item->oif);
    (*hop_count)++;
    break;
  case SUBNET:
    element= _new_IPTraceElementSubnet(jEnv, item->elt.subnet);
    break;
  default:
    fatal("invalid element type in IP Trace");
  }
  if ((*jEnv)->ExceptionOccurred(jEnv))
    return NULL;

  /* Initialize "hop_count" field value (direct access) */
  class= (*jEnv)->FindClass(jEnv, CLASS_IPTraceElement);
  if ((class == NULL) || (*jEnv)->ExceptionOccurred(jEnv))
    jni_abort(jEnv, "Could not get id of class \"IPTraceElement\"");

  field= (*jEnv)->GetFieldID(jEnv, class, "hop_count", "I");
  if ((field == NULL) || (*jEnv)->ExceptionOccurred(jEnv))
    jni_abort(jEnv, "Could not get id of field \"hop_count\" in class "
	      "\"IPTraceElement\"");
  
  (*jEnv)->SetIntField(jEnv, element, field, hop_count_val);
  if ((*jEnv)->ExceptionOccurred(jEnv))
    jni_abort(jEnv, "Could not get id of field \"hop_count\" in class "
	      "\"IPTraceElement\"");

  return element;
}

// -----[ _IPTrace_append ]------------------------------------------
static inline int _IPTrace_append(JNIEnv * jEnv, jobject joTrace,
				  jobject joElement)
{
  return cbgp_jni_call_void(jEnv, joTrace, "append",
			    METHOD_IPTrace_append,
			    joElement);
}

// -----[ cbgp_jni_new_IPTrace ]-------------------------------------
/**
 *
 */
jobject cbgp_jni_new_IPTrace(JNIEnv * jEnv,
			     net_node_t * pNode,
			     net_addr_t tDst,
			     ip_trace_t * trace)
{
  jobject joTrace;
  jobject joSrc;
  jobject joDst;
  jobject joElement;
  unsigned int index;
  ip_trace_item_t * trace_item;
  int hop_count= 0;

  // Convert src/dst to Java objects
  if ((joSrc= cbgp_jni_new_net_Node(jEnv, NULL, pNode)) == NULL)
    return NULL;
  if ((joDst= cbgp_jni_new_IPAddress(jEnv, tDst)) == NULL)
    return NULL;

  // Create new IPTrace object
  if ((joTrace= cbgp_jni_new(jEnv, CLASS_IPTrace, CONSTR_IPTrace,
			     joSrc, joDst, (jint) trace->status))
      == NULL)
    return NULL;

  // Add hops
  for (index= 0; index < ip_trace_length(trace); index++) {
    trace_item= ip_trace_item_at(trace, index);
    joElement= _new_IPTraceElement(jEnv, trace_item, &hop_count);
    if (joElement == NULL)
      return NULL;
    if (_IPTrace_append(jEnv, joTrace, joElement))
      return NULL;
  }

  return joTrace;
}
