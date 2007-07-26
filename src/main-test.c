// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @lastdate 21/05/2007
// @date 23/07/07
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/hash_utils.h>
#include <libgds/str_util.h>
#include <libgds/utest.h>

#include <api.h>
#include <bgp/as.h>
#include <bgp/comm.h>
#include <bgp/comm_hash.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/path_hash.h>
#include <bgp/path_segment.h>
#include <bgp/route.h>
#include <bgp/route-input.h>
#include <net/error.h>
#include <net/ipip.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/subnet.h>


/////////////////////////////////////////////////////////////////////
//
// NET ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_attr_address4 ]-----------------------------------
static int test_net_attr_address4()
{
  net_addr_t tAddress= IPV4_TO_INT(255,255,255,255);
  ASSERT_RETURN(tAddress == UINT32_MAX,
		"255.255.255.255 should be equal to %u", UINT32_MAX);
  tAddress= NET_ADDR_ANY;
  ASSERT_RETURN(tAddress == 0,
		"0.0.0.0 should be equal to 0");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_address4_2str ]------------------------------
static int test_net_attr_address4_2str()
{
  char acBuffer[16];
  ASSERT_RETURN(ip_address_to_string(IPV4_TO_INT(192,168,0,0),
				     acBuffer, sizeof(acBuffer)) == 11,
		"ip_address_to_string() should return 11");
  ASSERT_RETURN(strcmp(acBuffer, "192.168.0.0") == 0,
		"string for address 192.168.0.0 should be \"192.168.0.0\"");
  ASSERT_RETURN(ip_address_to_string(IPV4_TO_INT(192,168,1,23),
				     acBuffer, sizeof(acBuffer)) == 12,
		"ip_address_to_string() should return 12");
  ASSERT_RETURN(strcmp(acBuffer, "192.168.1.23") == 0,
		"string for address 192.168.1.23 should be \"192.168.1.23\"");
  ASSERT_RETURN(ip_address_to_string(IPV4_TO_INT(255,255,255,255),
				     acBuffer, sizeof(acBuffer)-1) < 0,
		"ip_address_to_string() should return < 0");
  ASSERT_RETURN(ip_address_to_string(IPV4_TO_INT(255,255,255,255),
				     acBuffer, sizeof(acBuffer)) == 15,
		"ip_address_to_string() should return 15");
  ASSERT_RETURN(strcmp(acBuffer, "255.255.255.255") == 0,
		"string for address 255.255.255.255 should be \"255.255.255.255\"");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_address4_str2 ]------------------------------
static int test_net_attr_address4_str2()
{
  net_addr_t tAddress;
  char * pcEndPtr;
  ASSERT_RETURN((ip_string_to_address("192.168.0.0", &pcEndPtr, &tAddress) >= 0) && (*pcEndPtr == '\0'),
		"address \"192.168.0.0\" should be valid");
  ASSERT_RETURN(tAddress == IPV4_TO_INT(192,168,0,0),
		"address should 192.168.0.0");
  ASSERT_RETURN((ip_string_to_address("192.168.1.23", &pcEndPtr, &tAddress) >= 0) && (*pcEndPtr == '\0'),
		"address \"192.168.1.23\" should be valid");
  ASSERT_RETURN(tAddress == IPV4_TO_INT(192,168,1,23),
		"address should 192.168.1.23");
  ASSERT_RETURN((ip_string_to_address("255.255.255.255", &pcEndPtr, &tAddress) >= 0) && (*pcEndPtr == '\0'),
		"address \"255.255.255.255\" should be valid");
  ASSERT_RETURN(tAddress == IPV4_TO_INT(255,255,255,255),
		"address should 255.255.255.255");
  ASSERT_RETURN((ip_string_to_address("64.233.183.147a", &pcEndPtr, &tAddress) >= 0) && (*pcEndPtr == 'a'),
		"address \"64.233.183.147a\" should be valid (followed by stuff)");
  ASSERT_RETURN(tAddress == IPV4_TO_INT(64,233,183,147),
		"address should 64.233.183.147");
  ASSERT_RETURN(ip_string_to_address("64a.233.183.147", &pcEndPtr, &tAddress) < 0,
		"address \"64a.233.183.147a\" should be invalid");
  ASSERT_RETURN(ip_string_to_address("255.256.0.0", &pcEndPtr, &tAddress) < 0,
		"address \"255.256.0.0\" should be invalid");
  ASSERT_RETURN((ip_string_to_address("255.255.0.0.255", &pcEndPtr, &tAddress) >= 0) && (*pcEndPtr != '\0'),
		"address \"255.255.0.0.255\" should be valid (followed by stuff)");
  ASSERT_RETURN(ip_string_to_address("255.255.0", &pcEndPtr, &tAddress) < 0,
		"address \"255.255.0.0.255\" should be invalid");
  ASSERT_RETURN(ip_string_to_address("255..255.0", &pcEndPtr, &tAddress) < 0,
		"address \"255..255.0\" should be invalid");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4 ]------------------------------------
static int test_net_attr_prefix4()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(255,255,255,255),
		     .uMaskLen= 32 };
  SPrefix * pPrefix;
  ASSERT_RETURN((sPrefix.tNetwork == UINT32_MAX),
		"255.255.255.255 should be equal to %u", UINT32_MAX);
  sPrefix.tNetwork= NET_ADDR_ANY;
  sPrefix.uMaskLen= 0;
  ASSERT_RETURN(sPrefix.tNetwork == 0,
		"0.0.0.0 should be equal to 0");
  pPrefix= create_ip_prefix(IPV4_TO_INT(1,2,3,4), 5);
  ASSERT_RETURN((pPrefix != NULL) &&
		(pPrefix->tNetwork == IPV4_TO_INT(1,2,3,4)) &&
		(pPrefix->uMaskLen == 5),
		"prefix should be 1.2.3.4/5");
  ip_prefix_destroy(&pPrefix);
  ASSERT_RETURN(pPrefix == NULL,
		"destroyed prefix should be null");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_in ]---------------------------------
static int test_net_attr_prefix4_in()
{
  SPrefix sPrefix1= { .tNetwork= IPV4_TO_INT(192,168,1,0),
		      .uMaskLen= 24 };
  SPrefix sPrefix2= { .tNetwork= IPV4_TO_INT(192,168,0,0),
		      .uMaskLen= 16 };
  SPrefix sPrefix3= { .tNetwork= IPV4_TO_INT(192,168,2,0),
		      .uMaskLen= 24 };
  ASSERT_RETURN(ip_prefix_in_prefix(sPrefix1, sPrefix2) != 0,
		"192.168.1/24 should be in 192.168/16");
  ASSERT_RETURN(ip_prefix_in_prefix(sPrefix3, sPrefix2) != 0,
		"192.168.2/24 should be in 192.168/16");
  ASSERT_RETURN(ip_prefix_in_prefix(sPrefix2, sPrefix1) == 0,
		"192.168/16 should not be in 192.168.1/24");
  ASSERT_RETURN(ip_prefix_in_prefix(sPrefix3, sPrefix1) == 0,
		"192.168.2/24 should not be in 192.168.1/24");
  ASSERT_RETURN(ip_prefix_in_prefix(sPrefix1, sPrefix3) == 0,
		"192.168.1/24 should not be in 192.168.2/24");
  ASSERT_RETURN(ip_prefix_in_prefix(sPrefix1, sPrefix1) != 0,
		"192.168.1/24 should be in 192.168.1/24");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_match ]------------------------------
static int test_net_attr_prefix4_match()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(0,0,0,0),
		     .uMaskLen= 0 };
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(0,0,0,0), sPrefix) != 0,
		"0.0.0.0 should be in 0.0.0.0/0");
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(255,255,255,255), sPrefix) != 0,
		"255.255.255.255 should be in 0.0.0.0/0");
  sPrefix.tNetwork= IPV4_TO_INT(255,255,255,255);
  sPrefix.uMaskLen= 32;
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(0,0,0,0), sPrefix) == 0,
		"0.0.0.0 should not be in 255.255.255.255/32");
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(255,255,255,255), sPrefix) != 0,
		"255.255.255.255 should be in 255.255.255.255/32");
  sPrefix.tNetwork= IPV4_TO_INT(130,104,229,224);
  sPrefix.uMaskLen= 27;
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(130,104,229,225), sPrefix) != 0,
		"130.104.229.225 should be in 130.104.229.224/27");
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(130,104,229,240), sPrefix) != 0,
		"130.104.229.240 should be in 130.104.229.224/27");
  ASSERT_RETURN(ip_address_in_prefix(IPV4_TO_INT(130,104,229,256), sPrefix) == 0,
		"130.104.229.256 should not be in 130.104.229.224/27");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_mask ]-------------------------------
static int test_net_attr_prefix4_mask()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(192,168,128,2),
		     .uMaskLen= 17 };
  ip_prefix_mask(&sPrefix);
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(192,168,128,0)) &&
		(sPrefix.uMaskLen == 17),
		"masked prefix should be 192.168.128.0/17");
  sPrefix.tNetwork= IPV4_TO_INT(255,255,255,255);
  sPrefix.uMaskLen= 1;
  ip_prefix_mask(&sPrefix);
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(128,0,0,0)) &&
		(sPrefix.uMaskLen == 1),
		"masked prefix should be 128.0.0.0/1");
  sPrefix.tNetwork= IPV4_TO_INT(255,255,255,255);
  sPrefix.uMaskLen= 32;
  ip_prefix_mask(&sPrefix);
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(255,255,255,255)) &&
		(sPrefix.uMaskLen == 32),
		"masked prefix should be 255.255.255.255/32");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_2str ]-------------------------------
static int test_net_attr_prefix4_2str()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(192,168,0,0),
		     .uMaskLen= 16 };
  char acBuffer[19];
  ASSERT_RETURN(ip_prefix_to_string(&sPrefix, acBuffer, sizeof(acBuffer))
		== 14,
		"ip_prefix_to_string() should return 14");
  ASSERT_RETURN(strcmp(acBuffer, "192.168.0.0/16") == 0,
		"string for prefix 192.168/16 should be \"192.168.0.0/16\"");
  sPrefix.tNetwork= IPV4_TO_INT(255,255,255,255);
  sPrefix.uMaskLen= 32;
  ASSERT_RETURN(ip_prefix_to_string(&sPrefix, acBuffer, sizeof(acBuffer)-1)
		< 0,
		"ip_prefix_to_string() should return < 0");
  ASSERT_RETURN(ip_prefix_to_string(&sPrefix, acBuffer, sizeof(acBuffer))
		== 18,
		"ip_prefix_to_string() should return 18");
  ASSERT_RETURN(strcmp(acBuffer, "255.255.255.255/32") == 0,
		"string for prefix 255.255.255.255/32 should "
		"be \"255.255.255.255/32\"");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_str2 ]-------------------------------
static int test_net_attr_prefix4_str2()
{
  SPrefix sPrefix;
  char * pcEndPtr;
  ASSERT_RETURN((ip_string_to_prefix("192.168.0.0/16", &pcEndPtr, &sPrefix) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"192.168.0.0/16\" should be valid");
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(192,168,0,0)) &&
		(sPrefix.uMaskLen == 16),
		"prefix should be 192.168.0.0/16");
  ASSERT_RETURN((ip_string_to_prefix("192.168/16", &pcEndPtr, &sPrefix) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"192.168/16\" should be valid");
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(192,168,0,0)) &&
		(sPrefix.uMaskLen == 16),
		"prefix should be 192.168.0.0/16");
  ASSERT_RETURN((ip_string_to_prefix("192.168.1.2/28", &pcEndPtr, &sPrefix) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"192.168.1.2/28\" should be valid");
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(192,168,1,2)) &&
		(sPrefix.uMaskLen == 28),
		"prefix should be 192.168.1.2/28");
  ASSERT_RETURN((ip_string_to_prefix("255.255.255.255/32", &pcEndPtr, &sPrefix) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"255.255.255.255/32\" should be valid");
  ASSERT_RETURN((sPrefix.tNetwork == IPV4_TO_INT(255,255,255,255)) &&
		(sPrefix.uMaskLen == 32),
		"prefix should be 255.255.255.255/32");
  ASSERT_RETURN((ip_string_to_prefix("192.168.1.2/16a", &pcEndPtr, &sPrefix) >= 0) && (*pcEndPtr == 'a'),
		"prefix should be valid (but followed by stuff)");
  ASSERT_RETURN((ip_string_to_prefix("192.168a.1.2/16", &pcEndPtr, &sPrefix) < 0),
		"prefix should be invalid");
  ASSERT_RETURN((ip_string_to_prefix("256/8", &pcEndPtr, &sPrefix) < 0),
		"prefix should be invalid");
  ASSERT_RETURN((ip_string_to_prefix("255.255.255.255.255/8", &pcEndPtr, &sPrefix) < 0),
		"prefix should be invalid");
  ASSERT_RETURN((ip_string_to_prefix("255.255.255.255/99", &pcEndPtr, &sPrefix) < 0),
		"prefix should be invalid");
  ASSERT_RETURN((ip_string_to_prefix("255.255", &pcEndPtr, &sPrefix) < 0),
		"prefix should be invalid");
  ASSERT_RETURN((ip_string_to_prefix("255.255./16", &pcEndPtr, &sPrefix) < 0),
		"prefix should be invalid");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_cmp ]--------------------------------
static int test_net_attr_prefix4_cmp()
{
  SPrefix sPrefix1= { .tNetwork= IPV4_TO_INT(1,2,3,4),
		      .uMaskLen= 5 };
  SPrefix sPrefix2= sPrefix1;
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == 0,
		"prefixes should be considered equal");
  sPrefix2.tNetwork= IPV4_TO_INT(1,2,3,3);
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == 1,
		"equal mask, higher network should be considered better");
  sPrefix2.tNetwork= sPrefix1.tNetwork;
  sPrefix2.uMaskLen= 6;
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == -1,
		"longest mask should be considered better");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_igpweight ]----------------------------------
static int test_net_attr_igpweight()
{
  SNetIGPWeights * pWeights= net_igp_weights_create(2);
  ASSERT_RETURN(pWeights != NULL,
		"net_igp_weights_create() should not return NULL");
  ASSERT_RETURN(net_igp_weights_depth(pWeights) == 2,
		"IGP weights'depth should be 2");
  net_igp_weights_destroy(&pWeights);
  ASSERT_RETURN(pWeights == NULL,
		"destroyed IGP weights should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_igpweight_add ]------------------------------
static int test_net_attr_igpweight_add()
{
  ASSERT_RETURN(net_igp_add_weights(5, 10) == 15,
		"IGP weights 5 and 10 should add to 15");
  ASSERT_RETURN(net_igp_add_weights(5, NET_IGP_MAX_WEIGHT)
		== NET_IGP_MAX_WEIGHT,
		"IGP weights 5 and 10 should add to %d", NET_IGP_MAX_WEIGHT);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET LINKS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_link ]--------------------------------------------
static int test_net_link()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetLink * pLink= NULL;
  ASSERT_RETURN((net_link_create_ptp(pNode1, pNode2, 10, 20, 1, &pLink)
		 == NET_SUCCESS) &&
		(pLink != NULL),
		"link creation should succeed");
  ASSERT_RETURN(pLink->uType == NET_LINK_TYPE_ROUTER,
		"link type is not correct");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pLink->tDest.pNode == pNode2,
		"destination node is not correct");
  ASSERT_RETURN(net_link_get_state(pLink, NET_LINK_FLAG_UP) != 0,
		"link should be up");
  ASSERT_RETURN((net_link_get_delay(pLink) == 10) &&
		(net_link_get_capacity(pLink) == 20) &&
		(net_link_get_load(pLink) == 0),
		"link attributes are not correct");
  net_link_destroy(&pLink);
  ASSERT_RETURN(pLink == NULL,
		"destroyed link should be NULL");
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_forward ]------------------------------------
static int test_net_link_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetNode * pNextHop;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  SNetLink * pLink= NULL;
  ASSERT_RETURN(net_link_create_ptp(pNode1, pNode2, 0, 0, 1, &pLink)
		== NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(pLink->fForward(0, pLink, &pNextHop, &pMsg) == NET_SUCCESS,
		"link forward should succeed");
  ASSERT_RETURN(pNextHop == pNode2,
		"link target is incorrect");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  net_link_destroy(&pLink);
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_forward_down ]-------------------------------
static int test_net_link_forward_down()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetNode * pNextHop= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  SNetLink * pLink= NULL;
  ASSERT_RETURN(net_link_create_ptp(pNode1, pNode2, 0, 0, 1, &pLink)
		== NET_SUCCESS,
		"link creation should succeed");
  net_link_set_state(pLink, NET_LINK_FLAG_UP, 0);
  ASSERT_RETURN(pLink->fForward(0, pLink, &pNextHop, &pMsg)
		== NET_ERROR_LINK_DOWN,
		"link forward should fail");
  ASSERT_RETURN(pNextHop == NULL,
		"link target should be unchanged (NULL)");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  net_link_destroy(&pLink);
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_subnet ]-------------------------------------
static int test_net_link_subnet()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_TRANSIT);
  SNetLink * pLink= NULL;
  ASSERT_RETURN((net_link_create_mtp(pNode1, pSubnet, IPV4_TO_INT(192,168,0,1),
				     10, 20, 1, &pLink) == NET_SUCCESS) &&
		(pLink != NULL),
		"link creation should succeed");
  ASSERT_RETURN(pLink->uType == NET_LINK_TYPE_TRANSIT,
		"link type is not correct");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pLink->tDest.pSubnet == pSubnet,
		"destination subnet is not correct");
  ASSERT_RETURN(pLink->tIfaceAddr == IPV4_TO_INT(192,168,0,1),
		"interface address is not correct");
  ASSERT_RETURN(net_link_get_state(pLink, NET_LINK_FLAG_UP) != 0,
		"link should be up");
  ASSERT_RETURN((net_link_get_delay(pLink) == 10) &&
		(net_link_get_capacity(pLink) == 20) &&
		(net_link_get_load(pLink) == 0),
		"link attributes are not correct");
  net_link_destroy(&pLink);
  ASSERT_RETURN(pLink == NULL,
		"destroyed link should be NULL");
  subnet_destroy(&pSubnet);
  ASSERT_RETURN(pSubnet == NULL,
		"destroyed subnet should be NULL");
  node_destroy(&pNode1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_subnet_forward ]-----------------------------
static int test_net_link_subnet_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_TRANSIT);
  SNetLink * pLink1= NULL;
  SNetLink * pLink2= NULL;
  SNetNode * pNextHop= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  ASSERT_RETURN(net_link_create_mtp(pNode1, pSubnet, IPV4_TO_INT(192,168,0,1),
				    10, 20, 1, &pLink1) == NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(subnet_add_link(pSubnet, pLink1) == NET_SUCCESS,
		"addition of link to subnet should succeed");
  ASSERT_RETURN(net_link_create_mtp(pNode2, pSubnet, IPV4_TO_INT(192,168,0,2),
				    10, 20, 1, &pLink2) == NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(subnet_add_link(pSubnet, pLink2) == NET_SUCCESS,
		"addition of link to subnet should succeed");
  ASSERT_RETURN(pLink1->fForward(IPV4_TO_INT(192,168,0,2), pLink1,
				  &pNextHop, &pMsg) == NET_SUCCESS,
		"link forward should succeed");
  ASSERT_RETURN(pNextHop == pNode2,
		"link target is incorrect");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  net_link_destroy(&pLink1);
  net_link_destroy(&pLink2);
  subnet_destroy(&pSubnet);
  node_destroy(&pNode1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_subnet_forward_unreach ]---------------------
static int test_net_link_subnet_forward_unreach()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_TRANSIT);
  SNetLink * pLink1= NULL;
  SNetNode * pNextHop= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  ASSERT_RETURN(net_link_create_mtp(pNode1, pSubnet, IPV4_TO_INT(192,168,0,1),
				    10, 20, 1, &pLink1) == NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(subnet_add_link(pSubnet, pLink1) == NET_SUCCESS,
		"addition of link to subnet should succeed");
  ASSERT_RETURN(pNextHop == NULL,
		"link target should be unchanged (NULL)");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");  
  net_link_destroy(&pLink1);
  subnet_destroy(&pSubnet);
  node_destroy(&pNode1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_tunnel ]-------------------------------------
static int test_net_link_tunnel()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetLink * pLink= NULL;
  SNetLink * pTunnel= NULL;
  ASSERT_RETURN(node_add_link_ptp(pNode1, pNode2, 10, 20, 1, 1)
		== NET_SUCCESS,
		"node_add_link_ptp() should succeed");
  pLink= node_find_link_ptp(pNode1, pNode2->tAddr);
  ASSERT_RETURN(pLink != NULL,
		"node_find_link_ptp() should not return NULL");
  ASSERT_RETURN((ipip_link_create(pNode1, IPV4_TO_INT(2,0,0,0),
				  IPV4_TO_INT(127,0,0,1),
				  pLink,
				  NET_ADDR_ANY,
				  &pTunnel) == NET_SUCCESS) &&
		(pTunnel != NULL),
		"ipip_link_create() should succeed");
  ASSERT_RETURN(pTunnel->uType == NET_LINK_TYPE_TUNNEL,
		"link type is incorrect");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pTunnel->tDest.tEndPoint == pNode2->tAddr,
		"tunnel endpoint is incorrect");
  ASSERT_RETURN(pTunnel->tIfaceAddr == IPV4_TO_INT(127,0,0,1),
		"interface address is incorrect");
  ASSERT_RETURN(net_link_get_state(pLink, NET_LINK_FLAG_UP) != 0,
		"link should be up");
  net_link_destroy(&pLink);
  ASSERT_RETURN(pLink == NULL,
		"destroyed link should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_link_tunnel_forward ]-----------------------------
static int test_net_link_tunnel_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetLink * pLink= NULL;
  SNetLink * pTunnel= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  SNetNode * pNextHop= NULL;
  ASSERT_RETURN(node_add_link_ptp(pNode1, pNode2, 10, 20, 1, 1)
		== NET_SUCCESS,
		"node_add_link_ptp() should succeed");
  pLink= node_find_link_ptp(pNode1, pNode2->tAddr);
  ASSERT_RETURN(pLink != NULL,
		"node_find_link_ptp() should not return NULL");
  ASSERT_RETURN((ipip_link_create(pNode1, IPV4_TO_INT(2,0,0,0),
				 IPV4_TO_INT(127,0,0,1),
				 pLink,
				 NET_ADDR_ANY,
				  &pTunnel) == NET_SUCCESS) &&
		(pTunnel != NULL),
		"ipip_link_create() should succeed");
  ASSERT_RETURN(pTunnel != NULL,
		"");
  ASSERT_RETURN(pTunnel->fForward(NET_ADDR_ANY, pTunnel->pContext,
				  &pNextHop, &pMsg) == NET_SUCCESS,
		"tunnel forward should succeed");
  ASSERT_RETURN(pNextHop == pNode2,
		"link target is incorrect");
  ASSERT_RETURN((pMsg != (SNetMessage *) 12345) &&
		(pMsg->pPayLoad == (void *) 12345),
		"initial message should be encapsulated");
  net_link_destroy(&pLink);
  ASSERT_RETURN(pLink == NULL,
		"destroyed link should be NULL");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_attr_aspath ]-------------------------------------
static int test_bgp_attr_aspath()
{
  SBGPPath * pPath= path_create();
  ASSERT_RETURN(path_length(pPath) == 0,
		"new path must have 0 length");
  path_destroy(&pPath);
  ASSERT_RETURN(pPath == NULL,
		"destroyed path must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_prepend ]-----------------------------
static int test_bgp_attr_aspath_prepend()
{
  SBGPPath * pPath= path_create();
  ASSERT_RETURN(path_append(&pPath, 1) >= 0,
		"path_prepend() should return >= 0 on success");
  ASSERT_RETURN(path_length(pPath) == 1,
		"prepended path must have length of 1");
  path_destroy(&pPath);
  ASSERT_RETURN(pPath == NULL,
		"destroyed path must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_prepend_too_much ]--------------------
static int test_bgp_attr_aspath_prepend_too_much()
{
  unsigned int uIndex;
  SBGPPath * pPath= path_create();
  for (uIndex= 0; uIndex < MAX_PATH_SEQUENCE_LENGTH; uIndex++) {
    ASSERT_RETURN(path_append(&pPath, uIndex) >= 0,
		  "path_prepend() should return >= 0 on success");
  }
  ASSERT_RETURN(path_append(&pPath, 256) < 0,
		  "path_prepend() should return < 0 on failure");
  path_destroy(&pPath);
  ASSERT_RETURN(pPath == NULL,
		"destroyed path must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_2str ]--------------------------------
static int test_bgp_attr_aspath_2str()
{
  int iResult;
  SBGPPath * pPath= NULL;
  char acBuffer[5]= "PLOP";
  // Empty path
  ASSERT_RETURN(path_to_string(pPath, 1, acBuffer, sizeof(acBuffer)) == 0,
		"path_to_string() should return 0");
  ASSERT_RETURN(strcmp(acBuffer, "") == 0,
		"string for null path must be \"\"");
  // Small path (fits in buffer)
  pPath= path_create();
  path_append(&pPath, 2611);
  iResult= path_to_string(pPath, 1, acBuffer, sizeof(acBuffer));
  ASSERT_RETURN(iResult == 4,
		"path_to_string() should return 4 (%d)", iResult);
  ASSERT_RETURN(strcmp(acBuffer, "2611") == 0,
		"string for path 2611 must be \"2611\"");
  // Larger path (does not fit in buffer)
  path_append(&pPath, 20965);
  iResult= path_to_string(pPath, 1, acBuffer, sizeof(acBuffer));
  ASSERT_RETURN(iResult < 0,
		"path_to_string() should return < 0 (%d)", iResult);
  path_destroy(&pPath);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_str2 ]--------------------------------
static int test_bgp_attr_aspath_str2()
{
  SBGPPath * pPath;
  pPath= path_from_string("");
  ASSERT_RETURN(pPath != NULL,
		"\"\" is a valid AS-Path");
  //ASSERT_RETURN(pPath);
  path_destroy(&pPath);
  pPath= path_from_string("12 34 56");
  ASSERT_RETURN(pPath != NULL,
		"\"12 34 56\" is a valid AS-Path");
  path_destroy(&pPath);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_cmp ]---------------------------------
static int test_bgp_attr_aspath_cmp()
{
  SBGPPath * pPath1= NULL, * pPath2= NULL;
  ASSERT_RETURN(path_cmp(pPath1, pPath2) == 0,
		"null paths should be considered equal");
  pPath1= path_create();
  ASSERT_RETURN(path_cmp(pPath1, pPath2) == 0,
		"null and empty paths should be considered equal");
  pPath2= path_create();
  ASSERT_RETURN(path_cmp(pPath1, pPath2) == 0,
		"empty paths should be considered equal");
  path_append(&pPath1, 1);
  ASSERT_RETURN(path_cmp(pPath1, pPath2) == 1,
		"longest path should be considered better");
  path_append(&pPath2, 2);
  ASSERT_RETURN(path_cmp(pPath1, pPath2) == -1,
		"for equal-size paths, should be considered better");
  path_destroy(&pPath1);
  path_destroy(&pPath2);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_contains ]----------------------------
static int test_bgp_attr_aspath_contains()
{
  SBGPPath * pPath= NULL;
  ASSERT_RETURN(path_contains(pPath, 1) == 0,
		"null path should not contain 1");
  pPath= path_create();
  ASSERT_RETURN(path_contains(pPath, 1) == 0,
		"empty path should not contain 1");
  path_append(&pPath, 2);
  ASSERT_RETURN(path_contains(pPath, 1) == 0,
		"path 2 should not contain 1");
  path_append(&pPath, 1);
  ASSERT_RETURN(path_contains(pPath, 1) != 0,
		"path 2 1 should contain 1");
  path_destroy(&pPath);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities ]--------------------------------
static int test_bgp_attr_communities()
{
  SCommunities * pComms= comm_create();
  comm_destroy(&pComms);
  ASSERT_RETURN(pComms == NULL, "destroyed Communities must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_append ]-------------------------
static int test_bgp_attr_communities_append()
{
  SCommunities * pComms= comm_create();
  ASSERT_RETURN(comm_contains(pComms, 1) == 0,
		"comm_contains() should return == 0");
  ASSERT_RETURN(comm_add(&pComms, 1) == 0,
		"comm_add() should return 0 on success");
  ASSERT_RETURN(pComms->uNum == 1,
		"Communities length should be 1");
  ASSERT_RETURN(comm_contains(pComms, 1) != 0,
		"comm_contains() should return != 0");
  comm_destroy(&pComms);
  ASSERT_RETURN(pComms == NULL, "destroyed Communities must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_remove ]-------------------------
static int test_bgp_attr_communities_remove()
{
  SCommunities * pComms= comm_create();
  ASSERT_RETURN(comm_add(&pComms, 1) == 0,
		"comm_add() should return 0 on success");
  ASSERT_RETURN(pComms->uNum == 1,
		"Communities length should be 1");
  ASSERT_RETURN(comm_add(&pComms, 2) == 0,
		"comm_add() should return 0 on success");
  ASSERT_RETURN(pComms->uNum == 2,
		"Communities length should be 2");
  comm_remove(&pComms, 1);
  ASSERT_RETURN(pComms->uNum == 1,
		"Communities length should be 1");
  comm_remove(&pComms, 2);
  ASSERT_RETURN(pComms == NULL,
		"Communities should be destroyed when empty");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_2str ]---------------------------
static int test_bgp_attr_communities_2str()
{
  SCommunities * pComms= NULL;
  char acBuffer[10]= "PLOP";
  // Empty Communities
  ASSERT_RETURN(comm_to_string(pComms, acBuffer, sizeof(acBuffer)) == 0,
		"comm_to_string() should return 0");
  ASSERT_RETURN(strcmp(acBuffer, "") == 0,
		"string for null Communities should be \"\"");
  // Small Communities (fits in buffer)
  pComms= comm_create();
  comm_add(&pComms, 1234);
  ASSERT_RETURN(comm_to_string(pComms, acBuffer, sizeof(acBuffer)) == 4,
		"comm_to_string() should return 4");
  ASSERT_RETURN(strcmp(acBuffer, "1234") == 0,
		"string for Communities 1234 should be \"1234\"");
  // Larger Communities (still fits in buffer)
  comm_add(&pComms, 2345);
  ASSERT_RETURN(comm_to_string(pComms, acBuffer, sizeof(acBuffer)) == 9,
		"comm_to_string() should return == 9");
  ASSERT_RETURN(strcmp(acBuffer, "1234 2345") == 0,
		"string for Communities 1234,2345 should be \"1234 2345\"");
  // Even larger Communities (does not fit in buffer)
  comm_add(&pComms, 3456);
  ASSERT_RETURN(comm_to_string(pComms, acBuffer, sizeof(acBuffer)) < 0,
		"comm_to_string() should return < 0");
  comm_destroy(&pComms);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_str2 ]---------------------------
static int test_bgp_attr_communities_str2()
{
  SCommunities * pComms;
  pComms= comm_from_string("");
  ASSERT_RETURN(pComms != NULL,
		"\"\" is a valid Communities");
  comm_destroy(&pComms);
  pComms= comm_from_string("1234:5678");
  ASSERT_RETURN(pComms != NULL,
		"\"1234:5678\" is a valid Communities");
  comm_destroy(&pComms);
  pComms= comm_from_string("65535:65535");
  ASSERT_RETURN(pComms != NULL,
		"\"1234:5678\" is a valid Communities");
  comm_destroy(&pComms);
  pComms= comm_from_string("12345678");
  ASSERT_RETURN(pComms != NULL,
		"\"12345678\" is a valid Communities");
  comm_destroy(&pComms);
  pComms= comm_from_string("4294967295");
  ASSERT_RETURN(pComms != NULL,
		"\"4294967295\" is a valid Communities");
  comm_destroy(&pComms);
  pComms= comm_from_string("1234:5678abc");
  ASSERT_RETURN(pComms == NULL,
		"\"1234:5678abc\" is not a valid Communities");
  pComms= comm_from_string("4294967296");
  ASSERT_RETURN(pComms == NULL,
		"\"4294967296\" is not a valid Communities");
  pComms= comm_from_string("65536:0");
  ASSERT_RETURN(pComms == NULL,
		"\"65536:0\" is not a valid Communities");
  pComms= comm_from_string("0:65536");
  ASSERT_RETURN(pComms == NULL,
		"\"0:65536\" is not a valid Communities");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_cmp ]----------------------------
static int test_bgp_attr_communities_cmp()
{
  SCommunities * pComms1= NULL, * pComms2= NULL;
  ASSERT_RETURN(comm_cmp(pComms1, pComms2) == 0,
		"null Communities should be considered equal");
  pComms1= comm_create();
  ASSERT_RETURN(comm_cmp(pComms1, pComms2) == 0,
		"null and empty Communities should be considered equal");
  pComms2= comm_create();
  ASSERT_RETURN(comm_cmp(pComms1, pComms2) == 0,
		"empty Communities should be considered equal");
  comm_add(&pComms1, 1);
  ASSERT_RETURN(comm_cmp(pComms1, pComms2) == 1,
		"longest Communities should be considered first");
  comm_add(&pComms2, 2);
  ASSERT_RETURN(comm_cmp(pComms1, pComms2) == -1,
		"in equal-size Communities, first largest value should win");
  comm_destroy(&pComms1);
  comm_destroy(&pComms2);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_contains ]-----------------------
static int test_bgp_attr_communities_contains()
{
  SCommunities * pComms= NULL;
  ASSERT_RETURN(comm_contains(pComms, 1) == 0,
		"null Communities should not contain 1");
  pComms= comm_create();
  ASSERT_RETURN(comm_contains(pComms, 1) == 0,
		"empty Communities should not contain 1");
  comm_add(&pComms, 2);
  ASSERT_RETURN(comm_contains(pComms, 1) == 0,
		"Communities 2 should not contain 1");
  comm_add(&pComms, 1);
  ASSERT_RETURN(comm_contains(pComms, 1) != 0,
		"Communities 2 1 should contain 1");
  comm_destroy(&pComms);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_cluster_list ]-------------------------------
static int test_bgp_attr_cluster_list()
{
  SClusterList * pClustList= cluster_list_create();
  ASSERT_RETURN(pClustList != NULL,
		"new Cluster-Id-List should not be NULL");
  cluster_list_destroy(&pClustList);
  ASSERT_RETURN(pClustList == NULL,
		"Cluster-Id-List should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_cluster_list_append ]------------------------
static int test_bgp_attr_cluster_list_append()
{
  SClusterList * pClustList= cluster_list_create();
  ASSERT_RETURN(pClustList != NULL,
		"new Cluster-Id-List should not be NULL");
  ASSERT_RETURN(cluster_list_append(pClustList, IPV4_TO_INT(1,2,3,4)) >= 0,
		"cluster_list_append() should succeed");
  ASSERT_RETURN(cluster_list_length(pClustList) == 1,
		"Cluster-Id-List length should be 1");
  ASSERT_RETURN(cluster_list_append(pClustList, IPV4_TO_INT(5,6,7,8)) >= 0,
		"cluster_list_append() should succeed");
  ASSERT_RETURN(cluster_list_length(pClustList) == 2,
		"Cluster-Id-List length should be 2");
  ASSERT_RETURN(cluster_list_append(pClustList, IPV4_TO_INT(9,0,1,2)) >= 0,
		"cluster_list_append() should succeed");
  ASSERT_RETURN(cluster_list_length(pClustList) == 3,
		"Cluster-Id-List length should be 3");
  ASSERT_RETURN(cluster_list_append(pClustList, IPV4_TO_INT(3,4,5,6)) >= 0,
		"cluster_list_append() should succeed");
  ASSERT_RETURN(cluster_list_length(pClustList) == 4,
		"Cluster-Id-List length should be 4");
  cluster_list_destroy(&pClustList);
  ASSERT_RETURN(pClustList == NULL,
		"Cluster-Id-List should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_cluster_list_contains ]----------------------
static int test_bgp_attr_cluster_list_contains()
{
  SClusterList * pClustList= cluster_list_create();
  cluster_list_append(pClustList, IPV4_TO_INT(1,2,3,4));
  cluster_list_append(pClustList, IPV4_TO_INT(5,6,7,8));
  cluster_list_append(pClustList, IPV4_TO_INT(9,0,1,2));
  ASSERT_RETURN(cluster_list_length(pClustList) == 3,
		"Cluster-Id-List length should be 3");
  ASSERT_RETURN(cluster_list_contains(pClustList, IPV4_TO_INT(1,2,3,4)) != 0,
		"Cluster-Id-List should contains 1.2.3.4");
  ASSERT_RETURN(cluster_list_contains(pClustList, IPV4_TO_INT(5,6,7,8))	!= 0,
		"Cluster-Id-List should contains 5.6.7.8");
  ASSERT_RETURN(cluster_list_contains(pClustList, IPV4_TO_INT(9,0,1,2)) != 0,
		"Cluster-Id-List should contains 9.0.1.2");
  ASSERT_RETURN(cluster_list_contains(pClustList, IPV4_TO_INT(2,1,0,9)) == 0,
		"Cluster-Id-List should not contain 2.1.0.9");
  ASSERT_RETURN(cluster_list_contains(pClustList, IPV4_TO_INT(0,0,0,0)) == 0,
		"Cluster-Id-List should not contain 0.0.0.0");
  ASSERT_RETURN(cluster_list_contains(pClustList, IPV4_TO_INT(255,255,255,255))
		== 0,
		"Cluster-Id-List should not contain 255.255.255.255");
  cluster_list_destroy(&pClustList);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP ROUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_route_basic ]-------------------------------------
static int test_bgp_route_basic()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(130,104,0,0),
		     .uMaskLen= 16 };
  SRoute * pRoute= route_create(sPrefix, NULL, IPV4_TO_INT(1,0,0,0),
				ROUTE_ORIGIN_IGP);
  ASSERT_RETURN(pRoute != NULL,
		"Route should not be NULL when created");
  route_destroy(&pRoute);
  ASSERT_RETURN(pRoute == NULL,
		"Route should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_route_communities ]-------------------------------
static int test_bgp_route_communities()
{
  SRoute * pRoute1, * pRoute2;
  SCommunities * pComm1, * pComm2;

  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(130,104,0,0),
		     .uMaskLen= 16 };

  pRoute1= route_create(sPrefix, NULL, IPV4_TO_INT(1,0,0,0), ROUTE_ORIGIN_IGP);
  pRoute2= route_create(sPrefix, NULL, IPV4_TO_INT(2,0,0,0), ROUTE_ORIGIN_IGP);

  // Copy of Communities must be different
  pComm1= comm_create();
  pComm2= comm_copy(pComm1);
  ASSERT_RETURN(pComm1 != pComm2,
		"Communities copy should not be equal to original");

  // Communities must not exist in global repository
  ASSERT_RETURN((comm_hash_get(pComm1) == NULL) &&
		(comm_hash_get(pComm2) == NULL),
		"Communities should not already be interned");

  // Communities must now be interned and equal
  route_set_comm(pRoute1, pComm1);
  route_set_comm(pRoute2, pComm2);
  ASSERT_RETURN(pRoute1->pAttr->pCommunities == pRoute2->pAttr->pCommunities,
		"interned Communities not equal");
  ASSERT_RETURN((comm_hash_get(pRoute1->pAttr->pCommunities) ==
		 pRoute1->pAttr->pCommunities) &&
		(comm_hash_get(pRoute2->pAttr->pCommunities) ==
		 pRoute2->pAttr->pCommunities),
		"Communities <-> hash mismatch");
  ASSERT_RETURN(comm_hash_refcnt(pRoute1->pAttr->pCommunities) == 2,
		"Incorrect reference count");

  // Append community value
  route_comm_append(pRoute1, 12345);
  ASSERT_RETURN(pRoute1->pAttr->pCommunities != pRoute2->pAttr->pCommunities,
		"interned Communities are equal");
  ASSERT_RETURN((comm_hash_get(pRoute1->pAttr->pCommunities) ==
		 pRoute1->pAttr->pCommunities) &&
		(comm_hash_get(pRoute2->pAttr->pCommunities) ==
		 pRoute2->pAttr->pCommunities),
		"Communities <-> hash mismatch");

  // Append Community value
  route_comm_append(pRoute2, 12345);
  ASSERT_RETURN(pRoute1->pAttr->pCommunities == pRoute2->pAttr->pCommunities,
		"interned Communities not equal");
  ASSERT_RETURN((comm_hash_get(pRoute1->pAttr->pCommunities) ==
		 pRoute1->pAttr->pCommunities) &&
		(comm_hash_get(pRoute2->pAttr->pCommunities) ==
		 pRoute2->pAttr->pCommunities),
		"Communities <-> hash mismatch");

  route_destroy(&pRoute1);
  route_destroy(&pRoute2);

  return EXIT_SUCCESS;
}

// -----[ test_bgp_route_aspath ]------------------------------------
static int test_bgp_route_aspath()
{
  SRoute * pRoute1, * pRoute2;
  SBGPPath * pPath1, * pPath2;

  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(130,104,0,0),
		     .uMaskLen= 16 };

  pRoute1= route_create(sPrefix, NULL, IPV4_TO_INT(1,0,0,0), ROUTE_ORIGIN_IGP);
  pRoute2= route_create(sPrefix, NULL, IPV4_TO_INT(2,0,0,0), ROUTE_ORIGIN_IGP);

  pPath1= path_create();
  pPath2= path_copy(pPath1);
  ASSERT_RETURN(pPath1 != pPath2,
		"AS-Path copy should not be equal to original");

  route_set_path(pRoute1, pPath1);
  route_set_path(pRoute2, pPath2);
  ASSERT_RETURN(pRoute1->pAttr->pASPathRef == pRoute2->pAttr->pASPathRef,
		"AS-Paths should not already be internet");

  // Prepend first route's AS-Path
  route_path_prepend(pRoute1, 666, 2);
  ASSERT_RETURN(pRoute1->pAttr->pASPathRef != pRoute2->pAttr->pASPathRef,
		"");

  // Prepend second route's AS-Path
  route_path_prepend(pRoute2, 666, 2);
  ASSERT_RETURN(pRoute1->pAttr->pASPathRef == pRoute2->pAttr->pASPathRef,
		"");

  // Destroy route 1
  route_destroy(&pRoute1);

  // Destroy route 2
  route_destroy(&pRoute2);

  return UTEST_SUCCESS;
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

#define ARRAY_SIZE(A) sizeof(A)/sizeof(A[0])

SUnitTest TEST_NET_ATTR[]= {
  {test_net_attr_address4, "IPv4 address"},
  {test_net_attr_address4_2str, "IPv4 address (-> string)"},
  {test_net_attr_address4_str2, "IPv4 address (<- string)"},
  {test_net_attr_prefix4, "IPv4 prefix"},
  {test_net_attr_prefix4_in, "IPv4 prefix in prefix"},
  {test_net_attr_prefix4_match, "IPv4 prefix match"},
  {test_net_attr_prefix4_mask, "IPv4 prefix mask"},
  {test_net_attr_prefix4_2str, "IPv4 prefix (-> string)"},
  {test_net_attr_prefix4_str2, "IPv4 prefix (<- string)"},
  {test_net_attr_prefix4_cmp, "IPv4 prefix (compare)"},
  {test_net_attr_igpweight, "IGP weight"},
  {test_net_attr_igpweight_add, "IGP weight add"},
};
#define TEST_NET_ATTR_SIZE ARRAY_SIZE(TEST_NET_ATTR)

SUnitTest TEST_NET_LINK[]= {
  {test_net_link, "link"},
  {test_net_link_forward, "link forward"},
  {test_net_link_forward_down, "link forward (down)"},
  {test_net_link_subnet, "subnet"},
  {test_net_link_subnet_forward, "subnet forward"},
  {test_net_link_subnet_forward_unreach, "subnet forward (unreach)"},
  {test_net_link_tunnel, "tunnel"},
  {test_net_link_tunnel_forward, "tunnel forward"},
};
#define TEST_NET_LINK_SIZE ARRAY_SIZE(TEST_NET_LINK)

SUnitTest TEST_BGP_ATTR[]= {
  {test_bgp_attr_aspath, "as-path"},
  {test_bgp_attr_aspath_prepend, "as-path prepend"},
  {test_bgp_attr_aspath_prepend_too_much, "as-path prepend (too much)"},
  {test_bgp_attr_aspath_2str, "as-path (-> string)"},
  {test_bgp_attr_aspath_str2, "as-path (<- string)"},
  {test_bgp_attr_aspath_cmp, "as-path (compare)"},
  {test_bgp_attr_aspath_contains, "as-path (contains)"},
  {test_bgp_attr_communities, "communities"},
  {test_bgp_attr_communities_append, "communities append"},
  {test_bgp_attr_communities_remove, "communities remove"},
  {test_bgp_attr_communities_2str, "communities (-> string)"},
  {test_bgp_attr_communities_str2, "communities (<- string)"},
  {test_bgp_attr_communities_cmp, "communities (compare)"},
  {test_bgp_attr_communities_contains, "communities (contains)"},
  {test_bgp_attr_cluster_list, "cluster-id-list"},
  {test_bgp_attr_cluster_list_append, "cluster-id-list append"},
  {test_bgp_attr_cluster_list_contains, "cluster-id-list contains"},
};
#define TEST_BGP_ATTR_SIZE ARRAY_SIZE(TEST_BGP_ATTR)

SUnitTest TEST_BGP_ROUTE[]= {
  {test_bgp_route_basic, "basic"},
  {test_bgp_route_communities, "attr-communities"},
  {test_bgp_route_aspath, "attr-aspath"},
};
#define TEST_BGP_ROUTE_SIZE ARRAY_SIZE(TEST_BGP_ROUTE)

SUnitTestSuite TEST_SUITES[]= {
  {"Net Attributes", TEST_NET_ATTR_SIZE, TEST_NET_ATTR},
  {"Net Links", TEST_NET_LINK_SIZE, TEST_NET_LINK},
  {"BGP Attributes", TEST_BGP_ATTR_SIZE, TEST_BGP_ATTR},
  {"BGP Routes", TEST_BGP_ROUTE_SIZE, TEST_BGP_ROUTE},
};
#define TEST_SUITES_SIZE ARRAY_SIZE(TEST_SUITES)

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  int iResult;

  libcbgp_banner();

  utest_init(0);
  utest_set_fork();
  utest_set_user(getenv("USER"));
  utest_set_project(PACKAGE_NAME, PACKAGE_VERSION);
  utest_set_xml_logging("cbgp-internal-check.xml");
  iResult= utest_run_suites(TEST_SUITES, TEST_SUITES_SIZE);
  
  utest_done();
  
  return (iResult==0?EXIT_SUCCESS:EXIT_FAILURE);
}
