// ==================================================================
// @(#)bgp_filter.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 27/02/2008
// @lastdate 27/02/2008
// ==================================================================

#ifndef __CLI_BGP_FILTER_H__
#define __CLI_BGP_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_ctx_create_bgp_filter_add_rule -----------------------
  int cli_ctx_create_bgp_filter_add_rule(SCliContext * pContext,
					 void ** ppItem);
  // ----- cli_ctx_create_bgp_filter_insert_rule --------------------
  int cli_ctx_create_bgp_filter_insert_rule(SCliContext * pContext,
					    void ** ppItem);
  // ----- cli_bgp_filter_remove_rule -------------------------------
  int cli_bgp_filter_remove_rule(SCliContext * pContext,
				 SCliCmd * pCmd);
  // ----- cli_ctx_destroy_bgp_filter_rule --------------------------
  void cli_ctx_destroy_bgp_filter_rule(void ** ppItem);
  // ----- cli_bgp_filter_show---------------------------------------
  int cli_bgp_filter_show(SCliContext * pContext, SCliCmd * pCmd);
  // -----[ cli_register_bgp_filter_rule ]---------------------------
  SCliCmds * cli_register_bgp_filter_rule();

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_FILTER_H__ */
