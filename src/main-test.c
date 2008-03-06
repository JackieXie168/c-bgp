// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/11/07
// @lastdate 27/02/2008
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
#include <bgp/filter_parser.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/path_hash.h>
#include <bgp/path_segment.h>
#include <bgp/peer.h>
#include <bgp/predicate_parser.h>
#include <bgp/route.h>
#include <bgp/route-input.h>
#include <net/error.h>
#include <net/igp_domain.h>
#include <net/iface.h>
//#include <net/ifaces_list.h>
#include <net/link-list.h>
#include <net/ipip.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/record-route.h>
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
  ASSERT_RETURN((ip_string_to_address("1.2.3.4.5", &pcEndPtr, &tAddress) == 0) &&
		(*pcEndPtr == '.'),
		"address \"1.2.3.4.5\" should be invalid");
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

// -----[ test_net_attr_prefix4_ge ]---------------------------------
static int test_net_attr_prefix4_ge()
{
  SPrefix sPrefix1= { .tNetwork= IPV4_TO_INT(192,168,0,0),
		      .uMaskLen= 16 };
  SPrefix sPrefix2= { .tNetwork= IPV4_TO_INT(192,168,1,0),
		      .uMaskLen= 24 };
  SPrefix sPrefix3= { .tNetwork= IPV4_TO_INT(192,168,0,0),
		      .uMaskLen= 15 };
  SPrefix sPrefix4= { .tNetwork= IPV4_TO_INT(192,169,0,0),
		      .uMaskLen= 24 };
  ASSERT_RETURN(ip_prefix_ge_prefix(sPrefix1, sPrefix1, 16),
		"192.168/16 should be ge (192.168/16, 16)");
  ASSERT_RETURN(ip_prefix_ge_prefix(sPrefix2, sPrefix1, 16),
		"192.168.1/24 should be ge (192.168/16, 16)");
  ASSERT_RETURN(!ip_prefix_ge_prefix(sPrefix3, sPrefix1, 16),
		"192.168/15 should not be ge (192.168/16, 16)");
  ASSERT_RETURN(!ip_prefix_ge_prefix(sPrefix4, sPrefix1, 16),
		"192.169.0/24 should not be ge (192.168/16, 16)");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_le ]---------------------------------
static int test_net_attr_prefix4_le()
{
  SPrefix sPrefix1= { .tNetwork= IPV4_TO_INT(192,168,0,0),
		      .uMaskLen= 16 };
  SPrefix sPrefix2= { .tNetwork= IPV4_TO_INT(192,168,1,0),
		      .uMaskLen= 24 };
  SPrefix sPrefix3= { .tNetwork= IPV4_TO_INT(192,168,1,0),
		      .uMaskLen= 25 };
  SPrefix sPrefix4= { .tNetwork= IPV4_TO_INT(192,169,0,0),
		      .uMaskLen= 24 };
  SPrefix sPrefix5= { .tNetwork= IPV4_TO_INT(192,168,0,0),
		      .uMaskLen= 15 };
  ASSERT_RETURN(ip_prefix_le_prefix(sPrefix2, sPrefix1, 24),
		"192.168.1/24 should be le (192.168/16, 24)");
  ASSERT_RETURN(!ip_prefix_le_prefix(sPrefix3, sPrefix1, 24),
		"192.168.0.0/25 should not be le (192.168/16, 24)");
  ASSERT_RETURN(!ip_prefix_le_prefix(sPrefix4, sPrefix1, 24),
		"192.169/24 should not be le (192.168/16, 24)");
  ASSERT_RETURN(ip_prefix_le_prefix(sPrefix1, sPrefix1, 24),
		"192.168/16 should be le (192.168/16, 24)");
  ASSERT_RETURN(!ip_prefix_le_prefix(sPrefix5, sPrefix1, 24),
		"192.168/15 should not be le (192.168/16, 24)");
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
  ASSERT_RETURN((ip_string_to_prefix("1.2.3.4.5", &pcEndPtr, &sPrefix) < 0),
		"prefix \"1.2.3.4.5\" should be invalid");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_cmp ]--------------------------------
static int test_net_attr_prefix4_cmp()
{
  SPrefix sPrefix1= { .tNetwork= IPV4_TO_INT(1,2,3,4),
		      .uMaskLen= 24 };
  SPrefix sPrefix2= sPrefix1;
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == 0,
		"1.2.3.4/24 should be equal to 1.2.3.4.24");
  sPrefix2.tNetwork= IPV4_TO_INT(1,2,2,4);
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == 1,
		"1.2.3.4/24 should be > than 1.2.2.4/24");
  sPrefix2.tNetwork= sPrefix1.tNetwork;
  sPrefix2.uMaskLen= 25;
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == -1,
		"1.2.3.4/24 should be > than 1.2.3.4/25");
  sPrefix1.tNetwork= IPV4_TO_INT(1,2,3,1);
  sPrefix1.uMaskLen= 24;
  sPrefix2.tNetwork= IPV4_TO_INT(1,2,3,0);
  sPrefix2.uMaskLen= 24;
  ASSERT_RETURN(ip_prefix_cmp(&sPrefix1, &sPrefix2) == 0,
		"1.2.3.1/24 should be > than 1.2.3.0/24");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_dest ]---------------------------------------
static int test_net_attr_dest()
{
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_dest_str2 ]----------------------------------
static int test_net_attr_dest_str2()
{
  SNetDest sDest;
  ASSERT_RETURN(ip_string_to_dest("*", &sDest) == 0,
		"destination should be valid");
  ASSERT_RETURN(sDest.tType == NET_DEST_ANY,
		"conversion is invalid");
  ASSERT_RETURN(ip_string_to_dest("1.2.3.4", &sDest) == 0,
		"destination should be valid"); 
  ASSERT_RETURN((sDest.tType == NET_DEST_ADDRESS) &&
		(sDest.uDest.tAddr == IPV4_TO_INT(1,2,3,4)),
		"conversion is invalid");
  ASSERT_RETURN(ip_string_to_dest("1.2.3.4/25", &sDest) == 0,
		"destination should be valid"); 
  ASSERT_RETURN((sDest.tType == NET_DEST_PREFIX) &&
		(sDest.uDest.sPrefix.tNetwork == IPV4_TO_INT(1,2,3,4)) &&
		(sDest.uDest.sPrefix.uMaskLen == 25),
		"conversion is invalid");
  ASSERT_RETURN(ip_string_to_dest("1.2.3.4/33", &sDest) < 0,
		"destination should be invalid");
  ASSERT_RETURN(ip_string_to_dest("1.2.3.4.5", &sDest) < 0,
		"destination should be invalid");
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
// NET NODES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_node ]--------------------------------------------
static int test_net_node()
{
  SNetNode * pNode= node_create(IPV4_TO_INT(1,0,0,0));
  ASSERT_RETURN(pNode != NULL, "node creation should succeed");
  ASSERT_RETURN(pNode->tAddr == IPV4_TO_INT(1,0,0,0),
		"incorrect node address (ID)");
  ASSERT_RETURN(pNode->pLinks != NULL,
		"interfaces list should be created");
  ASSERT_RETURN(pNode->pProtocols != NULL,
		"protocols list should be created");
  ASSERT_RETURN(pNode->pRT != NULL,
		"forwarding table should be created");
  node_destroy(&pNode);
  ASSERT_RETURN(pNode == NULL, "destroyed node should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_node_name ]---------------------------------------
static int test_net_node_name()
{
  SNetNode * pNode= node_create(IPV4_TO_INT(1,0,0,0));
  ASSERT_RETURN(node_get_name(pNode) == NULL,
		"node should not have a default name");
  node_set_name(pNode, "NODE 1");
  ASSERT_RETURN(!strcmp(node_get_name(pNode), "NODE 1"),
		"node should have name \"NODE 1\"");
  node_set_name(pNode, NULL);
  ASSERT_RETURN(node_get_name(pNode) == NULL,
		"node should not have a name");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET SUBNETS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_subnet ]------------------------------------------
static int test_net_subnet()
{
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_STUB);
  ASSERT_RETURN(pSubnet != NULL, "subnet creation should succeed");
  subnet_destroy(&pSubnet);
  ASSERT_RETURN(pSubnet == NULL, "destroyed subnet should be NULL");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET INTERFACES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_iface_lo ]----------------------------------------
static int test_net_iface_lo()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  net_addr_t tAddr= IPV4_TO_INT(172,0,0,1);
  SNetIface * pIface= NULL;
  ASSERT_RETURN(net_iface_factory(pNode1, net_iface_id_addr(tAddr),
				  NET_IFACE_LOOPBACK, &pIface) == NET_SUCCESS,
		"should be able to create loopback interface");
  ASSERT_RETURN(pIface->tType == NET_IFACE_LOOPBACK,
		"interface type should be LOOPBACK");
  ASSERT_RETURN(pIface->tIfaceAddr == tAddr,
		"incorrect interface address");
  ASSERT_RETURN(pIface->tIfaceMask == 32,
		"incorrect interface mask");
  ASSERT_RETURN(!net_iface_is_connected(pIface),
		"interface should not be connected");
  net_iface_destroy(&pIface);
  ASSERT_RETURN(pIface == NULL,
		"destroyed interface should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_rtr ]---------------------------------------
static int test_net_iface_rtr()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  SNetNode * pNode2= node_create(IPV4_TO_INT(1,0,0,2));
  SNetIface * pIface= NULL;
  ASSERT_RETURN(net_iface_factory(pNode1, net_iface_id_rtr(pNode2),
				  NET_IFACE_RTR, &pIface) == NET_SUCCESS,
		"should be able to create rtr interface");
  ASSERT_RETURN(pIface != NULL,
		"should return non-null interface");
  ASSERT_RETURN(pIface->tType == NET_IFACE_RTR,
		"interface type should be RTR");
  ASSERT_RETURN(pIface->tIfaceAddr == pNode2->tAddr,
		"incorrect interface address");
  ASSERT_RETURN(pIface->tIfaceMask == 32,
		"incorrect interface mask");
  ASSERT_RETURN(!net_iface_is_connected(pIface),
		"interface should not be connected");
  net_iface_destroy(&pIface);
  ASSERT_RETURN(pIface == NULL,
		"destroyed interface should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_ptp ]---------------------------------------
static int test_net_iface_ptp()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  SNetIface * pIface= NULL;
  net_iface_id_t tIfaceID= net_iface_id_pfx(IPV4_TO_INT(192,168,0,1), 24);
  ASSERT_RETURN(net_iface_factory(pNode1, tIfaceID, NET_IFACE_PTP,
				  &pIface) == NET_SUCCESS,
		"should be able to create ptp interface");
  ASSERT_RETURN(pIface != NULL,
		"should return non-null interface");
  ASSERT_RETURN(pIface->tType == NET_IFACE_PTP,
		"interface type should be PTP");
  ASSERT_RETURN(pIface->tIfaceAddr == IPV4_TO_INT(192,168,0,1),
		"incorrect interface address");
  ASSERT_RETURN(pIface->tIfaceMask == 24,
		"incorrect interface mask");
  ASSERT_RETURN(!net_iface_is_connected(pIface),
		"interface should not be connected");
  net_iface_destroy(&pIface);
  ASSERT_RETURN(pIface == NULL,
		"destroyed interface should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_ptmp ]--------------------------------------
static int test_net_iface_ptmp()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  SNetIface * pIface= NULL;
  net_iface_id_t tIfaceID= net_iface_id_pfx(IPV4_TO_INT(192,168,0,1), 24);
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(192,168,0,0), .uMaskLen=24 };
  ASSERT_RETURN(net_iface_factory(pNode1, tIfaceID, NET_IFACE_PTMP,
				  &pIface) == NET_SUCCESS,
		"should be able to create ptmp interface");
  ASSERT_RETURN(pIface != NULL,
		"should return non-null interface");
  ASSERT_RETURN(pIface->tType == NET_IFACE_PTMP,
		"interface type should be PTMP");
  ASSERT_RETURN(pIface->tIfaceAddr == IPV4_TO_INT(192,168,0,1),
		"incorrect interface address");
  ASSERT_RETURN(pIface->tIfaceMask == 24,
		"incorrect interface mask");
  ASSERT_RETURN(!net_iface_is_connected(pIface),
		"interface should not be connected");
  ASSERT_RETURN(net_iface_has_prefix(pIface, sPrefix),
		"interface should have prefix");
  net_iface_destroy(&pIface);
  ASSERT_RETURN(pIface == NULL,
		"destroyed interface should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_virtual ]-----------------------------------
static int test_net_iface_virtual()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  SNetIface * pIface= NULL;
  net_addr_t tAddr= IPV4_TO_INT(172,0,0,1);
  net_iface_id_t tIfaceID= net_iface_id_addr(tAddr);
  ASSERT_RETURN(net_iface_factory(pNode1, tIfaceID,
				  NET_IFACE_VIRTUAL,
				  &pIface) == NET_SUCCESS,
		"should be able to create virtual interface");
  ASSERT_RETURN(pIface != NULL,
		"should return non-null interface");
  ASSERT_RETURN(pIface->tType == NET_IFACE_VIRTUAL,
		"interface type should be VIRTUAL");
  ASSERT_RETURN(pIface->tIfaceAddr == tAddr,
		"incorrect interface address");
  ASSERT_RETURN(pIface->tIfaceMask == 32,
		"incorrect interface mask");
  ASSERT_RETURN(!net_iface_is_connected(pIface),
		"interface should not be connected");
  net_iface_destroy(&pIface);
  ASSERT_RETURN(pIface == NULL,
		"destroyed interface should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_str2type ]----------------------------------
static int test_net_iface_str2type()
{
  ASSERT_RETURN(net_iface_str2type("loopback") == NET_IFACE_LOOPBACK,
		"bad conversion for loopback type");
  ASSERT_RETURN(net_iface_str2type("router-to-router") == NET_IFACE_RTR,
		"bad conversion for router-to-router type");
  ASSERT_RETURN(net_iface_str2type("point-to-point") == NET_IFACE_PTP,
		"bad conversion for point-to-point type");
  ASSERT_RETURN(net_iface_str2type("point-to-multipoint") == NET_IFACE_PTMP,
		"bad conversion for point-to-multipoint type");
  ASSERT_RETURN(net_iface_str2type("virtual") == NET_IFACE_VIRTUAL,
		"bad conversion for virtual type");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_list ]--------------------------------------
static int test_net_iface_list()
{
  SNetNode * pNode= node_create(IPV4_TO_INT(1,0,0,1));
  SNetLinks * pIfaces= net_links_create();
  SNetIface * pIface1= NULL;
  SNetIface * pIface2= NULL;
  net_addr_t tAddr1= IPV4_TO_INT(1,0,0,2);
  net_addr_t tAddr2= IPV4_TO_INT(1,0,0,3);
  ASSERT_RETURN(net_iface_factory(pNode, net_iface_id_addr(tAddr1),
				  NET_IFACE_RTR, &pIface1) == NET_SUCCESS,
		"should be able to create interface");
  ASSERT_RETURN(net_iface_factory(pNode, net_iface_id_addr(tAddr2),
				  NET_IFACE_RTR, &pIface2) == NET_SUCCESS,
		"should be able to create interface");
  ASSERT_RETURN(net_links_add(pIfaces, pIface1) >= 0,
		"should be able to add interface");
  ASSERT_RETURN(net_links_add(pIfaces, pIface2) >= 0,
		"should be able to add interface");
  net_links_destroy(&pIfaces);
  ASSERT_RETURN(pIfaces == NULL,
		"destroyed interface list should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_list_duplicate ]----------------------------
static int test_net_iface_list_duplicate()
{
  SNetNode * pNode= node_create(IPV4_TO_INT(1,0,0,1));
  SNetLinks * pIfaces= net_links_create();
  SNetIface * pIface1= NULL;
  SNetIface * pIface2= NULL;
  net_addr_t tAddr= IPV4_TO_INT(1,0,0,2);
  ASSERT_RETURN(net_iface_factory(pNode, net_iface_id_addr(tAddr),
				  NET_IFACE_RTR, &pIface1) == NET_SUCCESS,
		"should be able to create interface");
  ASSERT_RETURN(net_iface_factory(pNode, net_iface_id_addr(tAddr),
				  NET_IFACE_RTR, &pIface2) == NET_SUCCESS,
		"should be able to create interface");
  ASSERT_RETURN(net_links_add(pIfaces, pIface1) >= 0,
		"should be able to add interface");
  ASSERT_RETURN(net_links_add(pIfaces, pIface2) < 0,
		"should not be able to add interface with same ID");
  net_links_destroy(&pIfaces);
  ASSERT_RETURN(pIfaces == NULL,
		"destroyed interface list should be NULL");
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
  net_iface_id_t tIfaceID;
  ASSERT_RETURN((net_link_create_rtr(pNode1, pNode2, BIDIR, &pLink)
		 == NET_SUCCESS) &&
		(pLink != NULL),
		"link creation should succeed");
  tIfaceID= net_iface_id(pLink);
  ASSERT_RETURN((tIfaceID.tNetwork == pLink->tIfaceAddr) &&
		(tIfaceID.uMaskLen == pLink->tIfaceMask),
		"link identifier is incorrect");
  ASSERT_RETURN(pLink->tType == NET_IFACE_RTR,
		"link type is not correct");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN((pLink->tDest.pIface != NULL) &&
		(pLink->tDest.pIface->pSrcNode == pNode2),
		"destination node is not correct");
  ASSERT_RETURN(net_iface_is_enabled(pLink) != 0,
		"link should be up");
  ASSERT_RETURN((net_iface_get_delay(pLink) == 0) &&
		(net_iface_get_capacity(pLink) == 0) &&
		(net_iface_get_load(pLink) == 0),
		"link attributes are not correct");
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_forward ]------------------------------------
static int test_net_link_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetMessage * pMsg= (SNetMessage *) 12345;
  SNetLink * pLink= NULL;
  SNetIface * pDstIface;
  ASSERT_RETURN(net_link_create_rtr(pNode1, pNode2, BIDIR, &pLink)
		== NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(pLink->fSend(0, pLink, &pDstIface, &pMsg) == NET_SUCCESS,
		"link forward should succeed");
  ASSERT_RETURN((pDstIface != NULL) && (pDstIface->pSrcNode == pNode2),
		"link target is incorrect");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_forward_down ]-------------------------------
static int test_net_link_forward_down()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetIface * pDstIface= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  SNetLink * pLink= NULL;
  ASSERT_RETURN(net_link_create_rtr(pNode1, pNode2, BIDIR, &pLink)
		== NET_SUCCESS,
		"link creation should succeed");
  net_iface_set_enabled(pLink, 0);
  ASSERT_RETURN(pLink->fSend(0, pLink, &pDstIface, &pMsg)
		== NET_ERROR_LINK_DOWN,
		"link forward should fail");
  ASSERT_RETURN(pDstIface == NULL,
		"link target should be unchanged (NULL)");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptp ]----------------------------------------
static int test_net_link_ptp()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  net_addr_t tAddr1= IPV4_TO_INT(192,168,0,1);
  net_addr_t tAddr2= IPV4_TO_INT(192,168,0,2);
  SNetLink * pLink= NULL;
  ASSERT_RETURN(net_link_create_ptp(pNode1, net_iface_id_pfx(tAddr1, 30),
				    pNode2, net_iface_id_pfx(tAddr2, 30),
				    BIDIR, &pLink)
		== NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(pLink != NULL,
		"return link should not be NULL");
  ASSERT_RETURN(pLink->tType == NET_IFACE_PTP,
		"interface type is incorrect");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pLink->tIfaceAddr == tAddr1,
		"interface address is incorrect");
  ASSERT_RETURN(pLink->tIfaceMask == 30,
		"interface mask is incorrect");
  ASSERT_RETURN((pLink->tDest.pIface != NULL) &&
		(pLink->tDest.pIface->pSrcNode == pNode2),
		"destination node is incorrect");
  ASSERT_RETURN((net_iface_get_delay(pLink) == 0) &&
		(net_iface_get_capacity(pLink) == 0) &&
		(net_iface_get_load(pLink) == 0),
		"link attributes are not correct");
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptp_forward ]--------------------------------
static int test_net_link_ptp_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  net_addr_t tAddr1= IPV4_TO_INT(192,168,0,1);
  net_addr_t tAddr2= IPV4_TO_INT(192,168,0,2);
  SNetLink * pLink= NULL;
  SNetIface * pDstIface;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  ASSERT_RETURN(net_link_create_ptp(pNode1, net_iface_id_pfx(tAddr1, 30),
				    pNode2, net_iface_id_pfx(tAddr2, 30),
				    BIDIR, &pLink)
		== NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(pLink != NULL,
		"return link should not be NULL");
  ASSERT_RETURN(pLink->tType == NET_IFACE_PTP,
		"interface type is incorrect");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pLink->tIfaceAddr == tAddr1,
		"interface address is incorrect");
  ASSERT_RETURN(pLink->tIfaceMask == 30,
		"interface mask is incorrect");
  ASSERT_RETURN((pLink->tDest.pIface != NULL) &&
		(pLink->tDest.pIface->pSrcNode == pNode2),
		"destination node is incorrect");
  ASSERT_RETURN(pLink->fSend(0, pLink, &pDstIface, &pMsg)
		== NET_SUCCESS,
		"link forward should succeed");
  ASSERT_RETURN((pDstIface != NULL) && (pDstIface->pSrcNode == pNode2),
		"link target is incorrect");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  net_link_destroy(&pLink);
  ASSERT_RETURN(pLink == NULL,
		"destroyed link should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptp_forward_down ]---------------------------
static int test_net_link_ptp_forward_down()
{
  return UTEST_SKIPPED;
}

// -----[ test_net_link_ptmp ]---------------------------------------
static int test_net_link_ptmp()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_TRANSIT);
  SNetLink * pLink= NULL;
  SPrefix sIfaceID;
  ASSERT_RETURN((net_link_create_ptmp(pNode1, pSubnet,
				      IPV4_TO_INT(192,168,0,1),
				      &pLink) == NET_SUCCESS) &&
		(pLink != NULL),
		"link creation should succeed");
  sIfaceID= net_iface_id(pLink);
  ASSERT_RETURN((sIfaceID.tNetwork == pLink->tIfaceAddr) &&
		(sIfaceID.uMaskLen == pLink->tIfaceMask),
		"link identifier is incorrect");
  ASSERT_RETURN(pLink->tType == NET_IFACE_PTMP,
		"link type is not correct");
  ASSERT_RETURN(pLink->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pLink->tDest.pSubnet == pSubnet,
		"destination subnet is not correct");
  ASSERT_RETURN(pLink->tIfaceAddr == IPV4_TO_INT(192,168,0,1),
		"interface address is not correct");
  ASSERT_RETURN(net_iface_is_enabled(pLink) != 0,
		"link should be up");
  ASSERT_RETURN((net_iface_get_delay(pLink) == 0) &&
		(net_iface_get_capacity(pLink) == 0) &&
		(net_iface_get_load(pLink) == 0),
		"link attributes are not correct");
  subnet_destroy(&pSubnet);
  ASSERT_RETURN(pSubnet == NULL,
		"destroyed subnet should be NULL");
  node_destroy(&pNode1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptmp_forward ]-------------------------------
static int test_net_link_ptmp_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_TRANSIT);
  SNetLink * pLink1= NULL;
  SNetLink * pLink2= NULL;
  SNetIface * pDstIface;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  ASSERT_RETURN(net_link_create_ptmp(pNode1, pSubnet,
				     IPV4_TO_INT(192,168,0,1),
				     &pLink1)
		== NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(net_link_create_ptmp(pNode2, pSubnet,
				     IPV4_TO_INT(192,168,0,2),
				     &pLink2) == NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(pLink1->fSend(IPV4_TO_INT(192,168,0,2), pLink1,
			      &pDstIface, &pMsg)
		== NET_SUCCESS,
		"link forward should succeed");
  ASSERT_RETURN((pDstIface != NULL) && (pDstIface->pSrcNode == pNode2),
		"link target is incorrect");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");
  subnet_destroy(&pSubnet);
  node_destroy(&pNode1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptmp_forward_unreach ]-----------------------
static int test_net_link_ptmp_forward_unreach()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_TRANSIT);
  SNetLink * pLink1= NULL;
  SNetNode * pNextHop= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  ASSERT_RETURN(net_link_create_ptmp(pNode1, pSubnet,
				     IPV4_TO_INT(192,168,0,1),
				     &pLink1) == NET_SUCCESS,
		"link creation should succeed");
  ASSERT_RETURN(pNextHop == NULL,
		"link target should be unchanged (NULL)");
  ASSERT_RETURN(pMsg == (SNetMessage *) 12345,
		"message content should not have changed");  
  subnet_destroy(&pSubnet);
  node_destroy(&pNode1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_tunnel ]-------------------------------------
static int test_net_link_tunnel()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetIface * pIface= NULL;
  SNetLink * pTunnel= NULL;
  ASSERT_RETURN(net_link_create_rtr(pNode1, pNode2, BIDIR, &pIface)
		== NET_SUCCESS,
		"node_add_link_rtr() should succeed");
  ASSERT_RETURN(pIface != NULL,
		"node_find_iface() should not return NULL");
  ASSERT_RETURN((ipip_link_create(pNode1, IPV4_TO_INT(2,0,0,0),
				  IPV4_TO_INT(127,0,0,1),
				  pIface,
				  NET_ADDR_ANY,
				  &pTunnel) == NET_SUCCESS) &&
		(pTunnel != NULL),
		"ipip_link_create() should succeed");
  ASSERT_RETURN(pTunnel->tType == NET_IFACE_VIRTUAL,
		"link type is incorrect");
  ASSERT_RETURN(pIface->pSrcNode == pNode1,
		"source node is not correct");
  ASSERT_RETURN(pTunnel->tDest.tEndPoint == pNode2->tAddr,
		"tunnel endpoint is incorrect");
  ASSERT_RETURN(pTunnel->tIfaceAddr == IPV4_TO_INT(127,0,0,1),
		"interface address is incorrect");
  ASSERT_RETURN(net_iface_is_enabled(pTunnel) != 0,
		"link should be up");
  net_link_destroy(&pTunnel);
  ASSERT_RETURN(pTunnel == NULL,
		"destroyed link should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_link_tunnel_forward ]-----------------------------
static int test_net_link_tunnel_forward()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(2,0,0,0));
  SNetIface * pIface= NULL;
  SNetLink * pTunnel= NULL;
  SNetMessage * pMsg= (SNetMessage *) 12345;
  SNetIface * pDstIface;
  ASSERT_RETURN(net_link_create_rtr(pNode1, pNode2, BIDIR, &pIface)
		== NET_SUCCESS,
		"node_add_link_rtr() should succeed");
  ASSERT_RETURN(pIface != NULL,
		"node_find_iface() should not return NULL");
  ASSERT_RETURN((ipip_link_create(pNode1, IPV4_TO_INT(2,0,0,0),
				 IPV4_TO_INT(127,0,0,1),
				 pIface,
				 NET_ADDR_ANY,
				  &pTunnel) == NET_SUCCESS) &&
		(pTunnel != NULL),
		"ipip_link_create() should succeed");
  ASSERT_RETURN(pTunnel != NULL,
		"");
  ASSERT_RETURN(pTunnel->fSend(NET_ADDR_ANY, pTunnel->pContext,
			       &pDstIface, &pMsg) == NET_SUCCESS,
		"tunnel forward should succeed");
  ASSERT_RETURN((pDstIface != NULL) && (pDstIface->pSrcNode == pNode2),
		"link target is incorrect");
  ASSERT_RETURN((pMsg != (SNetMessage *) 12345) &&
		(pMsg->pPayLoad == (void *) 12345),
		"initial message should be encapsulated");
  net_link_destroy(&pTunnel);
  ASSERT_RETURN(pTunnel == NULL,
		"destroyed link should be NULL");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET NETWORK
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_network ]-----------------------------------------
static int test_net_network()
{
  SNetwork * pNetwork= network_create();
  ASSERT_RETURN(pNetwork != NULL, "network creation should succeed");
  network_destroy(&pNetwork);
  ASSERT_RETURN(pNetwork == NULL, "destroyed network should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_node ]--------------------------------
static int test_net_network_add_node()
{
  SNetwork * pNetwork= network_create();
  SNetNode * pNode= node_create(IPV4_TO_INT(1,0,0,0));
  ASSERT_RETURN(network_add_node(pNode) == NET_SUCCESS,
		"node addition should succeed");
  network_destroy(&pNetwork);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_node_dup ]----------------------------
static int test_net_network_add_node_dup()
{
  SNetwork * pNetwork= network_create();
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pNode2= node_create(IPV4_TO_INT(1,0,0,0));
  ASSERT_RETURN(network_add_node(pNode1) == NET_SUCCESS,
		"node addition should succeed");
  ASSERT_RETURN(network_add_node(pNode2)
		== NET_ERROR_MGMT_NODE_ALREADY_EXISTS,
		"duplicate node addition should fail");
  network_destroy(&pNetwork);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_subnet ]------------------------------
static int test_net_network_add_subnet()
{
  SNetwork * pNetwork= network_create();
  SNetSubnet * pSubnet= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				      NET_SUBNET_TYPE_STUB);
  ASSERT_RETURN(network_add_subnet(pSubnet) == NET_SUCCESS,
		"subnet addition should succeed");
  network_destroy(&pNetwork);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_subnet_dup ]--------------------------
static int test_net_network_add_subnet_dup()
{
  SNetwork * pNetwork= network_create();
  SNetSubnet * pSubnet1= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				       NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnet2= subnet_create(IPV4_TO_INT(192,168,0,0), 16,
				       NET_SUBNET_TYPE_STUB);
  ASSERT_RETURN(network_add_subnet(pSubnet1) == NET_SUCCESS,
		"subnet addition should succeed");
  ASSERT_RETURN(network_add_subnet(pSubnet2)
		== NET_ERROR_MGMT_SUBNET_ALREADY_EXISTS,
		"duplicate subnet addision should fail");
  network_destroy(&pNetwork);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET IGP
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_igp_domain ]--------------------------------------
static int test_net_igp_domain()
{
  SIGPDomain * pDomain= igp_domain_create(11537, DOMAIN_IGP);
  ASSERT_RETURN(pDomain != NULL,
		"igp_domain_create() should succeed");
  igp_domain_destroy(&pDomain);
  ASSERT_RETURN(pDomain == NULL,
		"destroyed domain should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute ]-------------------------------------
static int test_net_igp_compute()
{
  SNetwork * pNetwork= network_create();
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  SNetNode * pNode2= node_create(IPV4_TO_INT(1,0,0,2));
  SNetNode * pNode3= node_create(IPV4_TO_INT(1,0,0,3));
  net_addr_t tIfaceAddr12= IPV4_TO_INT(192,168,0,1);
  net_addr_t tIfaceAddr21= IPV4_TO_INT(192,168,0,2);
  net_addr_t tIfaceAddr23= IPV4_TO_INT(192,168,0,5);
  net_addr_t tIfaceAddr32= IPV4_TO_INT(192,168,0,6);
  SIGPDomain * pDomain= igp_domain_create(1, DOMAIN_IGP);
  SNetLink * pLink;
  
  network_add_node(pNode1);
  network_add_node(pNode2);
  network_add_node(pNode3);
  ASSERT_RETURN(net_link_create_ptp(pNode1,
				    net_iface_id_pfx(tIfaceAddr12, 30),
				    pNode2,
				    net_iface_id_pfx(tIfaceAddr21, 30),
				    BIDIR, &pLink)
		== NET_SUCCESS,
		"ptp-link creation should succeed");
  ASSERT_RETURN(net_iface_set_metric(pLink, 0, 1, 1) == NET_SUCCESS,
		"should be able to set interface metric");
  ASSERT_RETURN(net_link_create_ptp(pNode2,
				    net_iface_id_pfx(tIfaceAddr23, 30),
				    pNode3,
				    net_iface_id_pfx(tIfaceAddr32, 30),
				    BIDIR, &pLink)
		== NET_SUCCESS,
		"ptp-link creation should succeed");
  ASSERT_RETURN(net_iface_set_metric(pLink, 0, 10, 1) == NET_SUCCESS,
		"should be able to set interface metric");
  igp_domain_add_router(pDomain, pNode1);
  igp_domain_add_router(pDomain, pNode2);
  igp_domain_add_router(pDomain, pNode3);
  igp_domain_compute(pDomain);
  network_destroy(&pNetwork);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET TRACES
//
/////////////////////////////////////////////////////////////////////

static inline int __test_net_traces_before()
{
  SNetNode * pNode1= node_create(IPV4_TO_INT(1,0,0,1));
  SNetNode * pNode2= node_create(IPV4_TO_INT(1,0,0,2));
  SNetNode * pNode3= node_create(IPV4_TO_INT(1,0,0,3));
  SIGPDomain * pDomain= igp_domain_create(1, DOMAIN_IGP);
  SNetLink * pLink;
  network_add_node(pNode1);
  network_add_node(pNode2);
  network_add_node(pNode3);
  ASSERT_RETURN(net_link_create_rtr(pNode1, pNode2, BIDIR, &pLink)
		== NET_SUCCESS,
		"rtr-link creation should succeed");
  net_iface_set_metric(pLink, 0, 10, 1);
  ASSERT_RETURN(net_link_create_rtr(pNode2, pNode3, BIDIR, &pLink)
		== NET_SUCCESS,
		"rtr-link creation should succeed");
  net_iface_set_metric(pLink, 0, 5, 1);
  igp_domain_add_router(pDomain, pNode1);
  igp_domain_add_router(pDomain, pNode2);
  igp_domain_add_router(pDomain, pNode3);
  igp_domain_compute(pDomain);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_ping ]-------------------------------------
static int test_net_traces_ping()
{
  SNetNode * pNode;
  __test_net_traces_before();
  pNode= network_find_node(IPV4_TO_INT(1,0,0,1));
  ASSERT_RETURN(icmp_ping_send_recv(pNode, NET_ADDR_ANY,
				    IPV4_TO_INT(1,0,0,2), 255) == NET_SUCCESS,
		"ping from 1.0.0.1 to 1.0.0.2 should succeed");
  ASSERT_RETURN(icmp_ping_send_recv(pNode, NET_ADDR_ANY,
				    IPV4_TO_INT(1,0,0,3), 255) == NET_SUCCESS,
		"ping from 1.0.0.1 to 1.0.0.3 should succeed");
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_traceroute ]-------------------------------
static int test_net_traces_traceroute()
{
  SNetNode * pNode;
  __test_net_traces_before();
  pNode= network_find_node(IPV4_TO_INT(1,0,0,1));
  return UTEST_SKIPPED;
}

// -----[ test_net_traces_recordroute ]------------------------------
static int test_net_traces_recordroute()
{
  SNetNode * pNode;
  SNetRecordRouteInfo * pInfo;
  SNetDest sDest= { .tType= NET_DEST_ADDRESS,
		    .uDest.tAddr= IPV4_TO_INT(1,0,0,3) };
  __test_net_traces_before();
  pNode= network_find_node(IPV4_TO_INT(1,0,0,1));
  pInfo= node_record_route(pNode, sDest, 0, 0, 0);
  ASSERT_RETURN(pInfo != NULL,
		"record-route info should not be NULL");
  ASSERT_RETURN(pInfo->iResult == NET_SUCCESS,
		"record-route should succeed");
  ASSERT_RETURN(pInfo->pTrace != NULL,
		"record-route path should not be NULL");
  ASSERT_RETURN(ip_trace_length(pInfo->pTrace) == 3,
		"record-route path's length should be 3");
  ASSERT_RETURN((ip_trace_item_at(pInfo->pTrace, 0)->uItem.pNode == pNode) &&
		(ip_trace_item_at(pInfo->pTrace, 1)->uItem.pNode->tAddr ==
		 IPV4_TO_INT(1,0,0,2)) &&
		(ip_trace_item_at(pInfo->pTrace, 2)->uItem.pNode->tAddr ==
		 IPV4_TO_INT(1,0,0,3)),
		"record-route path content is not correct");
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
  path_destroy(&pPath);
  pPath= path_from_string("12 34 56");
  ASSERT_RETURN(pPath != NULL,
		"\"12 34 56\" is a valid AS-Path");
  path_destroy(&pPath);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_set_str2 ]----------------------------
static int test_bgp_attr_aspath_set_str2()
{
  SBGPPath * pPath;
  SPathSegment * pSegment;
  pPath= path_from_string("{1 2 3}");
  ASSERT_RETURN(pPath != NULL, "\"{1 2 3}\" is a valid AS-Path");
  ASSERT_RETURN(path_num_segments(pPath) == 1,
		"Path should have a single segment");
  pSegment= (SPathSegment *) pPath->data[0];
  ASSERT_RETURN(pSegment->uType == AS_PATH_SEGMENT_SET,
		"Segment type should be AS_SET");
  ASSERT_RETURN(pSegment->uLength == 3,
		"Segment size should be 3");
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

// -----[ test_bgp_attr_aspath_rem_private ]-------------------------
static int test_bgp_attr_aspath_rem_private()
{
  SBGPPath * pPath= NULL;
  pPath= path_from_string("64511 64512 12 34 65535");
  ASSERT_RETURN(pPath != NULL, "64511 64512 12 34 65535 is a valid AS-Path");
  ASSERT_RETURN(path_length(pPath) == 5, "path length should be 5");
  ASSERT_RETURN(path_contains(pPath, 64511), "path should contain 64511");
  ASSERT_RETURN(path_contains(pPath, 64512), "path should contain 64512");
  ASSERT_RETURN(path_contains(pPath, 12), "path should contain 12");
  ASSERT_RETURN(path_contains(pPath, 34), "path should contain 34");
  ASSERT_RETURN(path_contains(pPath, 65535), "path should contain 65535");
  path_remove_private(pPath);
  ASSERT_RETURN(path_length(pPath) == 3, "path length should be 3");
  ASSERT_RETURN(path_contains(pPath, 64511), "path should contain 64511");
  ASSERT_RETURN(path_contains(pPath, 64512) == 0,
		"path should not contain 64512");
  ASSERT_RETURN(path_contains(pPath, 12), "path should contain 12");
  ASSERT_RETURN(path_contains(pPath, 34), "path should contain 34");
  ASSERT_RETURN(path_contains(pPath, 65535) == 0,
		"path should not contain 65535");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_match ]-------------------------------
static int test_bgp_attr_aspath_match()
{
  SRegEx * pRegEx= regex_init("^2611$", 0);
  SBGPPath * pPath;
  ASSERT_RETURN(pRegEx != NULL, "reg-ex should not be NULL");
  pPath= path_from_string("2611");
  ASSERT_RETURN(path_match(pPath, pRegEx) != 0, "path should match");
  path_destroy(&pPath);
  regex_reinit(pRegEx);
  pPath= path_from_string("5511 2611");
  ASSERT_RETURN(path_match(pPath, pRegEx) == 0, "path should not match");
  path_destroy(&pPath);
  regex_finalize(&pRegEx);
  ASSERT_RETURN(pRegEx == NULL, "destroyed reg-ex should be NULL");
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
// BGP FILTER ACTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_filter_action_comm_add ]--------------------------
int test_bgp_filter_action_comm_add()
{
  SFilterAction * pAction= filter_action_comm_append(1234);
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_COMM_APPEND,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_remove ]-----------------------
int test_bgp_filter_action_comm_remove()
{
  SFilterAction * pAction= filter_action_comm_remove(1234);
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_COMM_REMOVE,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_strip ]------------------------
int test_bgp_filter_action_comm_strip()
{
  SFilterAction * pAction= filter_action_comm_strip();
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_COMM_STRIP,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_local_pref ]------------------------
int test_bgp_filter_action_local_pref()
{
  SFilterAction * pAction= filter_action_pref_set(1234);
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_PREF_SET,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric ]----------------------------
int test_bgp_filter_action_metric()
{
  SFilterAction * pAction= filter_action_metric_set(1234);
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_METRIC_SET,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric_internal ]-------------------
int test_bgp_filter_action_metric_internal()
{
  SFilterAction * pAction= filter_action_metric_internal();
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_METRIC_INTERNAL,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_path_prepend ]----------------------
int test_bgp_filter_action_path_prepend()
{
  SFilterAction * pAction= filter_action_path_prepend(5);
  ASSERT_RETURN(pAction != NULL, "New action should be NULL");
  ASSERT_RETURN(pAction->uCode == FT_ACTION_PATH_PREPEND,
		"Incorrect action code");
  filter_action_destroy(&pAction);
  ASSERT_RETURN(pAction == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_add_str2 ]---------------------
int test_bgp_filter_action_comm_add_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("community add 1234", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_remove_str2 ]------------------
int test_bgp_filter_action_comm_remove_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("community remove 1234", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_strip_str2 ]-------------------
int test_bgp_filter_action_comm_strip_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("community strip", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_local_pref_str2 ]-------------------
int test_bgp_filter_action_local_pref_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("local-pref 1234", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric_str2 ]-----------------------
int test_bgp_filter_action_metric_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("metric 1234", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric_internal_str2 ]--------------
int test_bgp_filter_action_metric_internal_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("metric internal", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_path_prepend_str2 ]-----------------
int test_bgp_filter_action_path_prepend_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("as-path prepend 5", &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_expression_str2 ]-------------------
int test_bgp_filter_action_expression_str2()
{
  SFilterAction * pAction;
  ASSERT_RETURN(filter_parser_action("as-path prepend 2, local-pref 253",
				     &pAction) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&pAction);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP FILTER PREDICATES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_filter_predicate_comm_contains ]------------------
int test_bgp_filter_predicate_comm_contains()
{
  SFilterMatcher * pPredicate= filter_match_comm_contains(1234);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_COMM_CONTAINS,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_is ]---------------------
int test_bgp_filter_predicate_nexthop_is()
{
  SFilterMatcher * pPredicate=
    filter_match_nexthop_equals(IPV4_TO_INT(10,0,0,0));
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_NEXTHOP_IS,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_in ]---------------------
int test_bgp_filter_predicate_nexthop_in()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(10,0,0,0),
		     .uMaskLen= 16 };
  SFilterMatcher * pPredicate=
    filter_match_nexthop_in(sPrefix);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_NEXTHOP_IN,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_is ]----------------------
int test_bgp_filter_predicate_prefix_is()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(10,0,0,0),
		     .uMaskLen= 16 };
  SFilterMatcher * pPredicate=
    filter_match_prefix_equals(sPrefix);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_PREFIX_IS,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_in ]----------------------
int test_bgp_filter_predicate_prefix_in()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(10,0,0,0),
		     .uMaskLen= 16 };
  SFilterMatcher * pPredicate=
    filter_match_prefix_in(sPrefix);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_PREFIX_IN,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_ge ]----------------------
static int test_bgp_filter_predicate_prefix_ge()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(10,0,0,0),
		     .uMaskLen= 16 };
  SFilterMatcher * pPredicate=
    filter_match_prefix_ge(sPrefix, 24);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_PREFIX_GE,
		"Incorrect predicate code");
  ASSERT_RETURN(*((uint8_t *) (pPredicate->auParams+sizeof(SPrefix))) == 24,
		"Incorrect prefix length");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_le ]----------------------
static int test_bgp_filter_predicate_prefix_le()
{
  SPrefix sPrefix= { .tNetwork= IPV4_TO_INT(10,0,0,0),
		     .uMaskLen= 16 };
  SFilterMatcher * pPredicate=
    filter_match_prefix_le(sPrefix, 24);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_PREFIX_LE,
		"Incorrect predicate code");
  ASSERT_RETURN(*((uint8_t *) (pPredicate->auParams+sizeof(SPrefix))) == 24,
		"Incorrect prefix length");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_path_regexp ]--------------------
int test_bgp_filter_predicate_path_regexp()
{
  int iRegExIndex= 12345;
  SFilterMatcher * pPredicate=
    filter_match_path(iRegExIndex);
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_PATH_MATCHES,
		"Incorrect predicate code");
  ASSERT_RETURN(memcmp(pPredicate->auParams, &iRegExIndex,
		       sizeof(iRegExIndex)) == 0,
		"Incorrect regexp index");
  filter_matcher_destroy(&pPredicate);
  ASSERT_RETURN(pPredicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_and ]----------------------------
int test_bgp_filter_predicate_and()
{
  SFilterMatcher * pPredicate=
    filter_match_and(filter_match_comm_contains(1),
		     filter_match_comm_contains(2));
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_OP_AND,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_and_any ]------------------------
int test_bgp_filter_predicate_and_any()
{
  SFilterMatcher * pPredicate=
    filter_match_and(NULL,
		     filter_match_comm_contains(2));
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_COMM_CONTAINS,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_not ]----------------------------
int test_bgp_filter_predicate_not()
{
  SFilterMatcher * pPredicate=
    filter_match_not(filter_match_comm_contains(1));
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_OP_NOT,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_not_not ]------------------------
int test_bgp_filter_predicate_not_not()
{
  SFilterMatcher * pPredicate=
    filter_match_not(filter_match_not(filter_match_comm_contains(1)));
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_COMM_CONTAINS,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_or ]----------------------------
int test_bgp_filter_predicate_or()
{
  SFilterMatcher * pPredicate=
    filter_match_or(filter_match_comm_contains(1),
		    filter_match_comm_contains(2));
  ASSERT_RETURN(pPredicate != NULL,
		"New predicate should not be NULL");
  ASSERT_RETURN(pPredicate->uCode == FT_MATCH_OP_OR,
		"Incorrect predicate code");
  filter_matcher_destroy(&pPredicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_or_any ]------------------------
int test_bgp_filter_predicate_or_any()
{
  SFilterMatcher * pPredicate=
    filter_match_or(NULL,
		    filter_match_comm_contains(2));
  ASSERT_RETURN(pPredicate == NULL,
		"New predicate should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_comm_contains_str2 ]-------------
int test_bgp_filter_predicate_comm_contains_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("community is 1", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_is_str2 ]----------------
int test_bgp_filter_predicate_nexthop_is_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("next-hop is 1.0.0.0", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_in_str2 ]----------------
int test_bgp_filter_predicate_nexthop_in_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("next-hop in 10/8", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_is_str2 ]-----------------
int test_bgp_filter_predicate_prefix_is_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("prefix is 10/8", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_in_str2 ]-----------------
int test_bgp_filter_predicate_prefix_in_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("prefix in 10/8", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_ge_str2 ]-----------------
static int test_bgp_filter_predicate_prefix_ge_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("prefix ge 10/8 16", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_le_str2 ]-----------------
static int test_bgp_filter_predicate_prefix_le_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("prefix le 10/8 16", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_path_regexp_str2 ]---------------
int test_bgp_filter_predicate_path_regexp_str2()
{
  SFilterMatcher * pMatcher;
  ASSERT_RETURN(predicate_parser("path \"^(1_)\"", &pMatcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&pMatcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_expr_str2 ]----------------------
int test_bgp_filter_predicate_expr_str2()
{
  char * pcPredicates[]= {
    "(community is 1)",
    "(community is 1) | (community is 2)",
    "(path \"^(1_)\") & (community is 1) | (community is 2)",
    "(path \"^(1_)\") & ((community is 1) | (community is 2))",
    "!community is 1",
    "!(community is 1)",
    "(community is 1 & community is 2)",
    "!(!community is 1)",
    "((community is 1))",
  };
  SFilterMatcher * pMatcher;
  unsigned int uIndex;

  for (uIndex= 0; uIndex < sizeof(pcPredicates)/sizeof(pcPredicates[0]);
       uIndex++) {
    ASSERT_RETURN(predicate_parser(pcPredicates[uIndex], &pMatcher) >= 0,
		  "predicate_parser() should return >= 0");
    ASSERT_RETURN(pMatcher != NULL,
		  "New predicate should not be NULL");
    filter_matcher_destroy(&pMatcher);
    ASSERT_RETURN(pMatcher == NULL, "Predicate should be NULL when destroyed");
  }

  ASSERT_RETURN(predicate_parser("path \"^(1_)", &pMatcher) ==
		PREDICATE_PARSER_ERROR_UNFINISHED_STRING,
		"Should return \"unfinished-string\" error");
  ASSERT_RETURN(predicate_parser("plop", &pMatcher) ==
		PREDICATE_PARSER_ERROR_ATOM,
		"Should return \"atom\" error");
  ASSERT_RETURN(predicate_parser("(community is 1", &pMatcher) ==
		PREDICATE_PARSER_ERROR_PAR_MISMATCH,
		"Should return \"parenthesis-mismatch\" error");
  ASSERT_RETURN(predicate_parser("community is 1)", &pMatcher) ==
		PREDICATE_PARSER_ERROR_PAR_MISMATCH,
		"Should return \"parenthesis-mismatch\" error");
  ASSERT_RETURN(predicate_parser("& community is 1", &pMatcher) ==
		PREDICATE_PARSER_ERROR_LEFT_EXPR,
		"Should return \"left-expression\" error");
  ASSERT_RETURN(predicate_parser("community is 1 &", &pMatcher) ==
		PREDICATE_PARSER_ERROR_UNEXPECTED_EOF,
		"Should return \"unexpected-eof\" error");
  ASSERT_RETURN(predicate_parser("!!community is 1", &pMatcher) ==
		PREDICATE_PARSER_ERROR_UNARY_OP,
		"Should return \"unary-op-not-allowed\" error");

  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP PEER
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_peer ]--------------------------------------------
static int test_bgp_peer()
{
  net_addr_t tPeerAddr= IPV4_TO_INT(1,2,3,4);
  SBGPPeer * pPeer= bgp_peer_create(1234, tPeerAddr, NULL);
  ASSERT_RETURN(pPeer != NULL,
		"bgp peer creation should succeed");
  ASSERT_RETURN(pPeer->tAddr == tPeerAddr,
		"peer address is incorrect");
  ASSERT_RETURN(pPeer->tSrcAddr == NET_ADDR_ANY,
		"default source address should be 0.0.0.0"); 
  ASSERT_RETURN(pPeer->tNextHop == NET_ADDR_ANY,
		"default next-hop address should be 0.0.0.0");
  ASSERT_RETURN(pPeer->uSessionState == SESSION_STATE_IDLE,
		"session state should be IDLE");
  ASSERT_RETURN((pPeer->pInFilter == NULL) &&
		(pPeer->pOutFilter == NULL),
		"default in/out-filters should be NULL (accept any)");
  ASSERT_RETURN((pPeer->pAdjRIBIn != NULL) &&
		(pPeer->pAdjRIBOut != NULL),
		"Adj-RIB-in/out is not initialized");
  bgp_peer_destroy(&pPeer);
  ASSERT_RETURN(pPeer == NULL,
		"destroyed peer should be NULL");
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
  {test_net_attr_prefix4_ge, "IPv4 prefix ge prefix"},
  {test_net_attr_prefix4_le, "IPv4 prefix le prefix"},
  {test_net_attr_prefix4_match, "IPv4 prefix match"},
  {test_net_attr_prefix4_mask, "IPv4 prefix mask"},
  {test_net_attr_prefix4_2str, "IPv4 prefix (-> string)"},
  {test_net_attr_prefix4_str2, "IPv4 prefix (<- string)"},
  {test_net_attr_prefix4_cmp, "IPv4 prefix (compare)"},
  {test_net_attr_dest, "IPv4 destination"},
  {test_net_attr_dest_str2, "IPv4 destination (<- string)"},
  {test_net_attr_igpweight, "IGP weight"},
  {test_net_attr_igpweight_add, "IGP weight add"},
};
#define TEST_NET_ATTR_SIZE ARRAY_SIZE(TEST_NET_ATTR)

SUnitTest TEST_NET_NODE[]= {
  {test_net_node, "node"},
  {test_net_node_name, "node name"},
};
#define TEST_NET_NODE_SIZE ARRAY_SIZE(TEST_NET_NODE)

SUnitTest TEST_NET_SUBNET[]= {
  {test_net_subnet, "subnet"},
};
#define TEST_NET_SUBNET_SIZE ARRAY_SIZE(TEST_NET_SUBNET)

SUnitTest TEST_NET_IFACE[]= {
  {test_net_iface_lo, "interface lo"},
  {test_net_iface_rtr, "interface rtr"},
  {test_net_iface_ptp, "interface ptp"},
  {test_net_iface_ptmp, "interface ptmp"},
  {test_net_iface_virtual, "interface virtual"},
  {test_net_iface_str2type, "interface (string -> type)"},
  {test_net_iface_list, "interface-list"},
  {test_net_iface_list_duplicate, "interface-list (duplicate)"},
};
#define TEST_NET_IFACE_SIZE ARRAY_SIZE(TEST_NET_IFACE)

SUnitTest TEST_NET_LINK[]= {
  {test_net_link, "link"},
  {test_net_link_forward, "link forward"},
  {test_net_link_forward_down, "link forward (down)"},
  {test_net_link_ptp, "ptp"},
  {test_net_link_ptp_forward, "ptp forward"},
  {test_net_link_ptp_forward_down, "ptp forward (down)"},
  {test_net_link_ptmp, "ptmp"},
  {test_net_link_ptmp_forward, "ptmp forward"},
  {test_net_link_ptmp_forward_unreach, "ptmp forward (unreach)"},
  {test_net_link_tunnel, "tunnel"},
  {test_net_link_tunnel_forward, "tunnel forward"},
};
#define TEST_NET_LINK_SIZE ARRAY_SIZE(TEST_NET_LINK)

SUnitTest TEST_NET_NETWORK[]= {
  {test_net_network, "network"},
  {test_net_network_add_node, "network add node"},
  {test_net_network_add_node_dup, "network add node (duplicate)"},
  {test_net_network_add_subnet, "network add subnet"},
  {test_net_network_add_subnet_dup, "network add subnet (duplicate)"},
};
#define TEST_NET_NETWORK_SIZE ARRAY_SIZE(TEST_NET_NETWORK)

SUnitTest TEST_NET_IGP[]= {
  {test_net_igp_domain, "igp domain"},
  {test_net_igp_compute, "igp compute"},
};
#define TEST_NET_IGP_SIZE ARRAY_SIZE(TEST_NET_IGP)

SUnitTest TEST_NET_TRACES[]= {
  {test_net_traces_ping, "ping"},
  {test_net_traces_traceroute, "traceroute"},
  {test_net_traces_recordroute, "recordroute"},
};
#define TEST_NET_TRACES_SIZE ARRAY_SIZE(TEST_NET_TRACES)

SUnitTest TEST_BGP_ATTR[]= {
  {test_bgp_attr_aspath, "as-path"},
  {test_bgp_attr_aspath_prepend, "as-path prepend"},
  {test_bgp_attr_aspath_prepend_too_much, "as-path prepend (too much)"},
  {test_bgp_attr_aspath_2str, "as-path (-> string)"},
  {test_bgp_attr_aspath_str2, "as-path (<- string)"},
  {test_bgp_attr_aspath_set_str2, "as-path set (<- string)"},
  {test_bgp_attr_aspath_cmp, "as-path (compare)"},
  {test_bgp_attr_aspath_contains, "as-path (contains)"},
  {test_bgp_attr_aspath_rem_private, "as-path remove private"},
  {test_bgp_attr_aspath_match, "as-path match"},
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

SUnitTest TEST_BGP_FILTER_ACTION[]= {
  {test_bgp_filter_action_comm_add, "comm add"},
  {test_bgp_filter_action_comm_remove, "comm remove"},
  {test_bgp_filter_action_comm_strip, "comm strip"},
  {test_bgp_filter_action_local_pref, "local-pref"},
  {test_bgp_filter_action_metric, "metric"},
  {test_bgp_filter_action_metric_internal, "metric internal"},
  {test_bgp_filter_action_path_prepend, "as-path prepend"},
  {test_bgp_filter_action_comm_add_str2, "comm add (string ->)"},
  {test_bgp_filter_action_comm_remove_str2, "comm remove (string ->)"},
  {test_bgp_filter_action_comm_strip_str2, "comm strip (string ->)"},
  {test_bgp_filter_action_local_pref_str2, "local-pref (string ->)"},
  {test_bgp_filter_action_metric_str2, "metric (string ->)"},
  {test_bgp_filter_action_metric_internal_str2, "metric internal (string ->)"},
  {test_bgp_filter_action_path_prepend_str2, "as-path prepend (string ->)"},
  {test_bgp_filter_action_expression_str2, "expression (string ->)"},
};
#define TEST_BGP_FILTER_ACTION_SIZE ARRAY_SIZE(TEST_BGP_FILTER_ACTION)

SUnitTest TEST_BGP_FILTER_PRED[]= {
  {test_bgp_filter_predicate_comm_contains, "comm contains"},
  {test_bgp_filter_predicate_nexthop_is, "next-hop is"},
  {test_bgp_filter_predicate_nexthop_in, "next-hop in"},
  {test_bgp_filter_predicate_prefix_is, "prefix is"},
  {test_bgp_filter_predicate_prefix_in, "prefix in"},
  {test_bgp_filter_predicate_prefix_ge, "prefix ge"},
  {test_bgp_filter_predicate_prefix_le, "prefix le"},
  {test_bgp_filter_predicate_path_regexp, "path regexp"},
  {test_bgp_filter_predicate_and, "and(x,y)"},
  {test_bgp_filter_predicate_and_any, "and(any,x)"},
  {test_bgp_filter_predicate_not, "not(x)"},
  {test_bgp_filter_predicate_not_not, "not(not(x))"},
  {test_bgp_filter_predicate_or, "or(x,y)"},
  {test_bgp_filter_predicate_or_any, "or(any,y)"},
  {test_bgp_filter_predicate_comm_contains_str2, "comm contains (string ->)"},
  {test_bgp_filter_predicate_nexthop_is_str2, "nexthop is (string ->)"},
  {test_bgp_filter_predicate_nexthop_in_str2, "nexthop in (string ->)"},
  {test_bgp_filter_predicate_prefix_is_str2, "prefix is (string ->)"},
  {test_bgp_filter_predicate_prefix_in_str2, "prefix in (string ->)"},
  {test_bgp_filter_predicate_prefix_ge_str2, "prefix ge (string ->)"},
  {test_bgp_filter_predicate_prefix_le_str2, "prefix le (string ->)"},
  {test_bgp_filter_predicate_path_regexp_str2, "path regexp (string ->)"},
  {test_bgp_filter_predicate_expr_str2, "expression (string ->)"},
};
#define TEST_BGP_FILTER_PRED_SIZE ARRAY_SIZE(TEST_BGP_FILTER_PRED)

SUnitTest TEST_BGP_PEER[]= {
  {test_bgp_peer, "create"},
};
#define TEST_BGP_PEER_SIZE ARRAY_SIZE(TEST_BGP_PEER)

SUnitTestSuite TEST_SUITES[]= {
  {"Net Attributes", TEST_NET_ATTR_SIZE, TEST_NET_ATTR},
  {"Net Nodes", TEST_NET_NODE_SIZE, TEST_NET_NODE},
  {"Net Subnets", TEST_NET_SUBNET_SIZE, TEST_NET_SUBNET},
  {"Net Interfaces", TEST_NET_IFACE_SIZE, TEST_NET_IFACE},
  {"Net Links", TEST_NET_LINK_SIZE, TEST_NET_LINK},
  {"Net Network", TEST_NET_NETWORK_SIZE, TEST_NET_NETWORK},
  {"Net IGP", TEST_NET_IGP_SIZE, TEST_NET_IGP},
  {"Net Traces", TEST_NET_TRACES_SIZE, TEST_NET_TRACES},
  {"BGP Attributes", TEST_BGP_ATTR_SIZE, TEST_BGP_ATTR},
  {"BGP Routes", TEST_BGP_ROUTE_SIZE, TEST_BGP_ROUTE},
  {"BGP Filter Actions", TEST_BGP_FILTER_ACTION_SIZE, TEST_BGP_FILTER_ACTION},
  {"BGP Filter Predicates", TEST_BGP_FILTER_PRED_SIZE, TEST_BGP_FILTER_PRED},
  {"BGP Peer", TEST_BGP_PEER_SIZE, TEST_BGP_PEER},
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
