// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/10/07
// $Id: main-perf.c,v 1.6 2008-04-11 13:04:39 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <sys/time.h>

#include <libgds/hash_utils.h>
#include <libgds/str_util.h>

#include <api.h>
#include <bgp/comm.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/route.h>

// -----[ test_rib_perf ]--------------------------------------------
/**
 * Test function (only available in EXPERIMENTAL mode).
 *
 * Current test function measures the number of nodes required to
 * store a complete RIB into a non-compact trie (radix-tree) or in a
 * compact trie (Patricia tree).
 */
#ifdef __EXPERIMENTAL__
/*int test_rib_perf(int argc, char * argv[])
{
  int iIndex;
  SRoutes * pRoutes= mrtd_ascii_load_routes(NULL, pcArg);
  SRoute * pRoute;
  SRadixTree * pRadixTree;
  STrie * pTrie;

  if (pRoutes != NULL) {
    pRadixTree= radix_tree_create(32, NULL);
    pTrie= trie_create(NULL);
    for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
      pRoute= (SRoute *) pRoutes->data[iIndex];
      radix_tree_add(pRadixTree, pRoute->sPrefix.tNetwork,
		     pRoute->sPrefix.uMaskLen, pRoute);
      trie_insert(pTrie, pRoute->sPrefix.tNetwork,
		  pRoute->sPrefix.uMaskLen, pRoute);
    }
    fprintf(stdout, "radix-tree: %d\n", radix_tree_num_nodes(pRadixTree));
    fprintf(stdout, "trie: %d\n", trie_num_nodes(pTrie));
    radix_tree_destroy(&pRadixTree);
    trie_destroy(&pTrie);
    routes_list_destroy(&pRoutes);
    return 0;
  } else {
    fprintf(stderr, "could not load \"%s\"\n", pcArg);
    return -1;
  }
  }*/
#endif


/////////////////////////////////////////////////////////////////////
//
// BGP ATTRIBUTES HASH FUNCTION EVALUATION
//
/////////////////////////////////////////////////////////////////////

#define AS_PATH_STR_SIZE 1024

// -----[ _array_path_destroy ]--------------------------------------
static void _array_path_destroy(void * pItem)
{
  SBGPPath * pPath= (SBGPPath *) pItem;
  path_destroy(&pPath);
}

// -----[ _array_path_compare ]--------------------------------------
static int _array_path_compare(void * pItem1, void * pItem2,
			       unsigned int uEltSize)
{
  SBGPPath * pPath1= *((SBGPPath **) pItem1);
  SBGPPath * pPath2= *((SBGPPath **) pItem2);

  return path_cmp(pPath1, pPath2);
}

// -----[ _array_comm_destroy ]--------------------------------------
static void _array_comm_destroy(void * pItem)
{
}

// -----[ _array_comm_compare ]--------------------------------------
static int _array_comm_compare(void * pItem1, void * pItem2,
			       unsigned int uEltSize)
{
  SCommunities * pComm1= *((SCommunities **) pItem1);
  SCommunities * pComm2= *((SCommunities **) pItem2);

  return comm_cmp(pComm1, pComm2);
}


// -----[ path_to_buf ]----------------------------------------------
/**
 * Convert an AS-Path to a byte stream.
 */
static inline size_t path_to_buf(SBGPPath * pPath, uint8_t * puBuffer,
				 size_t tSize)
{
  unsigned int uIndex, uIndex2;
  SPathSegment * pSegment;
  uint8_t * puOrig= puBuffer;

  for (uIndex= 0; uIndex < path_num_segments(pPath); uIndex++) {
    pSegment= (SPathSegment *) pPath->data[uIndex];
    *(puBuffer++)= pSegment->uType;
    for (uIndex2= 0; uIndex2 < pSegment->uLength; uIndex2++) {
      *(puBuffer++)= pSegment->auValue[uIndex2] & 255;
      *(puBuffer++)= pSegment->auValue[uIndex2] >> 8;
    }
  }
  return puBuffer-puOrig;
}

// -----[ comm_to_buff ]---------------------------------------------
/**
 * Convert a Communities to a byte stream.
 */
static inline size_t comm_to_buf(SCommunities * pComm, uint8_t * puBuffer,
				 size_t tSize)
{
  unsigned int uIndex;
  uint8_t * puOrig= puBuffer;
  comm_t tComm;

  if (pComm != NULL) {
    for (uIndex= 0; uIndex < pComm->uNum; uIndex++) {
      tComm= (comm_t) pComm->asComms[uIndex];
      *(puBuffer++)= tComm & 255;
      tComm= tComm >> 8;
      *(puBuffer++)= tComm & 255;
      tComm= tComm >> 8;
      *(puBuffer++)= tComm & 255;
      tComm= tComm >> 8;
      *(puBuffer++)= tComm;
    }
  }
  return puBuffer-puOrig;
}


/* The golden ratio: an arbitrary value */
#define JHASH_GOLDEN_RATIO  0x9e3779b9

/* NOTE: Arguments are modified. */
#define __jhash_mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/* The most generic version, hashes an arbitrary sequence
 * of bytes.  No alignment or length assumptions are made about
 * the input key.
 */
uint32_t jhash (void *key, uint32_t length, uint32_t initval)
{
  uint32_t a, b, c, len;
  uint8_t *k = key;

  len = length;
  a = b = JHASH_GOLDEN_RATIO;
  c = initval;

  while (len >= 12)
    {
      a +=
        (k[0] + ((uint32_t) k[1] << 8) + ((uint32_t) k[2] << 16) +
         ((uint32_t) k[3] << 24));
      b +=
        (k[4] + ((uint32_t) k[5] << 8) + ((uint32_t) k[6] << 16) +
         ((uint32_t) k[7] << 24));
      c +=
        (k[8] + ((uint32_t) k[9] << 8) + ((uint32_t) k[10] << 16) +
         ((uint32_t) k[11] << 24));

      __jhash_mix (a, b, c);

      k += 12;
      len -= 12;
    }

  c += length;
  switch (len)
    {
    case 11:
      c += ((uint32_t) k[10] << 24);
    case 10:
      c += ((uint32_t) k[9] << 16);
    case 9:
      c += ((uint32_t) k[8] << 8);
    case 8:
      b += ((uint32_t) k[7] << 24);
    case 7:
      b += ((uint32_t) k[6] << 16);
    case 6:
      b += ((uint32_t) k[5] << 8);
    case 5:
      b += k[4];
    case 4:
      a += ((uint32_t) k[3] << 24);
    case 3:
      a += ((uint32_t) k[2] << 16);
    case 2:
      a += ((uint32_t) k[1] << 8);
    case 1:
      a += k[0];
    };

  __jhash_mix (a, b, c);

  return c;
}

// -----[ path_strhash ]---------------------------------------------
static uint32_t path_strhash(const void * pItem, unsigned int uHashSize)
{
  char acPathStr[AS_PATH_STR_SIZE];
  SBGPPath * pPath= (SBGPPath *) pItem;

  assert(path_to_string(pPath, 1,
			acPathStr, AS_PATH_STR_SIZE) < AS_PATH_STR_SIZE);
  return hash_utils_key_compute_string(acPathStr, uHashSize) % uHashSize;
}

// -----[ path_jhash]------------------------------------------------
static uint32_t path_jhash(const void * pItem, unsigned int uHashSize)
{
  SBGPPath * pPath= (SBGPPath *) pItem;
  uint8_t auBuffer[1024];
  size_t tSize;

  tSize= path_to_buf(pPath, auBuffer, sizeof(auBuffer));
  return jhash(auBuffer, tSize, JHASH_GOLDEN_RATIO) % uHashSize;
}

// -----[ comm_strhash ]---------------------------------------------
static uint32_t comm_strhash(const void * pItem, unsigned int uHashSize)
{
  char acStr[AS_PATH_STR_SIZE];
  SCommunities * pComm= (SCommunities *) pItem;

  assert(comm_to_string(pComm, acStr, sizeof(acStr)) < AS_PATH_STR_SIZE);
  return hash_utils_key_compute_string(acStr, uHashSize) % uHashSize;;
}

// -----[ comm_hash_zebra ]------------------------------------------
static uint32_t comm_hash_zebra(const void * pItem, unsigned int uHashSize)
{
  SCommunities * pComm= (SCommunities *) pItem;
  uint8_t auBuffer[1024];
  size_t tSize;
  uint32_t uKey= 0;

  tSize= comm_to_buf(pComm, auBuffer, sizeof(auBuffer));
  for (; tSize > 0; tSize--) {
    uKey+= auBuffer[tSize] & 255;
  }
  return uKey % uHashSize;
}

// -----[ comm_jhash ]-----------------------------------------------
static uint32_t comm_jhash(const void * pItem, unsigned int uHashSize)
{
  SCommunities * pComm= (SCommunities *) pItem;
  uint8_t auBuffer[1024];
  size_t tSize;

  tSize= comm_to_buf(pComm, auBuffer, sizeof(auBuffer));
  return jhash(auBuffer, tSize, JHASH_GOLDEN_RATIO) % uHashSize;
}

typedef uint32_t (*FHashFunction)(const void * pItem, unsigned int);

typedef struct {
  FHashFunction fHashFunc;
  char *        pcName;
} SHashMethod;

SHashMethod PATH_HASH_METHODS[]= {
  {path_strhash, "Path-String"},
  {path_hash_zebra, "Path-Zebra"},
  {path_hash_OAT, "Path-OAT"},
  {path_jhash, "Path-Jenkins"},
};
#define PATH_HASH_METHODS_NUM sizeof(PATH_HASH_METHODS)/sizeof(PATH_HASH_METHODS[0])

SHashMethod COMM_HASH_METHODS[]= {
  {comm_strhash, "Comm-String"},
  {comm_hash_zebra, "Comm-Zebra"},
  /*{comm_hash_OAT, "Comm-OAT"},*/
  {comm_jhash, "Comm-Jenkins"},
};
#define COMM_HASH_METHODS_NUM sizeof(COMM_HASH_METHODS)/sizeof(COMM_HASH_METHODS[0])

typedef struct {
  SHashMethod * pMethods;
  uint8_t       uNumMethods;
  char *        pcName;
  SPtrArray *   pArray;
} SAttrInfo;

SAttrInfo ATTR_INFOS[]= {
  {PATH_HASH_METHODS, PATH_HASH_METHODS_NUM, "Path", NULL},
  {COMM_HASH_METHODS, COMM_HASH_METHODS_NUM, "Comm", NULL},
};
#define ATTR_INFOS_NUM sizeof(ATTR_INFOS)/sizeof(ATTR_INFOS[0])

#ifdef HAVE_BGPDUMP
// -----[ _route_handler ]-------------------------------------------
/**
 * Handle routes loaded by the bgpdump library. Store the route's
 * attributes in the target arrays. Supported attributes are AS-Path
 * and Communities.
 */
static int _route_handler(int iStatus, SRoute * pRoute,
			  net_addr_t tPeerAddr,
			  unsigned int uPeerAS, void * pContext)
{
  SAttrInfo * pAttrInfos= (SAttrInfo *) pContext;

  if (iStatus != BGP_ROUTES_INPUT_STATUS_OK)
    return BGP_ROUTES_INPUT_SUCCESS;

  // AS-Path attribute
  ptr_array_add(pAttrInfos[0].pArray, &pRoute->pAttr->pASPathRef);
  pRoute->pAttr->pASPathRef= NULL;

  // Communities attribute
  ptr_array_add(pAttrInfos[1].pArray, &pRoute->pAttr->pCommunities);
  pRoute->pAttr->pCommunities= NULL;

  //route_destroy(&pRoute);
  return BGP_ROUTES_INPUT_SUCCESS;
}
#endif /* HAVE_BGPDUMP */

// -----[ test_path_hash_perf ]--------------------------------------
int test_path_hash_perf(int argc, char * argv[])
{
  unsigned int uIndex;
  struct timeval tp;
  double dStartTime;
  double dEndTime;
#define MAX_HASH_SIZE 65536
  unsigned int uHashSize= MAX_HASH_SIZE;
  int aiHash[MAX_HASH_SIZE];
  uint32_t uKey;
  double dChiSquare, dExpected, dElement;
  uint32_t uDegreeFreedom;
  unsigned int uAttrIndex;
  unsigned int uHashIndex;
  FHashFunction fHashFunc;

  if (argc < 3) {
    log_printf(pLogErr, "Error: incorrect number of arguments.\n");
    return -1;
  }

  if ((str_as_uint(argv[1], &uHashSize) < 0) ||
      (uHashSize > MAX_HASH_SIZE)) {
    log_printf(pLogErr, "Error: invalid hash size specified.\n");
    return -1;
  }

  ATTR_INFOS[0].pArray= ptr_array_create(ARRAY_OPTION_SORTED |
					 ARRAY_OPTION_UNIQUE,
					 _array_path_compare,
					 _array_path_destroy);
  ATTR_INFOS[1].pArray= ptr_array_create(ARRAY_OPTION_SORTED |
					 ARRAY_OPTION_UNIQUE,
					 _array_comm_compare,
					 _array_comm_destroy);

  // Load AS-Paths from specified BGP routing tables
  log_printf(pLogErr,
	     "***** loading files *******************"
	     "***************************************\n");
  while (argc-- > 2) {
    log_printf(pLogErr, "* %s...", argv[2]);
    log_flush(pLogErr);
#ifdef HAVE_BGPDUMP
    if (mrtd_binary_load(argv[2], _route_handler, ATTR_INFOS) != 0)
      log_printf(pLogErr, " KO :-(\n");
    else
      log_printf(pLogErr, " OK :-)\n");
#else
    log_printf(pLogErr, " KO :-(  /* not bound to libbgpdump */\n");
#endif /* HAVE_BGPDUMP */
    log_flush(pLogErr);
  }


  log_printf(pLogErr,
	     "\n***** analysing data ******************"
	     "***************************************\n");

  for (uAttrIndex= 0; uAttrIndex < ATTR_INFOS_NUM; uAttrIndex++) {

    SHashMethod * pMethods= ATTR_INFOS[uAttrIndex].pMethods;
    uint8_t uNumMethods= ATTR_INFOS[uAttrIndex].uNumMethods;
    SPtrArray * pArray= ATTR_INFOS[uAttrIndex].pArray;

    log_printf(pLogErr, "***** Attribute [%s]\n",
	       ATTR_INFOS[uAttrIndex].pcName);

    log_printf(pLogErr, "- number of different values: %d\n",
	       ptr_array_length(pArray));

    // Adjust hash size so that each expected count per bin is at
    // least 5 (this is a common rule of thumb for the adequacy of
    // using the Chi-2 goodness of fit statistic)
    if ((uHashSize == 0) || (uHashSize > ptr_array_length(pArray)/5))
      uHashSize= ptr_array_length(pArray)/5;
    
    // Compute number of degrees of freedom for Pearson's Chi-2 test:
    //   number of bins - number of independent parameters fitted (1) - 1
    uDegreeFreedom= uHashSize-2; 
    log_printf(pLogErr, "- degrees of freedom: %d\n", uDegreeFreedom);

    log_printf(pLogErr, "- number of hash functions: %d\n", uNumMethods);
    log_printf(pLogErr, "- hash table size         : %d\n", uHashSize);
    
    // For various hash functions,
    // - measure time for computing hash value
    // - measure goodness of fit using Chi-2 statistic
    for (uHashIndex= 0; uHashIndex < uNumMethods; uHashIndex++) {
      
      fHashFunc= pMethods[uHashIndex].fHashFunc;
      
      memset(aiHash, 0, sizeof(aiHash));
      
      // Measure computation time
      log_printf(pLogErr, "(%d) method \"%s\"\n",
		 uHashIndex, pMethods[uHashIndex].pcName);
      assert(gettimeofday(&tp, NULL) >= 0);
      dStartTime= tp.tv_sec*1000000.0 + tp.tv_usec*1.0;

      for (uIndex= 0; uIndex < ptr_array_length(pArray); uIndex++) {
	uKey= fHashFunc(pArray->data[uIndex], uHashSize);
	aiHash[uKey]++;
      }
      assert(gettimeofday(&tp, NULL) >= 0);
      dEndTime= tp.tv_sec*1000000.0 + tp.tv_usec*1.0;
      log_printf(pLogErr, "  - elapsed time   : %f s\n",
		 (dEndTime-dStartTime)/1000000.0);
      
      // Measure goodness of fit using Pearson's Chi-2 statistic
      dChiSquare= 0;
      dExpected= ptr_array_length(pArray)*1.0 / (uHashSize*1.0);
      for (uIndex= 0; uIndex < uHashSize; uIndex++) {
	dElement= aiHash[uIndex]*1.0 - dExpected;
	dChiSquare+= (dElement*dElement)/dExpected;
      }
      log_printf(pLogErr, "  - chi-2 statistic: %f\n", dChiSquare);
      
      // Need to compute p-value for the above statistic, based on
      // Chi-2 distribution with same number of degrees of freedom.
      // Question: how to decide if the test succeeds based on the
      // p-value ?
      
    }
  }

  //ptr_array_destroy(&pArrayPath);
  //ptr_array_destroy(&pArrayComm);

  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// MAIN PART
//
/////////////////////////////////////////////////////////////////////

// -----[ _main_init ]-----------------------------------------------
static void _main_init() __attribute((constructor));
static void _main_init()
{
  libcbgp_init();
}

// -----[ _main_done ]-----------------------------------------------
static void _main_done() __attribute((destructor));
static void _main_done()
{
  libcbgp_done();
}

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  libcbgp_banner();

  test_path_hash_perf(argc, argv);
  return EXIT_SUCCESS;
}
