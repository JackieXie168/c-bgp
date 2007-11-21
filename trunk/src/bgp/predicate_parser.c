// ==================================================================
// @(#)predicate_parser.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/02/2004
// @lastdate 21/11/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/memory.h>
#include <libgds/stack.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif

#include <bgp/filter.h>
#include <bgp/filter_registry.h>
#include <bgp/predicate_parser.h>

#define MAX_DEPTH 100

#define TOKEN_NONE     0
#define TOKEN_EOF      1
#define TOKEN_ATOM     2
#define TOKEN_NOT      3
#define TOKEN_AND      4
#define TOKEN_OR       5
#define TOKEN_PAROPEN  6
#define TOKEN_PARCLOSE 7

//#define FSM_DEBUG 1

#ifdef FSM_DEBUG
static char * token_names[]= {
  "none",
  "eof",
  "atom",
  "not",
  "and",
  "or",
  "par-open",
  "par-close",
};
#endif /* FSM_DEBUG */

#define PARSER_STATE_WAIT_UNARY_OP  0
#define PARSER_STATE_WAIT_BINARY_OP 1
#define PARSER_STATE_WAIT_ARG       2

#ifdef FSM_DEBUG
static char * state_names[]= {
  "unary-op",
  "binary-op",
  "argument",
};
#endif /* FSM_DEBUG */

typedef struct {
  int iCommand;
  SFilterMatcher * pLeftPredicate;
} SParserStackItem;

static struct {
  int iState;
  int iCommand;
  SFilterMatcher * pLeftPredicate;
  SFilterMatcher * pRightPredicate;
  int iAtomError;
  SStack * pStack;
  unsigned int uColumn;
} sParser;

#ifdef FSM_DEBUG
static void _dump_stack()
{
  unsigned int uIndex;
  SParserStackItem * pItem;

  log_printf(pLogErr, "state [\n  %s\n", state_names[sParser.iState]);
  log_printf(pLogErr, "  stack [\n");
  log_printf(pLogErr, "    0:%s (l:", token_names[sParser.iCommand]);
  filter_matcher_dump(pLogErr, sParser.pLeftPredicate);
  log_printf(pLogErr, ", r:");
  filter_matcher_dump(pLogErr, sParser.pRightPredicate);
  log_printf(pLogErr, ")\n");
  for (uIndex= stack_depth(sParser.pStack); uIndex > 0 ; uIndex--) {
    pItem= (SParserStackItem *) sParser.pStack->apItems[uIndex-1];
    log_printf(pLogErr, "    %d:%s (l:",
	       (stack_depth(sParser.pStack)-uIndex)+1,
	       token_names[pItem->iCommand]);
    filter_matcher_dump(pLogErr, pItem->pLeftPredicate);
    log_printf(pLogErr, ")\n");
  }
  log_printf(pLogErr, "  ]\n");
  log_printf(pLogErr, "]\n");
}
#endif /* FSM_DEBUG */

#ifdef FSM_DEBUG
static void _dump_op()
{
  log_printf(pLogErr, "  operation: and(");
  filter_matcher_dump(pLogErr, sParser.pLeftPredicate);
  log_printf(pLogErr, ",");
  filter_matcher_dump(pLogErr, sParser.pRightPredicate);
  log_printf(pLogErr, ")\n");
}
#endif /* FSM_DEBUG */


// -----[ predicate_parser_perror ]--------------------------------
void predicate_parser_perror(SLogStream * pStream, int iErrorCode)
{
  char * pcError= predicate_parser_strerror(iErrorCode);
  if (pcError != NULL)
    log_printf(pStream, pcError);
  else
    log_printf(pStream, "unknown error (%i)", iErrorCode);
}

// -----[ predicate_parser_strerror ]------------------------------
char * predicate_parser_strerror(int iErrorCode)
{
#define PREDICATE_PARSER_ERROR_BUF_SIZE 1000
  static char acBuffer[PREDICATE_PARSER_ERROR_BUF_SIZE];

  switch (iErrorCode) {
  case PREDICATE_PARSER_SUCCESS:
    return "success";
  case PREDICATE_PARSER_ERROR_UNEXPECTED:
    return "unexpected";
  case PREDICATE_PARSER_ERROR_UNFINISHED_STRING:
    return "unterminated string";
  case PREDICATE_PARSER_ERROR_ATOM:
    snprintf(acBuffer, PREDICATE_PARSER_ERROR_BUF_SIZE,
	     "atom-parsing error [%s]",
	     cli_strerror(sParser.iAtomError));
    return acBuffer;
  case PREDICATE_PARSER_ERROR_UNEXPECTED_EOF:
    return "unexpected end-of-file";
  case PREDICATE_PARSER_ERROR_PAR_MISMATCH:
    return "parentheses mismatch";
  case PREDICATE_PARSER_ERROR_STATE_TOO_LARGE:
    return "state too large";
  case PREDICATE_PARSER_ERROR_UNARY_OP:
    return "unary operation not allowed here";
  }
  return NULL;
}

// -----[ _predicate_parser_get_atom ]-------------------------------
/**
 * Extract the next atom from the expression.
 *
 * Todo: keep quotes obtained in string-state ?
 *       
 */
static int _predicate_parser_get_atom(char ** ppcExpr,
				      SFilterMatcher ** ppMatcher)
{
  char * pcPos;
  char *pcSubExpr;
  char *pcTmpExpr;
  int iLen;
  int iError;

  pcTmpExpr= *ppcExpr;
  while (*pcTmpExpr && ((pcPos= strchr(")&|!", *pcTmpExpr)) == NULL)) {

    // Mini state-machine (enter "string state")
    if (*pcTmpExpr == '\"') {
      pcTmpExpr++;
      // Eat anything while in "string state"...
      while (*pcTmpExpr && *pcTmpExpr != '\"')
	pcTmpExpr++;
      // End was reached before "string state" was left :-(
      if (!*pcTmpExpr)
	return PREDICATE_PARSER_ERROR_UNFINISHED_STRING;
    }

    pcTmpExpr++;
  }

  iLen= pcTmpExpr-(*ppcExpr);

  pcSubExpr= (char *) MALLOC(sizeof(char)*((iLen)+1));
  strncpy(pcSubExpr, *ppcExpr, iLen);
  pcSubExpr[iLen]= 0;
  pcTmpExpr= pcSubExpr;
  iError= ft_registry_predicate_parser(pcTmpExpr, ppMatcher);
  if (iError != 0) {
    sParser.iAtomError= iError;
    iError= PREDICATE_PARSER_ERROR_ATOM;
  }
  FREE(pcSubExpr);

  (*ppcExpr)+= iLen;

  if (iError < 0)
    return iError;
  else
    return iLen;
}

// -----[ _predicate_parser_get ]------------------------------------
static int _predicate_parser_get(char ** ppcExpr)
{
  char cCurrent;
  int iError;

  while (**ppcExpr != 0) {
    cCurrent= **ppcExpr;
    (*ppcExpr)++;
    sParser.uColumn++;
    switch (cCurrent) {
    case '!': return TOKEN_NOT;
    case '&': return TOKEN_AND;
    case '|': return TOKEN_OR;
    case '(': return TOKEN_PAROPEN;
    case ')': return TOKEN_PARCLOSE;
    case ' ':
    case '\t':
      break;
    default:
      (*ppcExpr)--;
      sParser.uColumn--;
      iError= _predicate_parser_get_atom(ppcExpr, &sParser.pLeftPredicate);
      if (iError >= 0) {
	sParser.uColumn+= iError;
	return TOKEN_ATOM;
      } else
	return iError;
    }
  }
  return TOKEN_EOF;
}

// -----[ _predicate_parser_push_state ]-----------------------------
/**
 * Push a new state on the stack. This must only be done for
 * operations that will be processed later (because their right-value
 * is not known yet).
 *
 * Typical stackable operations are
 *   AND(l,?)
 *   OR(l,?)
 *   NOT(?)
 *   PAROPEN(?)
 */
static int _predicate_parser_push_state(int iCommand)
{
  SParserStackItem * pItem;

#ifdef FSM_DEBUG
  log_printf(pLogErr, "push-state(%s)\n", token_names[iCommand]);
#endif /* FSM_DEBUG */

  // Push previous state
  pItem= (SParserStackItem *) MALLOC(sizeof(SParserStackItem));
  pItem->pLeftPredicate= sParser.pLeftPredicate;
  pItem->iCommand= iCommand;
  if (stack_push(sParser.pStack, pItem) != 0) {
    FREE(pItem);
    return -1;
  }

  // Initialise current state
  sParser.pLeftPredicate= NULL;
  sParser.pRightPredicate= NULL;
  sParser.iCommand= TOKEN_NONE;
  sParser.iState= PARSER_STATE_WAIT_UNARY_OP;
  return 0;
}

// -----[ _predicate_parser_pop_state ]------------------------------
static int _predicate_parser_pop_state()
{
  SParserStackItem * pItem;

#ifdef FSM_DEBUG
  log_printf(pLogErr, "  pop-state(");
#endif /* FSM_DEBUG */

  sParser.iCommand= TOKEN_NONE;
  assert(!stack_is_empty(sParser.pStack));

  pItem= (SParserStackItem *) stack_pop(sParser.pStack);
  sParser.pRightPredicate= sParser.pLeftPredicate;
  sParser.pLeftPredicate= pItem->pLeftPredicate;
  sParser.iCommand= pItem->iCommand;
  sParser.iState= PARSER_STATE_WAIT_BINARY_OP;

#ifdef FSM_DEBUG
  log_printf(pLogErr, "l:");
  filter_matcher_dump(pLogErr, sParser.pLeftPredicate);
  log_printf(pLogErr, ",r:");
  filter_matcher_dump(pLogErr, sParser.pRightPredicate);
  log_printf(pLogErr, ",command:%s)\n", token_names[sParser.iCommand]);
#endif /* FSM_DEBUG */

  FREE(pItem);
  return 0;
}

// -----[ _predicate_parser_apply_block_ops ]------------------------
/**
 * Apply all the stacked operations until the end of the current
 * block. A block is limited by TOKEN_EOF or by TOKEN_PAROPEN.
 */
static int _predicate_parser_apply_block_ops()
{

#ifdef FSM_DEBUG
  log_printf(pLogErr, "apply-ops [\n");
#endif /* FSM_DEBUG */

  do {

#ifdef FSM_DEBUG
    _dump_op();
#endif /* FSM_DEBUG */

    switch (sParser.iCommand) {
    case TOKEN_NONE:
      break;
    case TOKEN_AND:
      sParser.pLeftPredicate= filter_match_and(sParser.pLeftPredicate,
					       sParser.pRightPredicate);
      break;
    case TOKEN_NOT:
      sParser.pLeftPredicate= filter_match_not(sParser.pRightPredicate);
      break;
    case TOKEN_OR:
      sParser.pLeftPredicate= filter_match_or(sParser.pLeftPredicate,
					      sParser.pRightPredicate);
      break;
    case TOKEN_PARCLOSE:
      if (stack_is_empty(sParser.pStack))
	return PREDICATE_PARSER_ERROR_PAR_MISMATCH;
      _predicate_parser_pop_state();
      if (sParser.iCommand != TOKEN_PAROPEN)
	return PREDICATE_PARSER_ERROR_PAR_MISMATCH;
      sParser.pLeftPredicate= sParser.pRightPredicate;
      break;

    default:
      log_printf(pLogErr, "Error: unsupported operation %d\n",
		 sParser.iCommand);
      abort();
    }

    sParser.iCommand= TOKEN_NONE;
    sParser.pRightPredicate= NULL;

    if (stack_is_empty(sParser.pStack))
      break;

    if (((SParserStackItem *) stack_top(sParser.pStack))->iCommand
	== TOKEN_PAROPEN)
      break;

    if (_predicate_parser_pop_state() < 0)
      break;

  } while (1);

#ifdef FSM_DEBUG
  log_printf(pLogErr, "]\n");
#endif /* FSM_DEBUG */

  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _predicate_parser_wait_unary_op ]--------------------------
/**
 * This state only allows ATOM, NOT and PAR-OPEN
 */
static int _predicate_parser_wait_unary_op(int iToken)
{
  switch (iToken) {
  case TOKEN_ATOM:
    sParser.iCommand= TOKEN_NONE;
    _predicate_parser_apply_block_ops();
    sParser.iState= PARSER_STATE_WAIT_BINARY_OP;
    break;
  case TOKEN_EOF:
    return PREDICATE_PARSER_ERROR_UNEXPECTED_EOF;
  case TOKEN_NOT:
    if (_predicate_parser_push_state(iToken) < 0)
      return PREDICATE_PARSER_ERROR_STATE_TOO_LARGE;
    sParser.iState= PARSER_STATE_WAIT_ARG;
    break;
  case TOKEN_PARCLOSE:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  case TOKEN_PAROPEN:
    if (_predicate_parser_push_state(iToken))
      return PREDICATE_PARSER_ERROR_STATE_TOO_LARGE;
    break;
  case TOKEN_AND:
  case TOKEN_OR:
    return PREDICATE_PARSER_ERROR_LEFT_EXPR;
  default:
    abort();
  }
  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _predicate_parser_wait_binary_op ]-------------------------
/**
 * This states only allows AND, OR, EOF and PAR-CLOSE
 */
static int _predicate_parser_wait_binary_op(int iToken)
{
  switch (iToken) {
  case TOKEN_AND:
  case TOKEN_OR:
    if (_predicate_parser_push_state(iToken) < 0)
      return PREDICATE_PARSER_ERROR_UNEXPECTED;
    sParser.iState= PARSER_STATE_WAIT_UNARY_OP;
    break;
  case TOKEN_ATOM:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  case TOKEN_EOF:
    return PREDICATE_PARSER_END;
  case TOKEN_NOT:
    return PREDICATE_PARSER_ERROR_UNARY_OP;
  case TOKEN_PARCLOSE:
    sParser.iCommand= TOKEN_PARCLOSE;
    return _predicate_parser_apply_block_ops();
  case TOKEN_PAROPEN:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  default:
    abort();
  }
  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _preficate_parser_wait_arg ]-------------------------------
/**
 * This states only allows ATOM and PAR-OPEN
 */
static int _predicate_parser_wait_arg(int iToken)
{
  switch (iToken) {
  case TOKEN_ATOM:
    sParser.iCommand= TOKEN_NONE;
    _predicate_parser_apply_block_ops();
    sParser.iState= PARSER_STATE_WAIT_BINARY_OP;
    break;
  case TOKEN_AND:
  case TOKEN_OR:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  case TOKEN_NOT:
    return PREDICATE_PARSER_ERROR_UNARY_OP;
  case TOKEN_EOF:
    return PREDICATE_PARSER_ERROR_UNEXPECTED_EOF;
  case TOKEN_PAROPEN:
    if (_predicate_parser_push_state(iToken) < 0)
      return PREDICATE_PARSER_ERROR_STATE_TOO_LARGE;
    sParser.iState= PARSER_STATE_WAIT_UNARY_OP;
    break;
  default:
    abort();
  }
  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _predicate_parser_init ]-----------------------------------
static void _predicate_parser_init()
{
  sParser.iState= PARSER_STATE_WAIT_UNARY_OP;
  sParser.pStack= stack_create(MAX_DEPTH);
  sParser.iCommand= TOKEN_NONE;
  sParser.pLeftPredicate= NULL;
  sParser.pRightPredicate= NULL;
  sParser.uColumn= 0;
}

// -----[ _predicate_parser_done ]-----------------------------------
static void _predicate_parser_destroy()
{
  SParserStackItem * pItem;

  while (!stack_is_empty(sParser.pStack)) {
    pItem= (SParserStackItem *) stack_pop(sParser.pStack);
    filter_matcher_destroy(&pItem->pLeftPredicate);
    FREE(pItem);
  }
  stack_destroy(&sParser.pStack);
}

// ----- predicate_parser -------------------------------------------
/**
 * 
 */
int predicate_parser(char * pcExpr, SFilterMatcher ** ppMatcher)
{
  int iError= PREDICATE_PARSER_SUCCESS;
  int iToken;

  _predicate_parser_init();

  while (iError == PREDICATE_PARSER_SUCCESS) {

#ifdef FSM_DEBUG
    _dump_stack();
#endif /* FSM_DEBUG */

    // Get next token
    iToken= _predicate_parser_get(&pcExpr);
    if (iToken < 0) {
      iError= iToken;
      break;
    }

#ifdef FSM_DEBUG
    log_printf(pLogErr, "----[token:%s]---->\n",
	       token_names[iToken]);
#endif /* FSM_DEBUG */

    switch (sParser.iState) {
    case PARSER_STATE_WAIT_UNARY_OP:
      iError= _predicate_parser_wait_unary_op(iToken); break;
    case PARSER_STATE_WAIT_BINARY_OP:
      iError= _predicate_parser_wait_binary_op(iToken); break;
    case PARSER_STATE_WAIT_ARG:
      iError= _predicate_parser_wait_arg(iToken); break;
    default:
      abort();
    }
  }

  if (!stack_is_empty(sParser.pStack))
    if (iError >= 0)
      iError= PREDICATE_PARSER_ERROR_PAR_MISMATCH;

  *ppMatcher= sParser.pLeftPredicate;

  _predicate_parser_destroy();  

  return (iError < 0)?iError:PREDICATE_PARSER_SUCCESS;
}
