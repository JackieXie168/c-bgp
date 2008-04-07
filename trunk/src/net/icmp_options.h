// ==================================================================
// @(#)icmp_options.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/0/2008
// @lastdate 02/04/2008
// ==================================================================

#ifndef __NET_ICMP_OPTIONS_H__
#define __NET_ICMP_OPTIONS_H__

#include <net/error.h>
#include <net/net_types.h>
#include <net/routing.h>

#define ICMP_RR_OPTION_DELAY      0x01 /* Record total delay */
#define ICMP_RR_OPTION_WEIGHT     0x02 /* Record total weight */
#define ICMP_RR_OPTION_CAPACITY   0x04 /* Record max capacity */
#define ICMP_RR_OPTION_LOAD       0x08 /* Load traversed interfaces */
#define ICMP_RR_OPTION_LOOPBACK   0x10
#define ICMP_RR_OPTION_DEFLECTION 0x20 /* Check for deflection */
#define ICMP_RR_OPTION_QUICK_LOOP 0x40 /* Check for forwarding loops */
#define ICMP_RR_OPTION_ALT_DEST   0x80 /* Use alternate destination */

typedef enum {
  ICMP_OPT_STATE_INCOMING, /* Node and incoming iface are known (msg rcvd) */
  ICMP_OPT_STATE_OUTGOING, /* Outgoing iface is known (after lookup) */
  ICMP_OPT_STATE_ENCAP,    /* Probe packet is encapsulated */
  ICMP_OPT_STATE_DECAP,    /* Probe packet is decapsulated */
} icmp_opt_state_t;

typedef struct icmp_msg_opt_t {
  ip_trace_t * trace;    /* Record-route trace */
  uint8_t      options;  /* Record-route options */
  SPrefix      alt_dest; /* Alternate destination for lookup */
} icmp_msg_opt_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ icmp_process_options ]-------------------------------------
  /**
   * Process the options of an ICMP message. The function can be
   * invoked at different times of the IP processing (states):
   *   1). when the message is received by the IP forwarding process;
   *       in this case, the state is ICMP_OPT_STATE_INCOMING
   *   2). when the message is send by the IP forwarding process;
   *       in this case, the state is ICMP_OPT_STATE_OUTGOING
   *
   * The parameters are as follows:
   *   - node   : is the node where the IP processing occurs
   *   - iface  : is the incoming/outgoing interface (depending on
   *              state)
   *   - msg    : is the ICMP message
   *   - rtentry: is a reference to the current routing table entry;
   *              (it can be modified in order to perform RT lookups
   *              based on exact prefix matching)
   */
  net_error_t icmp_process_options(icmp_opt_state_t state,
				   net_node_t * node,
				   net_iface_t * iface,
				   net_msg_t * msg,
				   const rt_entry_t ** rtentry);

#ifdef __cplusplus
}
#endif

static inline void icmp_options_init(uint8_t * options) {
  *options= 0;
}

static inline void icmp_options_add(uint8_t * options, uint8_t option) {
  *options |= option;
}

#endif /* __NET_ICMP_OPTIONS_H__ */

