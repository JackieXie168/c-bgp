// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @lastdate 21/05/2007
// @date 04/09/07
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <sys/time.h>

#include <libgds/hash_utils.h>
#include <libgds/str_util.h>

#include <api.h>
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
int test_rib_perf(int argc, char * argv[])
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
}
#endif

// -----[ test_path_hash_perf ]--------------------------------------
#define AS_PATH_STR_SIZE 1024

static void _array_path_destroy(void * pItem)
{
  SBGPPath * pPath= (SBGPPath *) pItem;
  path_destroy(&pPath);
}

// -----[ _array_path_compare ]--------------------------------------
static int _array_path_compare(void * pItem1, void * pItem2,
			       unsigned int uEltSize)
{
  int iCmp;
  char acPathStr1[AS_PATH_STR_SIZE], acPathStr2[AS_PATH_STR_SIZE];
  SBGPPath * pPath1= *((SBGPPath **) pItem1);
  SBGPPath * pPath2= *((SBGPPath **) pItem2);

  // If paths pointers are equal, no need to compare their content.
  if (pPath1 == pPath2)
    return 0;

  assert(path_to_string(pPath1, 1, acPathStr1, AS_PATH_STR_SIZE)
	 < AS_PATH_STR_SIZE);
  assert(path_to_string(pPath2, 1, acPathStr2, AS_PATH_STR_SIZE)
	 < AS_PATH_STR_SIZE);
  iCmp= strcmp(acPathStr1, acPathStr2);
  return iCmp;
}

static int _route_handler(int iStatus, SRoute * pRoute,
			  net_addr_t tPeerAddr,
			  unsigned int uPeerAS, void * pContext)
{
  SPtrArray * pArray= (SPtrArray *) pContext;

  if (iStatus != BGP_ROUTES_INPUT_STATUS_OK)
    return BGP_ROUTES_INPUT_SUCCESS;

  ptr_array_add(pArray, &pRoute->pAttr->pASPathRef);
  pRoute->pAttr->pASPathRef= NULL;

  //route_destroy(&pRoute);
  return BGP_ROUTES_INPUT_SUCCESS;
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

uint32_t path_strhash(const void * pItem, const uint32_t uHashSize)
{
  char acPathStr[AS_PATH_STR_SIZE];
  SBGPPath * pPath= (SBGPPath *) pItem;

  assert(path_to_string(pPath, 1,
			acPathStr, AS_PATH_STR_SIZE) < AS_PATH_STR_SIZE);
  return hash_utils_key_compute_string(acPathStr, uHashSize) % uHashSize;
}

uint32_t path_jhash(const void * pItem, const uint32_t uHashSize)
{
  SBGPPath * pPath= (SBGPPath *) pItem;
  SPathSegment * pSegment;
  uint8_t auBuffer[1024];
  unsigned int uIndex, uIndex2;
  uint8_t * pBuffer= auBuffer;

  for (uIndex= 0; uIndex < path_num_segments(pPath); uIndex++) {
    pSegment= (SPathSegment *) pPath->data[uIndex];
    *(pBuffer++)= pSegment->uType;
    for (uIndex2= 0; uIndex2 < pSegment->uLength; uIndex2++) {
      *(pBuffer++)= pSegment->auValue[uIndex2] & 255;
      *(pBuffer++)= pSegment->auValue[uIndex2] >> 8;
    }
  }
  return jhash(auBuffer, pBuffer-auBuffer, JHASH_GOLDEN_RATIO) % uHashSize;
}

typedef uint32_t (*FASPHashFunction)(const void * pItem, const uint32_t);

FASPHashFunction afHashFunctions[]= {
  path_strhash,
  path_hash_zebra,
  path_hash_OAT,
  path_jhash,
};

int test_path_hash_perf(int argc, char * argv[])
{
  SPtrArray * pArray;
  unsigned int uIndex;
  struct timeval tp;
  double dStartTime;
  double dEndTime;
#define MAX_HASH_SIZE 65536
  uint32_t uHashSize= MAX_HASH_SIZE;
  int aiHash[MAX_HASH_SIZE];
  uint32_t uKey;
  double dChiSquare, dExpected, dElement;
  uint32_t uDegreeFreedom;
  unsigned int uHashIndex;
  FASPHashFunction fHashFunc;

  if (argc < 3) {
    log_printf(pLogErr, "Error: incorrect number of arguments.\n");
    return -1;
  }

  if ((str_as_uint(argv[1], &uHashSize) < 0) ||
      (uHashSize > MAX_HASH_SIZE)) {
    log_printf(pLogErr, "Error: invalid hash size specified.\n");
    return -1;
  }

  pArray= ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
			   _array_path_compare, _array_path_destroy);

  // Load AS-Paths from specified BGP routing tables
  log_printf(pLogErr, "***** loading files *****\n");
  while (argc-- > 2) {

    log_printf(pLogErr, "- file \"%s\"...", argv[2]);
    log_flush(pLogErr);
    if (mrtd_binary_load(argv[2], _route_handler, pArray) != 0)
      log_printf(pLogErr, " ko :-(\n");
    else
      log_printf(pLogErr, " ok :-)\n");
    log_flush(pLogErr);

  }
  log_printf(pLogErr, "- number of paths: %d\n", ptr_array_length(pArray));

  // Adjust hash size so that each expected count per bin is at
  // least 5 (this is a common rule of thumb for the adequacy of
  // using the Chi-2 goodness of fit statistic)
  if ((uHashSize == 0) || (uHashSize > ptr_array_length(pArray)/5))
    uHashSize= ptr_array_length(pArray)/5;

  // Compute number of degrees of freedom for Pearson's Chi-2 test
  uDegreeFreedom= uHashSize-2; /* Number of bins
				  - number of independent parameters fitted (1)
				  - 1 */
  log_printf(pLogErr, "- degrees freedom: %d\n", uDegreeFreedom);


  // For various hash functions,
  // - measure time for computing hash value
  // - measure goodness of fit using Chi-2 statistic
  for (uHashIndex= 0;
       uHashIndex < sizeof(afHashFunctions)/sizeof(afHashFunctions[0]);
       uHashIndex++) {

    fHashFunc= afHashFunctions[uHashIndex];

    memset(aiHash, 0, sizeof(aiHash));

    // Measure computation time
    log_printf(pLogErr, "***** measuring computation time *****\n");
    assert(gettimeofday(&tp, NULL) >= 0);
    dStartTime= tp.tv_sec*1000000.0 + tp.tv_usec*1.0;

    for (uIndex= 0; uIndex < ptr_array_length(pArray); uIndex++) {
      uKey= fHashFunc((SBGPPath *) pArray->data[uIndex], uHashSize);
      aiHash[uKey]++;
    }
    assert(gettimeofday(&tp, NULL) >= 0);
    dEndTime= tp.tv_sec*1000000.0 + tp.tv_usec*1.0;
    log_printf(pLogErr, "- elapsed time   : %f s\n",
	       (dEndTime-dStartTime)/1000000.0);

    // Measure goodness of fit using Pearson's Chi-2 statistic
    dChiSquare= 0;
    dExpected= ptr_array_length(pArray)*1.0 / (uHashSize*1.0);
    for (uIndex= 0; uIndex < uHashSize; uIndex++) {
      dElement= aiHash[uIndex]*1.0 - dExpected;
      dChiSquare+= (dElement*dElement)/dExpected;
    }
    log_printf(pLogErr, "- chi-2 statistic: %f\n", dChiSquare);

    // Need to compute p-value for the above statistic, based on
    // Chi-2 distribution with same number of degrees of freedom.
    // Question: how to decide if the test succeeds based on the
    // p-value ?

  }

  ptr_array_destroy(&pArray);

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
