/* Compatibility header for Helio DVFSRC MT6768 (v2 backport)
 * Define apenas o que faltar (via ifndef) para satisfazer o v2.
 * Offsets de dump (RECORD/MD) vão para uma janela neutra só para compilar.
 */
#ifndef __HELIO_DVFSRC_COMPAT_H
#define __HELIO_DVFSRC_COMPAT_H

/* Base */
#ifndef DVFSRC_BASIC_CONTROL
#define DVFSRC_BASIC_CONTROL        0x000
#endif
#ifndef DVFSRC_SW_REQ
#define DVFSRC_SW_REQ               0x004
#endif
#ifndef DVFSRC_SW_REQ2
#define DVFSRC_SW_REQ2              0x008
#endif
#ifndef DVFSRC_FORCE
#define DVFSRC_FORCE                0x00C
#endif
#ifndef DVFSRC_VCORE_REQUEST
#define DVFSRC_VCORE_REQUEST        0x048
#endif

/* SW bandwidth knobs */
#ifndef DVFSRC_SW_BW_0
#define DVFSRC_SW_BW_0              0x100
#endif
#ifndef DVFSRC_SW_BW_1
#define DVFSRC_SW_BW_1              0x104
#endif
#ifndef DVFSRC_SW_BW_2
#define DVFSRC_SW_BW_2              0x108
#endif
#ifndef DVFSRC_SW_BW_3
#define DVFSRC_SW_BW_3              0x10C
#endif
#ifndef DVFSRC_SW_BW_4
#define DVFSRC_SW_BW_4              0x110
#endif

/* EMI / QoS */
#ifndef DVFSRC_EMI_REQUEST
#define DVFSRC_EMI_REQUEST          0x118
#endif
#ifndef DVFSRC_EMI_REQUEST3
#define DVFSRC_EMI_REQUEST3         0x11C
#endif
#ifndef DVFSRC_EMI_HRT
#define DVFSRC_EMI_HRT              0x120
#endif
#ifndef DVFSRC_EMI_QOS0
#define DVFSRC_EMI_QOS0             0x124
#endif
#ifndef DVFSRC_EMI_QOS1
#define DVFSRC_EMI_QOS1             0x128
#endif

/* MD<->SPM (v2 usa vários destes) */
#ifndef DVFSRC_EMI_MD2SPM0
#define DVFSRC_EMI_MD2SPM0          0x12C
#endif
#ifndef DVFSRC_EMI_MD2SPM1
#define DVFSRC_EMI_MD2SPM1          0x130
#endif
#ifndef DVFSRC_EMI_MD2SPM2
#define DVFSRC_EMI_MD2SPM2          0x134
#endif
#ifndef DVFSRC_EMI_MD2SPM0_T
#define DVFSRC_EMI_MD2SPM0_T        0x138
#endif
#ifndef DVFSRC_EMI_MD2SPM1_T
#define DVFSRC_EMI_MD2SPM1_T        0x13C
#endif
#ifndef DVFSRC_EMI_MD2SPM2_T
#define DVFSRC_EMI_MD2SPM2_T        0x140
#endif

#ifndef DVFSRC_VCORE_HRT
#define DVFSRC_VCORE_HRT            0x144
#endif
#ifndef DVFSRC_MD_SW_CONTROL
#define DVFSRC_MD_SW_CONTROL        0x148
#endif
#ifndef DVFSRC_TIMEOUT_NEXTREQ
#define DVFSRC_TIMEOUT_NEXTREQ      0x14C
#endif
#ifndef DVFSRC_INT_EN
#define DVFSRC_INT_EN               0x150
#endif

/* Labels dos níveis */
#ifndef DVFSRC_LEVEL_LABEL_0_1
#define DVFSRC_LEVEL_LABEL_0_1      0x154
#endif
#ifndef DVFSRC_LEVEL_LABEL_2_3
#define DVFSRC_LEVEL_LABEL_2_3      0x158
#endif
#ifndef DVFSRC_LEVEL_LABEL_4_5
#define DVFSRC_LEVEL_LABEL_4_5      0x15C
#endif
#ifndef DVFSRC_LEVEL_LABEL_6_7
#define DVFSRC_LEVEL_LABEL_6_7      0x160
#endif
#ifndef DVFSRC_LEVEL_LABEL_8_9
#define DVFSRC_LEVEL_LABEL_8_9      0x164
#endif
#ifndef DVFSRC_LEVEL_LABEL_10_11
#define DVFSRC_LEVEL_LABEL_10_11    0x168
#endif
#ifndef DVFSRC_LEVEL_LABEL_12_13
#define DVFSRC_LEVEL_LABEL_12_13    0x16C
#endif
#ifndef DVFSRC_LEVEL_LABEL_14_15
#define DVFSRC_LEVEL_LABEL_14_15    0x170
#endif

#ifndef DVFSRC_RSRV_1
#define DVFSRC_RSRV_1               0x174
#endif
#ifndef DVFSRC_QOS_EN
#define DVFSRC_QOS_EN               0x178
#endif
#ifndef DVFSRC_EMI_QOS2
#define DVFSRC_EMI_QOS2             0x17C
#endif
#ifndef DVFSRC_EMI_REQUEST2
#define DVFSRC_EMI_REQUEST2         0x180
#endif
#ifndef DVFSRC_VCORE_REQUEST2
#define DVFSRC_VCORE_REQUEST2       0x184
#endif
#ifndef DVFSRC_VCORE_MD2SPM0
#define DVFSRC_VCORE_MD2SPM0        0x188
#endif
#ifndef DVFSRC_VCORE_MD2SPM1
#define DVFSRC_VCORE_MD2SPM1        0x18C
#endif
#ifndef DVFSRC_VCORE_MD2SPM2
#define DVFSRC_VCORE_MD2SPM2        0x190
#endif
#ifndef DVFSRC_VCORE_MD2SPM0_T
#define DVFSRC_VCORE_MD2SPM0_T      0x194
#endif
#ifndef DVFSRC_VCORE_MD2SPM1_T
#define DVFSRC_VCORE_MD2SPM1_T      0x198
#endif
#ifndef DVFSRC_VCORE_MD2SPM2_T
#define DVFSRC_VCORE_MD2SPM2_T      0x19C
#endif

/* Outros vistos no v2 */
#ifndef DVFSRC_MD_REQUEST
#define DVFSRC_MD_REQUEST           0x1A0
#endif
#ifndef DVFSRC_INT
#define DVFSRC_INT                  0x1A4
#endif
#ifndef DVFSRC_SEC_SW_REQ
#define DVFSRC_SEC_SW_REQ           0x1A8
#endif
#ifndef DVFSRC_MD_SCENARIO
#define DVFSRC_MD_SCENARIO          0x1AC
#endif
#ifndef DVFSRC_LAST
#define DVFSRC_LAST                 0x1B0
#endif
#ifndef DVFSRC_LEVEL
#define DVFSRC_LEVEL                0x1B4
#endif

/* RECORDs de dump — endereços neutros só pra compilar */
#ifndef DVFSRC_RECORD_BASE
#define DVFSRC_RECORD_BASE          0x300
#endif
#define __DVFSRC_REC_OF(i,j) (DVFSRC_RECORD_BASE + (((i)*3+(j))<<2))

#ifndef DVFSRC_RECORD_0_0
#define DVFSRC_RECORD_0_0           __DVFSRC_REC_OF(0,0)
#endif
#ifndef DVFSRC_RECORD_0_1
#define DVFSRC_RECORD_0_1           __DVFSRC_REC_OF(0,1)
#endif
#ifndef DVFSRC_RECORD_0_2
#define DVFSRC_RECORD_0_2           __DVFSRC_REC_OF(0,2)
#endif
#ifndef DVFSRC_RECORD_1_0
#define DVFSRC_RECORD_1_0           __DVFSRC_REC_OF(1,0)
#endif
#ifndef DVFSRC_RECORD_1_1
#define DVFSRC_RECORD_1_1           __DVFSRC_REC_OF(1,1)
#endif
#ifndef DVFSRC_RECORD_1_2
#define DVFSRC_RECORD_1_2           __DVFSRC_REC_OF(1,2)
#endif
#ifndef DVFSRC_RECORD_2_0
#define DVFSRC_RECORD_2_0           __DVFSRC_REC_OF(2,0)
#endif
#ifndef DVFSRC_RECORD_2_1
#define DVFSRC_RECORD_2_1           __DVFSRC_REC_OF(2,1)
#endif
#ifndef DVFSRC_RECORD_2_2
#define DVFSRC_RECORD_2_2           __DVFSRC_REC_OF(2,2)
#endif
#ifndef DVFSRC_RECORD_3_0
#define DVFSRC_RECORD_3_0           __DVFSRC_REC_OF(3,0)
#endif
#ifndef DVFSRC_RECORD_3_1
#define DVFSRC_RECORD_3_1           __DVFSRC_REC_OF(3,1)
#endif
#ifndef DVFSRC_RECORD_3_2
#define DVFSRC_RECORD_3_2           __DVFSRC_REC_OF(3,2)
#endif
#ifndef DVFSRC_RECORD_4_0
#define DVFSRC_RECORD_4_0           __DVFSRC_REC_OF(4,0)
#endif
#ifndef DVFSRC_RECORD_4_1
#define DVFSRC_RECORD_4_1           __DVFSRC_REC_OF(4,1)
#endif
#ifndef DVFSRC_RECORD_4_2
#define DVFSRC_RECORD_4_2           __DVFSRC_REC_OF(4,2)
#endif
#ifndef DVFSRC_RECORD_5_0
#define DVFSRC_RECORD_5_0           __DVFSRC_REC_OF(5,0)
#endif
#ifndef DVFSRC_RECORD_5_1
#define DVFSRC_RECORD_5_1           __DVFSRC_REC_OF(5,1)
#endif
#ifndef DVFSRC_RECORD_5_2
#define DVFSRC_RECORD_5_2           __DVFSRC_REC_OF(5,2)
#endif
#ifndef DVFSRC_RECORD_6_0
#define DVFSRC_RECORD_6_0           __DVFSRC_REC_OF(6,0)
#endif
#ifndef DVFSRC_RECORD_6_1
#define DVFSRC_RECORD_6_1           __DVFSRC_REC_OF(6,1)
#endif
#ifndef DVFSRC_RECORD_6_2
#define DVFSRC_RECORD_6_2           __DVFSRC_REC_OF(6,2)
#endif
#ifndef DVFSRC_RECORD_7_0
#define DVFSRC_RECORD_7_0           __DVFSRC_REC_OF(7,0)
#endif
#ifndef DVFSRC_RECORD_7_1
#define DVFSRC_RECORD_7_1           __DVFSRC_REC_OF(7,1)
#endif
#ifndef DVFSRC_RECORD_7_2
#define DVFSRC_RECORD_7_2           __DVFSRC_REC_OF(7,2)
#endif

/* RECORDs “MD_*” numa subjanela neutra */
#ifndef DVFSRC_RECORD_MD_BASE
#define DVFSRC_RECORD_MD_BASE       (DVFSRC_RECORD_BASE + 0x80)
#endif
#ifndef DVFSRC_RECORD_MD_0
#define DVFSRC_RECORD_MD_0          (DVFSRC_RECORD_MD_BASE + 0x00)
#endif
#ifndef DVFSRC_RECORD_MD_1
#define DVFSRC_RECORD_MD_1          (DVFSRC_RECORD_MD_BASE + 0x04)
#endif
#ifndef DVFSRC_RECORD_MD_2
#define DVFSRC_RECORD_MD_2          (DVFSRC_RECORD_MD_BASE + 0x08)
#endif
#ifndef DVFSRC_RECORD_MD_3
#define DVFSRC_RECORD_MD_3          (DVFSRC_RECORD_MD_BASE + 0x0C)
#endif
#ifndef DVFSRC_RECORD_MD_4
#define DVFSRC_RECORD_MD_4          (DVFSRC_RECORD_MD_BASE + 0x10)
#endif
#ifndef DVFSRC_RECORD_MD_5
#define DVFSRC_RECORD_MD_5          (DVFSRC_RECORD_MD_BASE + 0x14)
#endif
#ifndef DVFSRC_RECORD_MD_6
#define DVFSRC_RECORD_MD_6          (DVFSRC_RECORD_MD_BASE + 0x18)
#endif

#endif /* __HELIO_DVFSRC_COMPAT_H */
#ifndef DVFSRC_RECORD_MD_7
#define DVFSRC_RECORD_MD_7          (DVFSRC_RECORD_MD_BASE + 0x1C)
#endif

/* Contador genérico de records (janela neutra só p/ compilar) */
#ifndef DVFSRC_RECORD_COUNT
#define DVFSRC_RECORD_COUNT         (DVFSRC_RECORD_BASE + 0xFC)
#endif

/* Reserva neutra: se o teu tree não definir, aponta p/ bloco “record” */
#ifndef DVFSRC_RSRV_0
#define DVFSRC_RSRV_0               (DVFSRC_RECORD_BASE + 0xF4)
#endif
