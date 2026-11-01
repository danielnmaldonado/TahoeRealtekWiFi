/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2012 - 2017 Realtek Corporation.
 * Verbatim descriptor setters from pinned include/rtl8192e_xmit.h.
 * The host test supplies SET_BITS_TO_LE_4BYTE; no production codec helpers.
 * See rtl8192eu_txdesc_primary.json for the complete source checksum.
 */
#ifndef RTW_TEST_92E_PRIMARY_SETTERS_H
#define RTW_TEST_92E_PRIMARY_SETTERS_H
#define SET_TX_DESC_PKT_SIZE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc, 0, 16, __Value)
#define SET_TX_DESC_OFFSET_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc, 16, 8, __Value)
#define SET_TX_DESC_BMC_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc, 24, 1, __Value)
#define SET_TX_DESC_LAST_SEG_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc, 26, 1, __Value)
#define SET_TX_DESC_FIRST_SEG_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc, 27, 1, __Value)
#define SET_TX_DESC_OWN_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc, 31, 1, __Value)
#define SET_TX_DESC_MACID_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 0, 7, __Value)
#define SET_TX_DESC_QUEUE_SEL_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 8, 5, __Value)
#define SET_TX_DESC_RATE_ID_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 16, 5, __Value)
#define SET_TX_DESC_AGG_ENABLE_92E(__pTxDesc, __Value)		SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 12, 1, __Value)
#define SET_TX_DESC_BK_92E(__pTxDesc, __Value)				SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 16, 1, __Value)
#define SET_TX_DESC_AMPDU_DENSITY_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 20, 3, __Value)
#define SET_TX_DESC_USE_RATE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 8, 1, __Value)
#define SET_TX_DESC_CTS2SELF_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 11, 1, __Value)
#define SET_TX_DESC_RTS_ENABLE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 12, 1, __Value)
#define SET_TX_DESC_HW_RTS_ENABLE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 13, 1, __Value)
#define SET_TX_DESC_MAX_AGG_NUM_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 17, 5, __Value)
#define SET_TX_DESC_TX_RATE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 0, 7, __Value)
#define SET_TX_DESC_DATA_RATE_FB_LIMIT_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 8, 5, __Value)
#define SET_TX_DESC_RTS_RATE_FB_LIMIT_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 13, 4, __Value)
#define SET_TX_DESC_RETRY_LIMIT_ENABLE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 17, 1, __Value)
#define SET_TX_DESC_DATA_RETRY_LIMIT_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 18, 6, __Value)
#define SET_TX_DESC_RTS_RATE_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 24, 5, __Value)
#define SET_TX_DESC_DATA_SC_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 0, 4, __Value)
#define SET_TX_DESC_DATA_SHORT_92E(__pTxDesc, __Value)	SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 4, 1, __Value)
#define SET_TX_DESC_DATA_BW_92E(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 5, 2, __Value)
#define SET_TX_DESC_SEQ_92E(__pTxDesc, __Value)					SET_BITS_TO_LE_4BYTE(__pTxDesc+36, 12, 12, __Value)
#endif
