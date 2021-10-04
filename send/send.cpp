/*-
This program is to send a Ether packet to another server.
Author : Hox Zheng
Date : 2017年 12月 31日 星期日 15:21:04 CST
https://zenhox.github.io/2018/01/25/dpdk-pktSR/
 */

#include <cassert>
#include <cstdint>
#include <cinttypes>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <pthread.h>
#include <cstring>

#define RX_RING_SIZE 64
#define TX_RING_SIZE 64
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

//这里用skleten 默认配置
static const struct rte_eth_conf port_conf_default = {
	//.rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN },
	.rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_JUMBO_FRAME_LEN },
};


/*
 *这个是简单的端口初始化 
 *我在这里简单的端口0 初始化了一个 接收队列和一个发送队列
 *并且打印了一条被初始化的端口的MAC地址信息
 */
static inline int
port_init(struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	int retval;
	uint16_t q;


	/*配置端口0,给他分配一个接收队列和一个发送队列*/
	retval = rte_eth_dev_configure(0, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;
	
	
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(0, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(0), NULL);
		if (retval < 0)
			return retval;
	}


	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(0, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(0), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	
	retval = rte_eth_dev_set_mtu(0, 9000);
	if (retval < 0)
		return retval;
	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(0);
	if (retval < 0)
		return retval;

	return 0;
}

/*
我在main 函数里面 调用了输出化端口0的函数 
然后定义了以太网的头，以及报文。
申请了 m_pool
然后有在mempool里面 分配了两个m_buf
每个buf就是一个包
最后把他俩一次性发出去
 */
int main(int argc, char *argv[])
{
	//struct rte_mempool *mbuf_pool;

	/*进行总的初始话*/
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "initlize fail!");

	//I don't clearly know this two lines
	argc -= ret;
	argv += ret;

	/* Creates a new mempool in memory to hold the mbufs. */
	//分配内存池
	auto mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	//如果创建失败
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize all ports. */
	//初始话端口设备 顺便给他们分配  队列
		if (port_init(mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
					0);
	//定义报文信息
	struct Message {
		char data[10];
	};
	struct rte_ether_hdr *eth_hdr;
	struct Message obj = {{'H','e','l','l','o','2','0','1','8'}};
	struct Message *msg;
	//自己定义的包头
	struct rte_ether_addr s_addr = {{0x91,0xe2,0xba,0xec,0xab,0x32}};
	struct rte_ether_addr d_addr = {{0x01,0x1b,0x21,0x6e,0xd0,0x31}};
	uint16_t ether_type = 0x0a00; 	
	

	//对每个buf ， 给他们添加包
	auto mbuf_pool1=rte_mempool_lookup("MBUF_POOL");
	assert(mbuf_pool1);
	struct rte_mbuf * pkt[BURST_SIZE];
	int i;
	for(i=0;i<BURST_SIZE;i++) {
		pkt[i] = rte_pktmbuf_alloc(mbuf_pool1);
		eth_hdr = rte_pktmbuf_mtod(pkt[i],rte_ether_hdr*);
		eth_hdr->d_addr = d_addr;
		eth_hdr->s_addr = s_addr;
		eth_hdr->ether_type = ether_type;
		msg = (struct Message*) rte_pktmbuf_mtod_offset(pkt[i],char*, sizeof(struct rte_ether_hdr));
		*msg = obj;
		int pkt_size = sizeof(struct Message) + sizeof(struct rte_ether_hdr);
		pkt[i]->data_len = pkt_size;
		pkt[i]->pkt_len = pkt_size;
	}

	uint16_t nb_tx = rte_eth_tx_burst(0,0,pkt,BURST_SIZE);
	printf("发送成功%d个包\n",nb_tx);
	//发送完成，答应发送了多少个
	
	for(i=0;i<BURST_SIZE;i++)
		rte_pktmbuf_free(pkt[i]);

	return 0;
}
