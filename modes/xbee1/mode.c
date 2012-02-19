/*
  libxbee - a C library to aid the use of Digi's XBee wireless modules
            running in API mode (AP=2).

  Copyright (C) 2009  Attie Grande (attie@attie.co.uk)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../../internal.h"
#include "../../xbee_int.h"
#include "../../log.h"
#include "../../mode.h"
#include "../../frame.h"
#include "../common.h"
#include "mode.h"
#include "at.h"
#include "data.h"
#include "io.h"

static xbee_err shutdown(struct xbee *xbee);
static xbee_err init(struct xbee *xbee, va_list ap);

/* ######################################################################### */

static xbee_err init(struct xbee *xbee, va_list ap) {
	xbee_err ret;
	char *t;
	struct xbee_modeData *data;
	
	if (!xbee) return XBEE_EMISSINGPARAM;
	
	if ((data = malloc(sizeof(*data))) == NULL) return XBEE_ENOMEM;
	memset(data, 0, sizeof(*data));
	
	ret = XBEE_ENONE;
	
	/* currently I don't see a better way than this - using va_arg()... which is gross */	
	t = va_arg(ap, char*);
	if ((data->serialInfo.device = malloc(strlen(t) + 1)) == NULL) { ret = XBEE_ENOMEM; goto die; }
	strcpy(data->serialInfo.device, t);
	
	data->serialInfo.baudrate = va_arg(ap, int);
	
	if ((ret = xbee_serialSetup(&data->serialInfo)) != XBEE_ENONE) goto die;
	
	xbee->modeData = data;
	return XBEE_ENONE;
die:
	shutdown(xbee);
	return ret;
}

static xbee_err shutdown(struct xbee *xbee) {
	struct xbee_modeData *data;
	
	if (!xbee) return XBEE_EMISSINGPARAM;
	if (!xbee->mode || !xbee->modeData) return XBEE_EINVAL;
	
	data = xbee->modeData;
	
	if (data->serialInfo.f) fclose(data->serialInfo.f);
	if (data->serialInfo.fd != -1) close(data->serialInfo.fd);
	if (data->serialInfo.device) free(data->serialInfo.device);
	if (data->serialInfo.txBuf) free(data->serialInfo.txBuf);
	free(xbee->modeData);
	
	return XBEE_ENONE;
}

/* ######################################################################### */

xbee_err xbee_s1_transmitStatus_rx_func(struct xbee *xbee, unsigned char identifier, struct xbee_buf *buf, struct xbee_frameInfo *frameInfo, struct xbee_conAddress *address, struct xbee_pkt **pkt) {
	xbee_err ret;

	if (!xbee || !frameInfo || !buf || !address || !pkt) return XBEE_EMISSINGPARAM;
	
	ret	= XBEE_ENONE;
	
	if (buf->len != 3) {
		ret = XBEE_ELENGTH;
		goto die1;
	}
	
	frameInfo->active = 1;
	frameInfo->id = buf->data[1];
	frameInfo->retVal = buf->data[2];
	
	memset(address, 0, sizeof(*address));
	
	goto done;
die1:
done:
	return 0;
}

/* ######################################################################### */

const struct xbee_modeDataHandlerRx xbee_s1_transmitStatus_rx  = {
	.identifier = 0x89,
	.func = xbee_s1_transmitStatus_rx_func,
};
const struct xbee_modeConType xbee_s1_transmitStatus = {
	.name = "Transmit Status",
	.allowFrameId = 0,
	.useTimeout = 0,
	.rxHandler = &xbee_s1_transmitStatus_rx,
	.txHandler = NULL,
};

/* ######################################################################### */

static const struct xbee_modeConType *conTypes[] = {
/*&xbee_s1_modemStatus,*/
  &xbee_s1_transmitStatus,
	&xbee_s1_localAt,
	&xbee_s1_localAtQueued,
	&xbee_s1_remoteAt,
	&xbee_s1_16bitData,
	&xbee_s1_64bitData,
	&xbee_s1_16bitIo,
	&xbee_s1_64bitIo,
	NULL
};

struct xbee_mode mode_xbee1 = {
	.name = "xbee1",
	
	.conTypes = conTypes,
	
	.init = init,
	.shutdown = shutdown,
	
	.rx_io = xbee_xbeeRxIo,
	.tx_io = xbee_xbeeTxIo,
	
	.thread = NULL,
};

