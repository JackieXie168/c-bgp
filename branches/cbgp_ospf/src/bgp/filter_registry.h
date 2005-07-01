// ==================================================================
// @(#)filter_registry.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/03/2004
// @lastdate 17/05/2005
// ==================================================================

#ifndef __FILTER_REGISTRY_H__
#define __FILTER_REGISTRY_H__

#include <libgds/types.h>

#include <bgp/filter.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

// ----- ft_registry_predicate_get ----------------------------------
//extern SFtRegPredicate * ft_registry_predicate_get(uint8_t uID);
// ----- ft_registry_action_get -------------------------------------
//extern SFtRegAction * ft_registry_action_get(uint8_t uID);

// ----- ft_registry_predicate_parse ----------------------------
extern int ft_registry_predicate_parse(char * pcExpr,
				       SFilterMatcher ** ppMatcher);
// ----- ft_registry_action_parse -----------------------------------
extern int ft_registry_action_parse(char * pcExpr,
				    SFilterAction ** ppAction);

// ----- _ft_registry_init --------------------------------------
extern void _ft_registry_init();
// ----- _ft_registry_destroy -----------------------------------
extern void _ft_registry_destroy();

#endif
