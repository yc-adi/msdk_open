/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc., All Rights Reserved.
 * (now owned by Analog Devices, Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************
 *
 * Copyright 2023 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "netif/ethernet.h"

#include "lwipcfg.h"
#include "arch/cc.h"
#include "mxc_eth.h"

#include "mxc_device.h"
#include "emac.h"

static struct netif              mxc_eth_netif = {0};
static mxc_eth_config_t          mxc_eth_config = {0};
static int                       prev_link_status = -1;
static volatile unsigned int     eth_packet_is_in_que = 0;
static unsigned char             mxc_lwip_internal_buff[MXC_ETH_INTERNAL_BUFF_SIZE];

static err_t mxc_eth_netif_output(struct netif *netif, struct pbuf *p)
{
    int result;
    (void)(netif);

    LINK_STATS_INC(link.xmit);
    pbuf_copy_partial(p, mxc_lwip_internal_buff, p->tot_len, 0);

    result = MXC_EMAC_SendSync(mxc_lwip_internal_buff, p->tot_len);
    if (result)
        return ERR_TIMEOUT;

    return ERR_OK;
}

static err_t mxc_eth_netif_init(struct netif *netif)
{
    unsigned char hwaddr[MAC_LEN] = {MAC_BYTE1, MAC_BYTE2, MAC_BYTE3,
                                     MAC_BYTE4, MAC_BYTE5, MAC_BYTE6};

    netif->linkoutput = mxc_eth_netif_output;
    netif->output     = etharp_output;
    netif->mtu        = MXC_NETIF_MTU_SIZE;
    netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    memcpy(netif->hwaddr, hwaddr, MAC_LEN);
    netif->hwaddr_len = MAC_LEN;

    return ERR_OK;
}

static struct pbuf *get_recvd_frames(void)
{
    int             eth_data_len;
    unsigned char     *eth_data = &mxc_lwip_internal_buff[0];
    struct pbuf     *p = NULL;

    eth_data_len = MXC_EMAC_Recv(eth_data, MXC_ETH_INTERNAL_BUFF_SIZE);
    if ((eth_data_len < 0) || (eth_data_len > MXC_ETH_MAX_DATA_SIZE))
        return NULL;

    p = pbuf_alloc(PBUF_RAW, eth_data_len, PBUF_POOL);
    if (p != NULL)
        pbuf_take(p, eth_data, eth_data_len);

    return p;
}

int MXC_ETH_Init(mxc_eth_config_t *config)
{
    ip4_addr_t ipaddr, netmask, gw;

    if (!config)
        return E_NULL_PTR;

#if LWIP_NETIF_LINK_CALLBACK
    if (!config->link_callback)
        return E_NULL_PTR;
#endif

    if (!config->sys_get_ms)
        return E_NULL_PTR;

    memcpy(&mxc_eth_config, config, sizeof(mxc_eth_config_t));

#if NO_SYS
    lwip_init();
#else
    tcpip_init(config->init_done_callback, config->init_done_arg);
#endif

#if USE_DHCP
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);
    ip4_addr_set_zero(&gw);
#else
    LWIP_PORT_INIT_IPADDR(&ipaddr);
    LWIP_PORT_INIT_NETMASK(&netmask);
    LWIP_PORT_INIT_GW(&gw);
#endif

    netif_add(&mxc_eth_netif, &ipaddr, &netmask, &gw, NULL, mxc_eth_netif_init, netif_input);

    mxc_eth_netif.name[0] = config->name[0];
    mxc_eth_netif.name[1] = config->name[1];

#if LWIP_NETIF_LINK_CALLBACK
    netif_set_link_callback(&mxc_eth_netif, config->link_callback);
#endif

    netif_set_default(&mxc_eth_netif);
    netif_set_up(&mxc_eth_netif);

    return E_NO_ERROR;
}

void MXC_ETH_RecvIrq(void)
{
    eth_packet_is_in_que++;
}

int MXC_ETH_Tick(void)
{
    int         result;
    int         link_status;
    struct pbuf *p;

    /** Check Link State **/
    link_status = MXC_EMAC_ReadLinkStatus();
    if (link_status != prev_link_status) {
        if (link_status) {
            /* Link Down */
            netif_set_link_down(&mxc_eth_netif);

            result = MXC_EMAC_Stop();
            if (result)
                return result;
        } else {
            /* Link Up */
            result = MXC_EMAC_Start();
            if (result)
                return result;

            netif_set_link_up(&mxc_eth_netif);

#if USE_DHCP
            result = dhcp_start(&mxc_eth_netif);
            if (result)
                return result;
#endif
        }
    }
    prev_link_status = link_status;

    /** Check Received Frames **/
    if (eth_packet_is_in_que > 0) {
        __disable_irq();
        p = get_recvd_frames();
        eth_packet_is_in_que--;
        __enable_irq();
    } else {
        p = NULL;
    }

    if (p != NULL) {
        LINK_STATS_INC(link.recv);
        if (mxc_eth_netif.input(p, &mxc_eth_netif) != ERR_OK) {
            pbuf_free(p);
        }
    }

    /** Cyclic Timers Check **/
    sys_check_timeouts();

    return E_NO_ERROR;
}

u32_t sys_now(void)
{
    if (mxc_eth_config.sys_get_ms)
        return mxc_eth_config.sys_get_ms();
    else
        return 0;
}
