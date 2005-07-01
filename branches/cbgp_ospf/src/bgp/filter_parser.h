// ==================================================================
// @(#)filter_parser.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/08/2003
// @lastdate 01/03/2004
// ==================================================================

#ifndef __FILTER_PARSER_H__
#define __FILTER_PARSER_H__

#include <bgp/filter.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

// ----- filter_parser_rule -----------------------------------------
extern int filter_parser_rule(char * pcRule, SFilterRule ** ppRule);
// ----- filter_parser_run ------------------------------------------
/*
extern int filter_parser_run(char * pcScript, SFilter ** ppFilter);
*/
// ----- filter_parser_action ---------------------------------------
extern int filter_parser_action(char * pcAction,
				SFilterAction ** ppAction);

#endif
