// ==================================================================
// @(#)predicate_parser.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/02/2004
// @lastdate 01/03/2004
// ==================================================================

#ifndef __PREDICATE_PARSER_H__
#define __PREDICATE_PARSER_H__

#include <bgp/filter.h>

// ----- predicate_parse --------------------------------------------
extern int predicate_parse(char ** ppcExpr,
			   SFilterMatcher ** ppMatcher);

extern void predicate_parser_test();

#endif
