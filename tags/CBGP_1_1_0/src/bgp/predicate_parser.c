// ==================================================================
// @(#)predicate_parser.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/02/2004
// @lastdate 29/02/2004
// ==================================================================

#include <libgds/memory.h>
#include <stdio.h>

#include <readline/readline.h>

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
  while (*pcTmpExpr && ((pcPos= strchr(")&|", *pcTmpExpr)) == NULL))
    pcTmpExpr++;

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
  char * pcPos= strchr(*ppcExpr, ')');
  char *pcSubExpr;
  char *pcTmpExpr;
  int iLen;
  int iError;

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
  char * pcMatcher;
  SFilterMatcher * pMatcher;

  while ((pcMatcher= readline("# ")) != NULL) {

    if (predicate_parse(&pcMatcher, &pMatcher)) {
      fprintf(stdout, "error :(\n");
    } else {
      fprintf(stdout, "filter: ");
      filter_matcher_dump(stdout, pMatcher);
      fprintf(stdout, "\n");

      filter_matcher_destroy(&pMatcher);
    }

  }
  fprintf(stdout, "\n");
}
