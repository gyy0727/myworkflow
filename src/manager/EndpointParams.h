/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-25 17:55:20
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-25 17:55:21
 * @FilePath     : /myworkflow/src/manager/EndpointParams.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */
#ifndef _ENDPOINTPARAMS_H_
#define _ENDPOINTPARAMS_H_

#include <sys/types.h>
#include <sys/socket.h>

/**
 * @file   EndpointParams.h
 * @brief  Network config for client task
 */

enum TransportType
{
	TT_TCP,
	TT_UDP,
	TT_SCTP,
};

struct EndpointParams
{
	int address_family;
	size_t max_connections;
	int connect_timeout;
	int response_timeout;
};

static constexpr struct EndpointParams ENDPOINT_PARAMS_DEFAULT =
{
	.address_family			=	AF_UNSPEC,
	.max_connections		= 200,
	.connect_timeout		= 10 * 1000,
	.response_timeout		= 10 * 1000,
};

#endif

