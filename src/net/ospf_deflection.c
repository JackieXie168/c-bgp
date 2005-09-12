// ==================================================================
// @(#)ospf_deflection.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 8/09/2005
// @lastdate 08/09/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef OSPF_SUPPORT

#include <assert.h>
#include <net/net_types.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/network.h>
#include <net/node.h>
#include <net/link-list.h>
#include <net/link.h>
#include <net/prefix.h>
#include <net/igp_domain.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL DESTINATION INFO OBJECT 
///////
///////  Info on destination that can have forwarding deflection
///////
////////////////////////////////////////////////////////////////////////////////

typedef SPrefix SCDestInfo; //Info on critical destination

// ----- _cd_info_create -------------------------------------------------------
SCDestInfo * _cd_info_create(SPrefix sDestPrefix) {
  SCDestInfo * pCDInfo = (SCDestInfo *) MALLOC(sizeof(SCDestInfo));
  *pCDInfo = sDestPrefix;
  return pCDInfo;
} 

// ----- _cd_info_destroy ------------------------------------------------------
void _cd_info_destroy(void * pItem) {  
  ip_prefix_destroy((SPrefix **) pItem);
}

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL DESTINATION INFO LIST OBJECT 
///////
////////////////////////////////////////////////////////////////////////////////

typedef SPtrArray * cd_info_list_t;

// ----- _cd_list_create -------------------------------------------------------
cd_info_list_t _cd_list_create() {
  cd_info_list_t pList = ptr_array_create(0, NULL, _cd_info_destroy);
  return pList;
} 

// ----- _cd_list_add ----------------------------------------------------------
int _cd_list_add(cd_info_list_t tList, SCDestInfo sDInfo) {
  SCDestInfo * pDInfo = (SCDestInfo *) MALLOC(sizeof(SCDestInfo));
  *pDInfo = sDInfo;
  fprintf(stdout, "Aggiungo dest ");
  ip_prefix_dump(stdout, *pDInfo);
fprintf(stdout, "\n");
  return ptr_array_add(tList, &pDInfo);
} 

// ----- _cd_list_destroy ------------------------------------------------------
void _cd_list_destroy(cd_info_list_t tList){ 
  ptr_array_destroy(&tList);	
} 

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL ROUTER INFO OBJECT 
///////
///////  Info on couple of a border router that in a given area
///////  could be cause of deflection.
///////
////////////////////////////////////////////////////////////////////////////////

typedef struct { 
  SNetNode         * pBR;         //router causes deflection
  SNetNode         * pER;         //exit router that can be selected
  net_link_delay_t   tRCost_BR_ER;
  ospf_area_t        tArea;       //checked area
  cd_info_list_t     tCDList;     //Critical Destination List
} SCRouterInfo;


// ----- _crouter_info_create --------------------------------------------------
SCRouterInfo * _crouter_info_create(SNetNode * pBR, SNetNode * pER, 
                            net_link_delay_t tRCost_BR_ER, ospf_area_t tArea){
  SCRouterInfo * pCRInfo = (SCRouterInfo *) MALLOC(sizeof(SCRouterInfo));
  pCRInfo->pBR = pBR;
  pCRInfo->pER = pER;
  pCRInfo->tRCost_BR_ER = tRCost_BR_ER;
  pCRInfo->tArea = tArea;
  pCRInfo->tCDList = _cd_list_create();
  return pCRInfo;
}

// ----- _crouter_info_destroy -------------------------------------------------
void _crouter_info_destroy(void * pItem) {  
  SCRouterInfo** ppCRInfo = (SCRouterInfo**) (pItem);
  if (*ppCRInfo != NULL) {
    _cd_list_destroy((*ppCRInfo)->tCDList);
    free(*ppCRInfo);
    *ppCRInfo = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL ROUTER INFO LIST OBJECT 
///////
////////////////////////////////////////////////////////////////////////////////

typedef SPtrArray * cr_info_list_t;

// ----- _cr_info_list_create -------------------------------------------------
cr_info_list_t _cr_info_list_create() {
  cr_info_list_t pList = ptr_array_create(0, NULL, _crouter_info_destroy);
  return pList;
} 

// ----- _cr_info_list_add ----------------------------------------------------
int _cr_info_list_add(cr_info_list_t tList, SCRouterInfo * pCRInfo) {
  return ptr_array_add(tList, &pCRInfo);
} 

// ----- cr_info_list_destroy --------------------------------------------------
void cr_info_list_destroy(cr_info_list_t tList){ 
  ptr_array_destroy(&tList);	
} 


////////////////////////////////////////////////////////////////////////////////
///////
///////  OSPF DEFLECTION CHECK FUNCTION
///////
////////////////////////////////////////////////////////////////////////////////

// ----- ospf_build_cd_for_each ---------------------------------------------
int build_cd_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem, 
		                                              void * pContext){ 
  SCRouterInfo * pCtx = (SCRouterInfo *) pContext;
  SOSPFRouteInfoList * pRIList = (SOSPFRouteInfoList *) pItem;
  SOSPFRouteInfo * pRIExitRouter = NULL;// = ospf_ri_list_get_route(pRIList);
  SOSPFRouteInfo * pRIBorderRouter;
  SPrefix sRoutePrefix;
  sRoutePrefix.tNetwork = (net_addr_t) uKey;
  sRoutePrefix.uMaskLen = uKeyLen;
  int iIndex;

  for (iIndex = 0; iIndex < ptr_array_length(pRIList); iIndex++){
    ptr_array_get_at(pRIList, iIndex, &pRIExitRouter);
    if (ospf_ri_route_type_is(pRIExitRouter, NET_ROUTE_IGP))
      break;
    else{
      pRIExitRouter = NULL;
    }
  }
  
  if (pRIExitRouter != NULL && 
      ospf_route_is_adver_in_area(pRIExitRouter, pCtx->tArea)) {
    
    pRIBorderRouter = OSPF_rt_find_exact(pCtx->pBR->pOspfRT, 
		                  sRoutePrefix, NET_ROUTE_ANY, pCtx->tArea);
    if (pRIBorderRouter == NULL){
	fprintf(stdout, "Aggiungo dest a cdl (manca route) ");
        ip_prefix_dump(stdout, sRoutePrefix);
	fprintf(stdout, "\n");

        _cd_list_add(pCtx->tCDList, sRoutePrefix);
    }
    else if (ospf_ri_get_cost(pRIExitRouter) + pCtx->tRCost_BR_ER < 
		                         ospf_ri_get_cost(pRIBorderRouter)){
        _cd_list_add(pCtx->tCDList, sRoutePrefix);
	fprintf(stdout, "Aggiungo dest a cdl ");
        ip_prefix_dump(stdout, sRoutePrefix);
	fprintf(stdout, "\n");
    } 
  }

  return OSPF_SUCCESS;
}


// ----- build_critical_destination_list ---------------------------------------
int build_critical_destination_list(SCRouterInfo * pCRInfo)
{
 LOG_DEBUG("bcdl on ER "); 
 ip_address_dump(stdout, pCRInfo->pER->tAddr);
 fprintf(stdout, " area %d BR ", pCRInfo->tArea);
 ip_address_dump(stdout, pCRInfo->pBR->tAddr);
 fprintf(stdout, "\n");
 if (OSPF_rt_for_each(pCRInfo->pER->pOspfRT, build_cd_for_each, pCRInfo) 
		                                                == OSPF_SUCCESS)
   return ptr_array_length(pCRInfo->tCDList);
 return 0;
}



// ----- build_critical_routers_list -------------------------------------------
int build_critical_routers_list(uint16_t uOspfDomain,
                                          ospf_area_t tArea, cr_info_list_t * tCRList) {
  int iIndexI, iIndexJ;
  SOSPFRouteInfo * pRouteIJArea, * pRouteIJBackbone;
  *tCRList = _cr_info_list_create();
  SCRouterInfo * pCRInfo;
  /* Builds the set of BR that belongs to area and are on the backbone */
  SPtrArray * pBRList = ospf_domain_get_br_on_bb_in_area(uOspfDomain, tArea);
  SNetNode * pBRI, *pBRJ;
  SPrefix sPrefix;
  
  /* Build all possibile couple of border router in the set. 
   * For each couple check if condition 1 is verified
   * If is verified check condition 2, too.
   * */
  for(iIndexI = 0; iIndexI < ptr_array_length(pBRList); iIndexI++){ 
    ptr_array_get_at(pBRList, iIndexI, &pBRI);
    for(iIndexJ = 0; iIndexJ < ptr_array_length(pBRList); iIndexJ++){ 
      if (iIndexI == iIndexJ)
        continue;
      ptr_array_get_at(pBRList, iIndexJ, &pBRJ);
      sPrefix.tNetwork = ospf_node_get_id(pBRJ);
      sPrefix.uMaskLen = 32;
      pRouteIJArea = OSPF_rt_find_exact(pBRI->pOspfRT, sPrefix,
		                        NET_ROUTE_IGP, tArea);

      /* If BR1 has not a route towards BR2 in area tArea this wants 
       * to say that there is a partition in the area. In this case no
       * deflection is possibile because there not will be a source that
       * has BR1 on its path towards BR2, if BR2.
       * */
      if (pRouteIJArea == NULL)
	continue;
      
      pRouteIJBackbone = OSPF_rt_find_exact(pBRI->pOspfRT, sPrefix, 
		                            NET_ROUTE_IGP, BACKBONE_AREA);
      /* If BR1 has not a route towards BR2 in backbone this wants 
       * to say that there is a partition in backbone.
       * Deflection can be possibile if BR2 will be choosen 
       * as exit router to exit area for some couple source-destination
       * where source has BR1 on the path towards BR2.
       * This happends because BR2 announces destination in area tArea
       * but BR1 has not a route towards it because he does not consider
       * summary in area != backbone.
       *
       * If we assume that when a route does not exist in a routing table
       * it has a cost infinite, condition 1 we try to verify is still
       * verified.
       * */
      if (pRouteIJBackbone == NULL) {      
	fprintf(stdout, "partizione nel backbone - verificata la cond 1\n");
	pCRInfo = _crouter_info_create(pBRI, pBRJ,
			               ospf_ri_get_cost(pRouteIJArea), tArea);
	int iRes =build_critical_destination_list(pCRInfo);
	fprintf(stdout, "Returned %d", iRes);
	if (iRes)   
	  _cr_info_list_add(*tCRList, pCRInfo);
	else
	  _crouter_info_destroy(&pCRInfo);
        continue;
      }   
      
      /* Checking condition 2. 
       * If it's true computing critical destination list for this 
       * couple of routers and adding them to critical routers list. 
       * */
      if (ospf_ri_get_cost(pRouteIJArea) < ospf_ri_get_cost(pRouteIJBackbone))
        fprintf(stdout, "Verificata condizione 1\n");
        pCRInfo = _crouter_info_create(pBRI, pBRJ,
			               ospf_ri_get_cost(pRouteIJArea), tArea);
        
	int iRes =build_critical_destination_list(pCRInfo); 
	fprintf(stdout, "Returned %d", iRes);
        if (iRes)   
	  _cr_info_list_add(*tCRList, pCRInfo);
	else
	  _crouter_info_destroy(&pCRInfo);
    }
  }
  return ptr_array_length(*tCRList);
}

// ----- cr_list_check_deflection_for_each -------------------------------------
/*int cr_list_check_deflection_for_each(void * pItem, void * pContext) {
  LOG_DEBUG("arrivato!\n");
  SCRouterInfo * pCRInfo = (SCRouterInfo *) (pItem);
  fprintf(stdout, "CRInfo BR: \n");
assert(pCRInfo != NULL);
assert(pCRInfo->pBR != NULL);
assert(pCRInfo->pER != NULL);
  ip_address_dump(stdout, pCRInfo->pBR->tAddr);
  fprintf(stdout, "ERInfo ER: ");
  ip_address_dump(stdout, pCRInfo->pER->tAddr);
  fprintf(stdout, "\n");
  
  fprintf(stdout, "Compute RSPT on ER\n");
  fprintf(stdout, "Trova BR in RSPT\n");
  fprintf(stdout, "Per ogni foglia di BR in RSPT\n");
  fprintf(stdout, "--- Per ogni dest in CRInfo\n");
  fprintf(stdout, "------- Trova route in routing table in foglia\n");
  fprintf(stdout, "------- if (advRouter(route) == ER(CRInfo) deflection!\n");
  return OSPF_SUCCESS;
}*/

// ----- cr_list_check_deflection ----------------------------------------------
int cr_list_check_deflection(cr_info_list_t tList) {
  LOG_DEBUG("check actual deflection\n");
  int iIndex, iDestIdx;
  SCRouterInfo * pCRInfo;
  SCDestInfo * pCDInfo;

  for (iIndex = 0; iIndex < ptr_array_length(tList); iIndex++){
    ptr_array_get_at(tList, iIndex, &pCRInfo);
    
    fprintf(stdout, "CRInfo BR: ");
    ip_address_dump(stdout, pCRInfo->pBR->tAddr);
    fprintf(stdout, "\n");
    fprintf(stdout, "ERInfo ER: ");
    ip_address_dump(stdout, pCRInfo->pER->tAddr);
    fprintf(stdout, "\nCritical dest list: \n");
    
    for (iDestIdx = 0; iDestIdx < ptr_array_length(pCRInfo->tCDList); 
		                                                   iDestIdx++){
      ptr_array_get_at(pCRInfo->tCDList, iDestIdx, &pCDInfo);
      ip_prefix_dump(stdout, *pCDInfo);
      fprintf(stdout, "\n");
      
    }
    
    fprintf(stdout, "Compute RSPT on ER\n");
    fprintf(stdout, "Trova BR in RSPT\n");
    fprintf(stdout, "Per ogni foglia di BR in RSPT\n");
    fprintf(stdout, "--- Per ogni dest in CRInfo\n");
    fprintf(stdout, "------- Trova route in routing table in foglia\n");
    fprintf(stdout, "------- if (advRouter(route) == ER(CRInfo) deflection!\n");
  }
return 0;
}

////////////////////////////////////////////////////////////////////////////////
///////
///////  OSPF DEFLECTION TEST FUNCTION
///////
////////////////////////////////////////////////////////////////////////////////

//----- ospf_deflection_test() -------------------------------------------------
int ospf_deflection_test(uint16_t uOspfDomain) { 
  cr_info_list_t tCBRList = NULL;
  if (build_critical_routers_list(uOspfDomain, 2, &tCBRList)){  
    LOG_DEBUG("CIAO!\n");
    assert(tCBRList != NULL);
    if (cr_list_check_deflection(tCBRList))
      fprintf(stdout, "Deflection!\n");
    
  }
  cr_info_list_destroy(tCBRList);
  return OSPF_SUCCESS;
}


#endif
