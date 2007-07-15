// ==================================================================
// @(#)predicate_parser.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/02/2004
// @lastdate 24/04/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/log.h>
#include <libgds/memory.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif

#include <bgp/filter.h>
#include <bgp/filter_registry.h>

int predicate_parse(char ** ppcExpr, SFilterMatcher ** ppMatcher);

// ----- predicate_parse_sub_atom -----------------------------------
/**
 *
 */
int predicate_parse_sub_atom(char ** ppcExpr, SFilterMatcher ** ppMatcher)
{
  char * pcPos;
  char *pcSubExpr;
  char *pcTmpExpr;
  int iLen;
  int iError;

  pcTmpExpr= *ppcExpr;
  while (*pcTmpExpr && ((pcPos= strchr(")&|", *pcTmpExpr)) == NULL)) {
    if (*pcTmpExpr == '\"') {
      pcTmpExpr++;
      while (*pcTmpExpr && *pcTmpExpr != '\"')
	pcTmpExpr++;
      if (!*pcTmpExpr)
	return -1;
    }
    pcTmpExpr++;
  }

  iLen= pcTmpExpr-(*ppcExpr);

  pcSubExpr= (char *) MALLOC(sizeof(char)*((iLen)+1));
  strncpy(pcSubExpr, *ppcExpr, iLen);
  pcSubExpr[iLen]= 0;
  pcTmpExpr= pcSubExpr;
  iError= ft_registry_predicate_parse(pcTmpExpr, ppMatcher);
  FREE(pcSubExpr);

  (*ppcExpr)+= iLen;

  return iError;
}

// ----- predicate_sub_expr -----------------------------------------
/**
 *
 */
int predicate_parse_sub_expr(char ** ppcExpr, SFilterMatcher ** ppMatcher)
{
  char * pcPos;
  char *pcSubExpr;
  char *pcTmpExpr;
  int iLen;
  int iError;

  pcPos = *ppcExpr;
  while (*pcPos && (strchr(")", *pcPos) == NULL)) {
    if (*pcPos == '\"') {
      pcPos++;
      while (*pcPos && *pcPos != '\"') {
	pcPos++;
      }
      if (!*pcPos)
	return -1;
    }
    pcPos++;
  }

  if (pcPos == NULL)
    return -1;

  iLen= pcPos-(*ppcExpr);

  pcSubExpr= (char *) MALLOC(sizeof(char)*((iLen)+1));
  strncpy(pcSubExpr, *ppcExpr, iLen);
  pcSubExpr[iLen]= 0;
  pcTmpExpr= pcSubExpr;
  iError= predicate_parse(&pcTmpExpr, ppMatcher);
  FREE(pcSubExpr);
  (*ppcExpr)+= iLen+1;

  return iError;
}

// ----- predicate_parse --------------------------------------------
/**
 * 
 */
int predicate_parse(char ** ppcExpr, SFilterMatcher ** ppMatcher)
{
  char * pcExpr= *ppcExpr;
  char cCurrent;
  SFilterMatcher * pMatcher1= NULL;
  SFilterMatcher * pMatcher2= NULL;
  int iError= 0;

  while (*pcExpr != 0) {
    cCurrent= *pcExpr;
    switch (cCurrent) {
    case '!':
      pcExpr++;
      iError= predicate_parse(&pcExpr, &pMatcher1);
      pMatcher1= filter_match_not(pMatcher1);
      break;
    case '(':
      pcExpr++;
      iError= predicate_parse_sub_expr(&pcExpr, &pMatcher1);
      //iError= predicate_parse(&pcExpr, &pMatcher1);
      break;
    case '&':
      pcExpr++;
      iError= predicate_parse(&pcExpr, &pMatcher2);
      pMatcher1= filter_match_and(pMatcher1, pMatcher2);
      break;
    case '|':
      pcExpr++;
      iError= predicate_parse(&pcExpr, &pMatcher2);
      pMatcher1= filter_match_or(pMatcher1, pMatcher2);
      break;
    case ' ':
    case '\t':
      pcExpr++;
      break;
    default:
      iError= predicate_parse_sub_atom(&pcExpr, &pMatcher1);

    } 
    if (iError) break;
  }

  *ppcExpr= pcExpr;
  *ppMatcher= pMatcher1;
  
  return iError;
}

// ----- main -------------------------------------------------------
/**
 *
 */
void predicate_parser_test()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
  char * pcMatcher;
  SFilterMatcher * pMatcher;

  while ((pcMatcher= readline("# ")) != NULL) {

    if (predicate_parse(&pcMatcher, &pMatcher)) {
      log_printf(pLogOut, "error :(\n");
    } else {
      log_printf(pLogOut, "filter: ");
      filter_matcher_dump(pLogOut, pMatcher);
      log_printf(pLogOut, "\n");

      filter_matcher_destroy(&pMatcher);
    }

  }
  log_printf(pLogOut, "\n");
#endif
}
