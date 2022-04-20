// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Wouter Franken <wouter.franken@mind.be>
 *
 * Here's a rough representation that shows the various buses that form the
 * Network On Chip (NOC) for the apq8064:
 *
 *                         Multimedia Subsystem (MMSS)
 *         |----------+-----------------------------------+-----------|
 *                    |                                   |
 *                    |                                   |
 *        Config      |                     Application   | Subsystem (APPSS)
 *       |------------+-+-----------|        |------------+-+-----------|
 *                      |                                   |
 *                      |                                   |
 *                      |             System                |
 *     |--------------+-+---------------------------------+-+-------------|
 *                    |                                   |
 *                    |                                   |
 *        Peripheral  |                           On Chip | Memory (OCMEM)
 *       |------------+-------------|        |------------+-------------|
 */

#include <dt-bindings/interconnect/qcom,apq8064.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interconnect-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/mfd/qcom_rpm.h>

#include <dt-bindings/mfd/qcom-rpm.h>

enum {
	APQ8064_AFAB_MAS_AMPSS_M0 = 1,
	APQ8064_AFAB_MAS_AMPSS_M1,
	APQ8064_AFAB_TO_MFAB,
	APQ8064_AFAB_TO_SFAB,
	APQ8064_AFAB_SLV_EBI_CH0,
	APQ8064_AFAB_SLV_EBI_CH1,
	APQ8064_AFAB_SLV_AMPSS_L2,
	APQ8064_MFAB_MAS_MDP_PORT0,
	APQ8064_MFAB_MAS_MDP_PORT1,
	APQ8064_MFAB_MAS_ROTATOR,
	APQ8064_MFAB_MAS_GRAPHICS_3D,
	APQ8064_MFAB_MAS_GRAPHICS_3D_PORT1,
	APQ8064_MFAB_MAS_JPEG_DEC,
	APQ8064_MFAB_MAS_VIDEO_CAP,
	APQ8064_MFAB_MAS_VIDEO_ENC,
	APQ8064_MFAB_MAS_VFE,
	APQ8064_MFAB_MAS_VPE,
	APQ8064_MFAB_MAS_JPEG_ENC,
	APQ8064_MFAB_MAS_VIDEO_DEC,
	APQ8064_MFAB_TO_AFAB,
	APQ8064_MFAB_SLV_MM_IMEM,
	APQ8064_SFAB_MAS_SPS,
	APQ8064_SFAB_MAS_ADM_PORT0,
	APQ8064_SFAB_MAS_ADM_PORT1,
	APQ8064_SFAB_MAS_LPASS_PROC,
	APQ8064_SFAB_MAS_GSS_NAV,
	APQ8064_SFAB_MAS_PCIE,
	APQ8064_SFAB_MAS_RIVA,
	APQ8064_SFAB_MAS_SATA,
	APQ8064_SFAB_MAS_CRYPTO,
	APQ8064_SFAB_MAS_LPASS,
	APQ8064_SFAB_MAS_MMSS_FPB,
	APQ8064_SFAB_MAS_ADM0_CI,
	APQ8064_SFAB_TO_AFAB,
	APQ8064_SFAB_TO_SYS_FPB,
	APQ8064_SFAB_TO_CPSS_FPB,
	APQ8064_SFAB_SLV_SPS,
	APQ8064_SFAB_SLV_SYSTEM_IMEM,
	APQ8064_SFAB_SLV_CORESIGHT,
	APQ8064_SFAB_SLV_PCIE,
	APQ8064_SFAB_SLV_CRYPTO,
	APQ8064_SFAB_SLV_RIVA,
	APQ8064_SFAB_SLV_SATA,
	APQ8064_SFAB_SLV_AMPSS,
	APQ8064_SFAB_SLV_GSS,
	APQ8064_SFAB_SLV_LPASS,
	APQ8064_SFAB_SLV_MMSS_FPB,
	APQ8064_SYS_FPB_MAS_SPDM,
	APQ8064_SYS_FPB_MAS_RPM,
	APQ8064_SYS_FPB_TO_SFAB,
	APQ8064_SYS_FPB_SLV_SPDM,
	APQ8064_SYS_FPB_SLV_RPM,
	APQ8064_SYS_FPB_SLV_RPM_MSG_RAM,
	APQ8064_SYS_FPB_SLV_MPM,
	APQ8064_SYS_FPB_SLV_PMIC1_SSBI1_A,
	APQ8064_SYS_FPB_SLV_PMIC1_SSBI1_B,
	APQ8064_SYS_FPB_SLV_PMIC1_SSBI1_C,
	APQ8064_SYS_FPB_SLV_PMIC2_SSBI2_A,
	APQ8064_SYS_FPB_SLV_PMIC2_SSBI2_B,
	APQ8064_CPSS_FPB_TO_SFAB,
	APQ8064_CPSS_FPB_SLV_GSBI1_UART,
	APQ8064_CPSS_FPB_SLV_GSBI2_UART,
	APQ8064_CPSS_FPB_SLV_GSBI3_UART,
	APQ8064_CPSS_FPB_SLV_GSBI4_UART,
	APQ8064_CPSS_FPB_SLV_GSBI5_UART,
	APQ8064_CPSS_FPB_SLV_GSBI6_UART,
	APQ8064_CPSS_FPB_SLV_GSBI7_UART,
	APQ8064_CPSS_FPB_SLV_GSBI8_UART,
	APQ8064_CPSS_FPB_SLV_GSBI9_UART,
	APQ8064_CPSS_FPB_SLV_GSBI10_UART,
	APQ8064_CPSS_FPB_SLV_GSBI11_UART,
	APQ8064_CPSS_FPB_SLV_GSBI12_UART,
	APQ8064_CPSS_FPB_SLV_GSBI1_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI2_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI3_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI4_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI5_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI6_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI7_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI8_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI9_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI10_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI11_QUP,
	APQ8064_CPSS_FPB_SLV_GSBI12_QUP,
	APQ8064_CPSS_FPB_SLV_EBI2_NAND,
	APQ8064_CPSS_FPB_SLV_EBI2_CS0,
	APQ8064_CPSS_FPB_SLV_EBI2_CS1,
	APQ8064_CPSS_FPB_SLV_EBI2_CS2,
	APQ8064_CPSS_FPB_SLV_EBI2_CS3,
	APQ8064_CPSS_FPB_SLV_EBI2_CS4,
	APQ8064_CPSS_FPB_SLV_EBI2_CS5,
	APQ8064_CPSS_FPB_SLV_USB_FS1,
	APQ8064_CPSS_FPB_SLV_USB_FS2,
	APQ8064_CPSS_FPB_SLV_TSIF,
	APQ8064_CPSS_FPB_SLV_MSM_TSSC,
	APQ8064_CPSS_FPB_SLV_MSM_PDM,
	APQ8064_CPSS_FPB_SLV_MSM_DIMEM,
	APQ8064_CPSS_FPB_SLV_MSM_TCSR,
	APQ8064_CPSS_FPB_SLV_MSM_PRNG,
};

#define to_apq8064_icc_provider(_provider) \
	container_of(_provider, struct apq8064_icc_provider, provider)

static const struct clk_bulk_data apq8064_icc_bus_clocks[] = {
	{ .id = "bus" },
	{ .id = "bus_a" },
};

/**
 * struct apq8064_icc_provider - Qualcomm specific interconnect provider
 * @provider: generic interconnect provider
 * @bus_clks: the clk_bulk_data table of bus clocks
 * @num_clks: the total number of clk_bulk_data entries
 * @arb: arbitration data for all nodes
 * @rpm: RPM handle
 * @rpm_id: RPM resource id for this interconnect
 */
struct apq8064_icc_provider {
	struct icc_provider provider;
	struct clk_bulk_data *bus_clks;
	int num_clks;
	uint16_t *bwsum;
	uint16_t *arb;
	u64 *actarb;
	struct qcom_rpm *rpm;
	int rpm_id;
	int num_mports;
	int num_sports;
	int num_tieredslaves;
};

#define APQ8064_ICC_MAX_LINKS	3
#define APQ8064_MAX_TIERS	2
#define APQ8064_MAX_MPORTS	2
#define APQ8064_MAX_SPORTS	2
#define APQ8064_DEFAULT_TIER 1

#define IS_GW(n) (((n)->num_mports > 0) && ((n)->num_sports > 0))
#define IS_MASTER(n) (((n)->num_mports > 0) && ((n)->num_sports == 0))
#define IS_SLAVE(n) (((n)->num_mports == 0) && ((n)->num_sports > 0))

/**
 * struct apq8064_icc_node - Qualcomm specific interconnect nodes
 * @name: the node name used in debugfs
 * @id: a unique node identifier
 * @buswidth: width of the interconnect between a node and the bus (bytes)
 * @mports: port ids for devices that are bus masters
 * @num_mports: amount of port ids in mports
 * @sports: port ids for devices that are bus slaves
 * @num_sports: amount of port ids in sports
 * @tiers: tiers this node belongs to
 * @num_tiers: number of tiers this node belongs to
 * @links: an array of nodes where we can go next while traversing
 * @num_links: the total number of @links
 * @rate: current bus clock rate in Hz
 */
struct apq8064_icc_node {
	unsigned char *name;
	u16 id;
	u16 buswidth;
	const int *mports;
	int num_mports;
	const int *sports;
	int num_sports;
	const int *tiers;
	int num_tiers;
	const int *links;
	int num_links;
	u64 rate;
};

struct apq8064_icc_desc {
	int rpm_id;
	struct apq8064_icc_node **nodes;
	size_t num_nodes;
};

static const int mas_ampss_m0_mports[] = {0};

static struct apq8064_icc_node mas_ampss_m0 = {
	.name = "mas_ampss_m0",
	.id = APQ8064_AFAB_MAS_AMPSS_M0,
	.buswidth = 8,
	.mports = mas_ampss_m0_mports,
	.num_mports = ARRAY_SIZE(mas_ampss_m0_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_ampss_m1_mports[] = {1};

static struct apq8064_icc_node mas_ampss_m1 = {
	.name = "mas_ampss_m1",
	.id = APQ8064_AFAB_MAS_AMPSS_M1,
	.buswidth = 8,
	.mports = mas_ampss_m1_mports,
	.num_mports = ARRAY_SIZE(mas_ampss_m1_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};


static const int afab_to_mfab_mports[] = {2, 3};

static const int afab_to_mfab_sports[] = {3};

static const int afab_to_mfab_links[] = {
	APQ8064_AFAB_SLV_EBI_CH0
};

static struct apq8064_icc_node afab_to_mfab = {
	.name = "afab_to_mfab",
	.id = APQ8064_AFAB_TO_MFAB,
	.buswidth = 8,
	.mports = afab_to_mfab_mports,
	.num_mports = ARRAY_SIZE(afab_to_mfab_mports),
	.sports = afab_to_mfab_sports,
	.num_sports = ARRAY_SIZE(afab_to_mfab_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = afab_to_mfab_links,
	.num_links = ARRAY_SIZE(afab_to_mfab_links),
};

static const int afab_to_sfab_mports[] = {4, 5};

static const int afab_to_sfab_sports[] = {4};

static struct apq8064_icc_node afab_to_sfab = {
	.name = "afab_to_sfab",
	.id = APQ8064_AFAB_TO_SFAB,
	.buswidth = 8,
	.mports = afab_to_sfab_mports,
	.num_mports = ARRAY_SIZE(afab_to_sfab_mports),
	.sports = afab_to_sfab_sports,
	.num_sports = ARRAY_SIZE(afab_to_sfab_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};


static const int slv_ebi_ch0_sports[] = {0, 1};

static const int slv_ebi_ch0_tiers[] = {0, 1};

static struct apq8064_icc_node slv_ebi_ch0 = {
	.name = "slv_ebi_ch0",
	.id = APQ8064_AFAB_SLV_EBI_CH0,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_ebi_ch0_sports,
	.num_sports = ARRAY_SIZE(slv_ebi_ch0_sports),
	.tiers = slv_ebi_ch0_tiers,
	.num_tiers = ARRAY_SIZE(slv_ebi_ch0_tiers),
	.links = NULL,
	.num_links = 0,
};

static const int slv_ebi_ch1_sports[] = {1};

static const int slv_ebi_ch1_tiers[] = {1};

static struct apq8064_icc_node slv_ebi_ch1 = {
	.name = "slv_ebi_ch1",
	.id = APQ8064_AFAB_SLV_EBI_CH1,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_ebi_ch1_sports,
	.num_sports = ARRAY_SIZE(slv_ebi_ch1_sports),
	.tiers = slv_ebi_ch1_tiers,
	.num_tiers = ARRAY_SIZE(slv_ebi_ch1_tiers),
	.links = NULL,
	.num_links = 0,
};

static const int slv_ampss_l2_sports[] = {2};

static const int slv_ampss_l2_tiers[] = {2};

static struct apq8064_icc_node slv_ampss_l2 = {
	.name = "slv_ampss_l2",
	.id = APQ8064_AFAB_SLV_AMPSS_L2,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_ampss_l2_sports,
	.num_sports = ARRAY_SIZE(slv_ampss_l2_sports),
	.tiers = slv_ampss_l2_tiers,
	.num_tiers = ARRAY_SIZE(slv_ampss_l2_tiers),
	.links = NULL,
	.num_links = 0,
};


static struct apq8064_icc_node *apq8064_afab_nodes[] = {
	[AFAB_MAS_AMPSS_M0] = &mas_ampss_m0,
	[AFAB_MAS_AMPSS_M1] = &mas_ampss_m1,
	[AFAB_TO_MFAB] = &afab_to_mfab,
	[AFAB_TO_SFAB] = &afab_to_sfab,
	[AFAB_SLV_EBI_CH0] = &slv_ebi_ch0,
	[AFAB_SLV_EBI_CH1] = &slv_ebi_ch1,
	[AFAB_SLV_AMPSS_L2] = &slv_ampss_l2,
};

static struct apq8064_icc_desc apq8064_afab = {
	.rpm_id = QCOM_RPM_APPS_FABRIC_ARB,
	.nodes = apq8064_afab_nodes,
	.num_nodes = ARRAY_SIZE(apq8064_afab_nodes),
};

static const int mas_mdp_p0_mports[] = {0};

static int mas_mdp_p0_links[] = {
	APQ8064_MFAB_TO_AFAB
};

 static struct apq8064_icc_node mas_mdp_p0 = {
	.name = "mas_mdp_p0",
	.id = APQ8064_MFAB_MAS_MDP_PORT0,
	.buswidth = 8,
	.mports = mas_mdp_p0_mports,
	.num_mports = ARRAY_SIZE(mas_mdp_p0_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = mas_mdp_p0_links,
	.num_links = ARRAY_SIZE(mas_mdp_p0_links),
};

static const int mas_mdp_m1_mports[] = {1};

static struct apq8064_icc_node mas_mdp_m1 = {
	.name = "mas_mdp_m1",
	.id = APQ8064_MFAB_MAS_MDP_PORT1,
	.buswidth = 8,
	.mports = mas_mdp_m1_mports,
	.num_mports = ARRAY_SIZE(mas_mdp_m1_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_rot_mports[] = {2};

static struct apq8064_icc_node mas_rot = {
	.name = "mas_rot",
	.id = APQ8064_MFAB_MAS_ROTATOR,
	.buswidth = 8,
	.mports = mas_rot_mports,
	.num_mports = ARRAY_SIZE(mas_rot_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_gfx3d_mports[] = {3};

static struct apq8064_icc_node mas_gfx3d = {
	.name = "mas_gfx3d",
	.id = APQ8064_MFAB_MAS_GRAPHICS_3D,
	.buswidth = 8,
	.mports = mas_gfx3d_mports,
	.num_mports = ARRAY_SIZE(mas_gfx3d_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_gfx3d_p1_mports[] = {4};

static struct apq8064_icc_node mas_gfx3d_p1 = {
	.name = "mas_gfx3d_p1",
	.id = APQ8064_MFAB_MAS_GRAPHICS_3D_PORT1,
	.buswidth = 8,
	.mports = mas_gfx3d_p1_mports,
	.num_mports = ARRAY_SIZE(mas_gfx3d_p1_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_jpeg_mports[] = {5};

static struct apq8064_icc_node mas_jpeg = {
	.name = "mas_jpeg",
	.id = APQ8064_MFAB_MAS_JPEG_DEC,
	.buswidth = 8,
	.mports = mas_jpeg_mports,
	.num_mports = ARRAY_SIZE(mas_jpeg_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_video_cap_mports[] = {6};

static struct apq8064_icc_node mas_video_cap = {
	.name = "mas_video_cap",
	.id = APQ8064_MFAB_MAS_VIDEO_CAP,
	.buswidth = 8,
	.mports = mas_video_cap_mports,
	.num_mports = ARRAY_SIZE(mas_video_cap_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_video_enc_mports[] = {12};

static struct apq8064_icc_node mas_video_enc = {
	.name = "mas_video_enc",
	.id = APQ8064_MFAB_MAS_VIDEO_ENC,
	.buswidth = 8,
	.mports = mas_video_enc_mports,
	.num_mports = ARRAY_SIZE(mas_video_enc_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_vfe_mports[] = {7};

static struct apq8064_icc_node mas_vfe = {
	.name = "mas_vfe",
	.id = APQ8064_MFAB_MAS_VFE,
	.buswidth = 8,
	.mports = mas_vfe_mports,
	.num_mports = ARRAY_SIZE(mas_vfe_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_vpe_mports[] = {8};

static struct apq8064_icc_node mas_vpe = {
	.name = "mas_vpe",
	.id = APQ8064_MFAB_MAS_VPE,
	.buswidth = 8,
	.mports = mas_vpe_mports,
	.num_mports = ARRAY_SIZE(mas_vpe_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_jpeg_enc_mports[] = {9};

static struct apq8064_icc_node mas_jpeg_enc = {
	.name = "mas_jpeg_enc",
	.id = APQ8064_MFAB_MAS_JPEG_ENC,
	.buswidth = 8,
	.mports = mas_jpeg_enc_mports,
	.num_mports = ARRAY_SIZE(mas_jpeg_enc_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_video_dec_mports[] = {10};

static struct apq8064_icc_node mas_video_dec = {
	.name = "mas_video_dec",
	.id = APQ8064_MFAB_MAS_VIDEO_DEC,
	.buswidth = 8,
	.mports = mas_video_dec_mports,
	.num_mports = ARRAY_SIZE(mas_video_dec_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};


static const int mfab_to_afab_mports[] = {11};

static const int mfab_to_afab_sports[] = {1, 2};

static const int mfab_to_afab_tiers[] = {1, 2};

static const int mfab_to_afab_links[] = {
	APQ8064_AFAB_TO_MFAB
};

static struct apq8064_icc_node mfab_to_afab = {
	.name = "mfab_to_afab",
	.id = APQ8064_MFAB_TO_AFAB,
	.buswidth = 8,
	.mports = mfab_to_afab_mports,
	.num_mports = ARRAY_SIZE(mfab_to_afab_mports),
	.sports = mfab_to_afab_sports,
	.num_sports = ARRAY_SIZE(mfab_to_afab_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = mfab_to_afab_links,
	.num_links = ARRAY_SIZE(mfab_to_afab_links),
};


static const int slv_mm_imem_sports[] = {0};

static const int slv_mm_imem_tiers[] = {0};

static struct apq8064_icc_node slv_mm_imem = {
	.name = "slv_mm_imem",
	.id = APQ8064_MFAB_SLV_MM_IMEM,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_mm_imem_sports,
	.num_sports = ARRAY_SIZE(slv_mm_imem_sports),
	.tiers = slv_mm_imem_tiers,
	.num_tiers = ARRAY_SIZE(slv_mm_imem_tiers),
	.links = NULL,
	.num_links = 0,
};


static struct apq8064_icc_node *apq8064_mfab_nodes[] = {
	[MFAB_MAS_MDP_PORT0] = &mas_mdp_p0,
	[MFAB_MAS_MDP_PORT1] = &mas_mdp_m1,
	[MFAB_MAS_ROTATOR] = &mas_rot,
	[MFAB_MAS_GRAPHICS_3D] = &mas_gfx3d,
	[MFAB_MAS_GRAPHICS_3D_PORT1] = &mas_gfx3d_p1,
	[MFAB_MAS_JPEG_DEC] = &mas_jpeg,
	[MFAB_MAS_VIDEO_CAP] = &mas_video_cap,
	[MFAB_MAS_VIDEO_ENC] = &mas_video_enc,
	[MFAB_MAS_VFE] = &mas_vfe,
	[MFAB_MAS_VPE] = &mas_vpe,
	[MFAB_MAS_JPEG_ENC] = &mas_jpeg_enc,
	[MFAB_MAS_VIDEO_DEC] = &mas_video_dec,
	[MFAB_TO_AFAB] = &mfab_to_afab,
	[MFAB_SLV_MM_IMEM] = &slv_mm_imem,
};

static struct apq8064_icc_desc apq8064_mfab = {
	.rpm_id = QCOM_RPM_MM_FABRIC_ARB,
	.nodes = apq8064_mfab_nodes,
	.num_nodes = ARRAY_SIZE(apq8064_mfab_nodes),
};

static const int mas_sps_mports[] = {1};

static struct apq8064_icc_node mas_sps = {
	.name = "mas_sps",
	.id = APQ8064_SFAB_MAS_SPS,
	.buswidth = 8,
	.mports = mas_sps_mports,
	.num_mports = ARRAY_SIZE(mas_sps_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_adm_p0_mports[] = {2};

static struct apq8064_icc_node mas_adm_p0 = {
	.name = "mas_adm_p0",
	.id = APQ8064_SFAB_MAS_ADM_PORT0,
	.buswidth = 8,
	.mports = mas_adm_p0_mports,
	.num_mports = ARRAY_SIZE(mas_adm_p0_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_adm_p1_mports[] = {3};

static struct apq8064_icc_node mas_adm_p1 = {
	.name = "mas_adm_p1",
	.id = APQ8064_SFAB_MAS_ADM_PORT1,
	.buswidth = 8,
	.mports = mas_adm_p1_mports,
	.num_mports = ARRAY_SIZE(mas_adm_p1_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_lpass_proc_mports[] = {4};

static struct apq8064_icc_node mas_lpass_proc = {
	.name = "mas_lpass_proc",
	.id = APQ8064_SFAB_MAS_LPASS_PROC,
	.buswidth = 8,
	.mports = mas_lpass_proc_mports,
	.num_mports = ARRAY_SIZE(mas_lpass_proc_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_gss_nav_mports[] = {5};

static struct apq8064_icc_node mas_gss_nav = {
	.name = "mas_gss_nav",
	.id = APQ8064_SFAB_MAS_GSS_NAV,
	.buswidth = 8,
	.mports = mas_gss_nav_mports,
	.num_mports = ARRAY_SIZE(mas_gss_nav_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_pcie_mports[] = {6};

static struct apq8064_icc_node mas_pcie = {
	.name = "mas_pcie",
	.id = APQ8064_SFAB_MAS_PCIE,
	.buswidth = 8,
	.mports = mas_pcie_mports,
	.num_mports = ARRAY_SIZE(mas_pcie_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_riva_mports[] = {7};

static struct apq8064_icc_node mas_riva = {
	.name = "mas_riva",
	.id = APQ8064_SFAB_MAS_RIVA,
	.buswidth = 8,
	.mports = mas_riva_mports,
	.num_mports = ARRAY_SIZE(mas_riva_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_sata_mports[] = {8};

static struct apq8064_icc_node mas_sata = {
	.name = "mas_sata",
	.id = APQ8064_SFAB_MAS_SATA,
	.buswidth = 8,
	.mports = mas_sata_mports,
	.num_mports = ARRAY_SIZE(mas_sata_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_crypto_mports[] = {13};

static struct apq8064_icc_node mas_crypto = {
	.name = "mas_crypto",
	.id = APQ8064_SFAB_MAS_CRYPTO,
	.buswidth = 8,
	.mports = mas_crypto_mports,
	.num_mports = ARRAY_SIZE(mas_crypto_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_lpass_mports[] = {9};

static struct apq8064_icc_node mas_lpass = {
	.name = "mas_lpass",
	.id = APQ8064_SFAB_MAS_LPASS,
	.buswidth = 8,
	.mports = mas_lpass_mports,
	.num_mports = ARRAY_SIZE(mas_lpass_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_mmss_fpb_mports[] = {0};

static struct apq8064_icc_node mas_mmss_fpb = {
	.name = "mas_mmss_fpb",
	.id = APQ8064_SFAB_MAS_MMSS_FPB,
	.buswidth = 8,
	.mports = mas_mmss_fpb_mports,
	.num_mports = ARRAY_SIZE(mas_mmss_fpb_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int mas_adm0_ci_mports[] = {13};

static struct apq8064_icc_node mas_adm0_ci = {
	.name = "mas_adm0_ci",
	.id = APQ8064_SFAB_MAS_ADM0_CI,
	.buswidth = 8,
	.mports = mas_adm0_ci_mports,
	.num_mports = ARRAY_SIZE(mas_adm0_ci_mports),
	.sports = NULL,
	.num_sports = 0,
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};


static const int sfab_to_afab_mports[] = {0};

static const int sfab_to_afab_sports[] = {0, 1};

static const int sfab_to_afab_tiers[] = {0, 1};

static struct apq8064_icc_node sfab_to_afab = {
	.name = "sfab_to_afab",
	.id = APQ8064_SFAB_TO_AFAB,
	.buswidth = 8,
	.mports = sfab_to_afab_mports,
	.num_mports = ARRAY_SIZE(sfab_to_afab_mports),
	.sports = sfab_to_afab_sports,
	.num_sports = ARRAY_SIZE(sfab_to_afab_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int sfab_to_sys_fpb_mports[] = {11};

static const int sfab_to_sys_fpb_sports[] = {10};

static struct apq8064_icc_node sfab_to_sys_fpb = {
	.name = "sfab_to_sys_fpb",
	.id = APQ8064_SFAB_TO_SYS_FPB,
	.buswidth = 8,
	.mports = sfab_to_sys_fpb_mports,
	.num_mports = ARRAY_SIZE(sfab_to_sys_fpb_mports),
	.sports = sfab_to_sys_fpb_sports,
	.num_sports = ARRAY_SIZE(sfab_to_sys_fpb_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int sfab_to_cpss_fpb_mports[] = {10};

static const int sfab_to_cpss_fpb_sports[] = {9};

static struct apq8064_icc_node sfab_to_cpss_fpb = {
	.name = "sfab_to_cpss_fpb",
	.id = APQ8064_SFAB_TO_CPSS_FPB,
	.buswidth = 8,
	.mports = sfab_to_cpss_fpb_mports,
	.num_mports = ARRAY_SIZE(sfab_to_cpss_fpb_mports),
	.sports = sfab_to_cpss_fpb_sports,
	.num_sports = ARRAY_SIZE(sfab_to_cpss_fpb_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_mmss_fpb_mports[] = {12};

static const int slv_mmss_fpb_sports[] = {11};

static struct apq8064_icc_node slv_mmss_fpb = {
	.name = "slv_mmss_fpb",
	.id = APQ8064_SFAB_SLV_MMSS_FPB,
	.buswidth = 8,
	.mports = slv_mmss_fpb_mports,
	.num_mports = ARRAY_SIZE(slv_mmss_fpb_mports),
	.sports = slv_mmss_fpb_sports,
	.num_sports = ARRAY_SIZE(slv_mmss_fpb_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};


static const int slv_sps_sports[] = {2};

static struct apq8064_icc_node slv_sps = {
	.name = "slv_sps",
	.id = APQ8064_SFAB_SLV_SPS,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_sps_sports,
	.num_sports = ARRAY_SIZE(slv_sps_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_sys_imem_sports[] = {3};

static const int slv_sys_imem_tiers[] = {2};

static struct apq8064_icc_node slv_sys_imem = {
	.name = "slv_sys_imem",
	.id = APQ8064_SFAB_SLV_SYSTEM_IMEM,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_sys_imem_sports,
	.num_sports = ARRAY_SIZE(slv_sys_imem_sports),
	.tiers = slv_sys_imem_tiers,
	.num_tiers = ARRAY_SIZE(slv_sys_imem_tiers),
	.links = NULL,
	.num_links = 0,
};

static const int slv_coresight_sports[] = {4};

static struct apq8064_icc_node slv_coresight = {
	.name = "slv_coresight",
	.id = APQ8064_SFAB_SLV_CORESIGHT,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_coresight_sports,
	.num_sports = ARRAY_SIZE(slv_coresight_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_pcie_sports[] = {5};

static struct apq8064_icc_node slv_pcie = {
	.name = "slv_pcie",
	.id = APQ8064_SFAB_SLV_PCIE,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_pcie_sports,
	.num_sports = ARRAY_SIZE(slv_pcie_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_crypto_sports[] = {14};

static struct apq8064_icc_node slv_crypto = {
	.name = "slv_crypto",
	.id = APQ8064_SFAB_SLV_CRYPTO,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_crypto_sports,
	.num_sports = ARRAY_SIZE(slv_crypto_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_riva_sports[] = {12};

static struct apq8064_icc_node slv_riva = {
	.name = "slv_riva",
	.id = APQ8064_SFAB_SLV_RIVA,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_riva_sports,
	.num_sports = ARRAY_SIZE(slv_riva_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_sata_sports[] = {13};

static struct apq8064_icc_node slv_sata = {
	.name = "slv_sata",
	.id = APQ8064_SFAB_SLV_SATA,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_sata_sports,
	.num_sports = ARRAY_SIZE(slv_sata_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_ampss_sports[] = {6};

static struct apq8064_icc_node slv_ampss = {
	.name = "slv_ampss",
	.id = APQ8064_SFAB_SLV_AMPSS,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_ampss_sports,
	.num_sports = ARRAY_SIZE(slv_ampss_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_gss_sports[] = {7};

static struct apq8064_icc_node slv_gss = {
	.name = "slv_gss",
	.id = APQ8064_SFAB_SLV_GSS,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_gss_sports,
	.num_sports = ARRAY_SIZE(slv_gss_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};

static const int slv_lpass_sports[] = {8};

static struct apq8064_icc_node slv_lpass = {
	.name = "slv_lpass",
	.id = APQ8064_SFAB_SLV_LPASS,
	.buswidth = 8,
	.mports = NULL,
	.num_mports = 0,
	.sports = slv_lpass_sports,
	.num_sports = ARRAY_SIZE(slv_lpass_sports),
	.tiers = NULL,
	.num_tiers = 0,
	.links = NULL,
	.num_links = 0,
};


static struct apq8064_icc_node *apq8064_sfab_nodes[] = {
	[SFAB_MAS_SPS] = &mas_sps,
	[SFAB_MAS_ADM_PORT0] = &mas_adm_p0,
	[SFAB_MAS_ADM_PORT1] = &mas_adm_p1,
	[SFAB_MAS_LPASS_PROC] = &mas_lpass_proc,
	[SFAB_MAS_GSS_NAV] = &mas_gss_nav,
	[SFAB_MAS_PCIE] = &mas_pcie,
	[SFAB_MAS_RIVA] = &mas_riva,
	[SFAB_MAS_SATA] = &mas_sata,
	[SFAB_MAS_CRYPTO] = &mas_crypto,
	[SFAB_MAS_LPASS] = &mas_lpass,
	[SFAB_MAS_MMSS_FPB] = &mas_mmss_fpb,
	[SFAB_MAS_ADM0_CI] = &mas_adm0_ci,
	[SFAB_TO_AFAB] = &sfab_to_afab,
	[SFAB_TO_SYS_FPB] = &sfab_to_sys_fpb,
	[SFAB_TO_CPSS_FPB] = &sfab_to_cpss_fpb,
	[SFAB_SLV_SPS] = &slv_sps,
	[SFAB_SLV_SYSTEM_IMEM] = &slv_sys_imem,
	[SFAB_SLV_CORESIGHT] = &slv_coresight,
	[SFAB_SLV_PCIE] = &slv_pcie,
	[SFAB_SLV_CRYPTO] = &slv_crypto,
	[SFAB_SLV_RIVA] = &slv_riva,
	[SFAB_SLV_SATA] = &slv_sata,
	[SFAB_SLV_AMPSS] = &slv_ampss,
	[SFAB_SLV_GSS] = &slv_gss,
	[SFAB_SLV_LPASS] = &slv_lpass,
	[SFAB_SLV_MMSS_FPB] = &slv_mmss_fpb,
};

static struct apq8064_icc_desc apq8064_sfab = {
	.rpm_id = QCOM_RPM_SYS_FABRIC_ARB,
	.nodes = apq8064_sfab_nodes,
	.num_nodes = ARRAY_SIZE(apq8064_sfab_nodes),
};

// TODO: Add CSSP FPB and SYS FPB?
/*static void apq8064_icc_rpm_smd_send(struct device *dev, int rsc_type,*/
					 /*char *name, int id, u64 val)*/
/*{*/
	/*int ret;*/

	/*if (id == -1)*/
		/*return;*/

	/*
	 * Setting the bandwidth requests for some nodes fails and this same
	 * behavior occurs on the downstream MSM 3.4 kernel sources based on
	 * errors like this in that kernel:
	 *
	 *   msm_rpm_get_error_from_ack(): RPM NACK Unsupported resource
	 *   AXI: msm_bus_rpm_req(): RPM: Ack failed
	 *   AXI: msm_bus_rpm_commit_arb(): RPM: Req fail: mas:32, bw:240000000
	 *
	 * Since there's no publicly available documentation for this hardware,
	 * and the bandwidth for some nodes in the path can be set properly,
	 * let's not return an error.
	 */
	/*ret = qcom_icc_rpm_smd_send(QCOM_SMD_RPM_ACTIVE_STATE, rsc_type, id,*/
					/*val);*/
	/*if (ret)*/
		/*dev_dbg(dev, "Cannot set bandwidth for node %s (%d): %d\n",*/
			/*name, id, ret);*/
/*}*/

#define MSM_BUS_GET_BW_INFO(val, type, bw) \
	do { \
		(type) = MSM_BUS_GET_BW_TYPE(val); \
		(bw) = MSM_BUS_GET_BW(val);	\
	} while (0)


#define MSM_BUS_GET_BW_INFO_BYTES (val, type, bw) \
	do { \
		(type) = MSM_BUS_GET_BW_TYPE(val); \
		(bw) = msm_bus_get_bw_bytes(val); \
	} while (0)


#define TIER_SHIFT 15
#define TIERMASK 1 << TIER_SHIFT
#define TIER_1 1 << TIER_SHIFT
#define TIER_2 0 << TIER_SHIFT
#define BWMASK 0x7FFF
#define MAX_BW BWMASK

#define ROUNDED_BW_VAL_FROM_BYTES(bw) \
	((((bw) >> 17) + 1) & TIERMASK ? MAX_BW : (((bw) >> 17) + 1))

#define BW_VAL_FROM_BYTES(bw) \
	((((bw) >> 17) & TIERMASK) ? MAX_BW : ((bw) >> 17))

static uint32_t msm_bus_set_bw_bytes(unsigned long bw)
{
	return ((((bw) & 0x1FFFF) && (((bw) >> 17) == 0)) ?
		ROUNDED_BW_VAL_FROM_BYTES(bw) : BW_VAL_FROM_BYTES(bw));

}

uint64_t msm_bus_get_bw_bytes(unsigned long val)
{
	return ((val) & BWMASK) << 17;
}

uint16_t msm_bus_get_bw(unsigned long val)
{
	return (val) & BWMASK;
}

static uint16_t msm_bus_create_bw_tier_pair_bytes(uint8_t type,
	unsigned long bw)
{
	return (((type) == 0 ? TIER_1 : TIER_2) |
	 (msm_bus_set_bw_bytes(bw)));
};

uint16_t msm_bus_create_bw_tier_pair(uint8_t type, unsigned long bw)
{
	return ((type) == 0 ? TIER_1 : TIER_2) | ((bw) & BWMASK);
}

#define GET_TIER(n) (((n) & TIERMASK) >> 15)

static int apq8064_icc_set(struct icc_node *src, struct icc_node *dst)
{
	struct apq8064_icc_node *src_fn, *dst_fn;
	struct apq8064_icc_provider *fp;
	u64 sum_bw, max_peak_bw, rate;
	u32 agg_avg = 0, agg_peak = 0;
	struct icc_provider *provider;
	struct icc_node *n;
	int ret, i, j, ports, tiers, index;
	u64 interleavedbw;

	src_fn = src->data;
	dst_fn = dst->data;
	provider = src->provider;
	fp = to_apq8064_icc_provider(provider);

	list_for_each_entry(n, &provider->nodes, node_list)
		provider->aggregate(n, 0, n->avg_bw, n->peak_bw,
				    &agg_avg, &agg_peak);

	sum_bw = icc_units_to_bps(agg_avg);
	max_peak_bw = icc_units_to_bps(agg_peak);

	do_div(sum_bw, src_fn->num_mports);
	ports = src_fn->num_mports;
	tiers = dst_fn->num_tiers;
	for (i = 0; i < tiers; i++) {
		for (j = 0; j < ports; j++) {
			uint16_t dst_tier;
			/*
			 * For interleaved gateway ports and slave ports,
			 * there is one-one mapping between gateway port and
			 * the slave port
			 */
			if (IS_GW(src_fn) && i != j &&
				(dst_fn->num_sports > 1))
				continue;

			if (!dst_fn->tiers)
				dst_tier = 1;
			else
				dst_tier = dst_fn->tiers[i];
			index = ((dst_tier * fp->num_mports) + src_fn->mports[j]);
			/* If there is tier, calculate arb for commit */
			if (dst_fn->tiers) {
				uint16_t tier = 1;
				u64 tieredbw = fp->actarb[index];

				/*
				 * Make sure gateway to slave port bandwidth
				 * is not divided when slave is interleaved
				 */
				tieredbw = sum_bw;
				if (!IS_GW(src_fn) || dst_fn->num_sports <= 1)
					do_div(tieredbw, dst_fn->num_sports);

				/* Update Arb for fab,get HW Mport from enum */
				fp->arb[index] =
					msm_bus_create_bw_tier_pair_bytes(tier,
					tieredbw);
				fp->actarb[index] = sum_bw;
			}
		}
	}

	/* Update bwsum for slaves on fabric */
	ports = dst_fn->num_sports;
	for (i = 0; i < ports; i++) {
		interleavedbw = sum_bw;
		do_div(interleavedbw, dst_fn->num_sports);
		fp->bwsum[dst_fn->sports[i]]
			= (uint16_t)msm_bus_create_bw_tier_pair_bytes(0, interleavedbw);
	}

	rate = max(sum_bw, max_peak_bw);

	do_div(rate, src_fn->buswidth);

	rate = min_t(u32, rate, INT_MAX);

	if (src_fn->rate == rate)
		return 0;

	for (i = 0; i < fp->num_clks; i++) {
		ret = clk_set_rate(fp->bus_clks[i].clk, rate);
		if (ret) {
			dev_err(provider->dev, "%s clk_set_rate error: %d\n",
				fp->bus_clks[i].id, ret);
			ret = 0;
		}
	}

	src_fn->rate = rate;

	return 0;
}

/*static int apq8064_icc_set(struct icc_node *src, struct icc_node *dst)*/
/*{*/
	/*struct apq8064_icc_node *src_fn, *dst_fn;*/
	/*struct apq8064_icc_provider *fp;*/
	/*u64 sum_bw, max_peak_bw, rate;*/
	/*u32 agg_avg = 0, agg_peak = 0;*/
	/*struct icc_provider *provider;*/
	/*struct icc_node *n;*/
	/*int ret, i;*/

	/*src_fn = src->data;*/
	/*dst_fn = dst->data;*/
	/*provider = src->provider;*/
	/*fp = to_apq8064_icc_provider(provider);*/

	/*sum_bw = icc_units_to_bps(agg_avg);*/
	/*max_peak_bw = icc_units_to_bps(agg_peak);*/

	/*// TODO: set bw on internal arb struct*/

	/*// TODO: What to do with this, just remove it?*/
	/**/
	/**/
	/*rate = max(sum_bw, max_peak_bw);*/

	/*do_div(rate, src_fn->buswidth);*/

	/*rate = min_t(u32, rate, INT_MAX);*/

	/*if (src_fn->rate == rate)*/
		/*return 0;*/

	/*for (i = 0; i < qp->num_clks; i++) {*/
		/*ret = clk_set_rate(fp->bus_clks[i].clk, rate);*/
		/*if (ret) {*/
			/*dev_err(provider->dev, "%s clk_set_rate error: %d\n",*/
				/*qp->bus_clks[i].id, ret);*/
			/*ret = 0;*/
		/*}*/
	/*}*/

	/*src_fn->rate = rate;*/
	/**/

	/*return 0;*/
/*}*/

static int apq8064_icc_commit(struct icc_provider *p)
{
	int i, count, index = 0;
	uint32_t *rpm_data;
	struct apq8064_icc_provider *fp;
	int ret = 0;
	
	fp = to_apq8064_icc_provider(p);

	count = ((fp->num_mports * fp->num_tieredslaves) + (fp->num_sports) + 1)/2;
	rpm_data = kzalloc((sizeof(uint32_t *) * count), GFP_KERNEL);

	/*
	 * Copy bwsum to rpm data
	 * Since bwsum is uint16, the values need to be adjusted to
	 * be copied to value field of rpm-data, which is 32 bits.
	 */
	for (i = 0; i < (fp->num_sports - 1); i += 2) {
		rpm_data[index] = (*(fp->bwsum + i + 1)) << 16 | *(fp->bwsum + i);
		index++;
	}
	/* Account for odd number of slaves */
	if (fp->num_sports & 1) {
		rpm_data[index] = (*(fp->arb)) << 16 | *(fp->bwsum + i);
		index++;
		i = 1;
	} else
		i = 0;

	/* Copy arb values to rpm data */
	for (; i < (fp->num_tieredslaves * fp->num_mports); i += 2) {
		rpm_data[index] = (*(fp->arb + i + 1)) << 16 | *(fp->arb + i);
		index++;
	}
	
	ret = qcom_rpm_write(fp->rpm, QCOM_RPM_ACTIVE_STATE, fp->rpm_id, rpm_data, count);
	if (ret)
		goto out;

	ret = qcom_rpm_write(fp->rpm, QCOM_RPM_SLEEP_STATE, fp->rpm_id, rpm_data, count);
	if (ret)
		goto out;

out:
	kfree(rpm_data);
	return ret;
}

static int apq8064_get_bw(struct icc_node *node, u32 *avg, u32 *peak)
{
	*avg = 0;
	*peak = 0;

	return 0;
}

static int apq8064_icc_probe(struct platform_device *pdev)
{
	const struct apq8064_icc_desc *desc;
	struct apq8064_icc_node **fnodes;
	struct apq8064_icc_provider *fp;
	struct device *dev = &pdev->dev;
	struct icc_onecell_data *data;
	struct icc_provider *provider;
	struct icc_node *node;
	size_t num_nodes, i;
	int ret;

	/*[> wait for the RPM proxy <]*/
	/*if (!qcom_icc_rpm_available())*/
		/*return -EPROBE_DEFER;*/

	desc = of_device_get_match_data(dev);
	if (!desc)
		return -EINVAL;

	fnodes = desc->nodes;
	num_nodes = desc->num_nodes;

	fp = devm_kzalloc(dev, sizeof(*fp), GFP_KERNEL);
	if (!fp)
		return -ENOMEM;

	data = devm_kzalloc(dev, struct_size(data, nodes, num_nodes),
			    GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	fp->bus_clks = devm_kmemdup(dev, apq8064_icc_bus_clocks,
				    sizeof(apq8064_icc_bus_clocks), GFP_KERNEL);
	if (!fp->bus_clks)
		return -ENOMEM;

	fp->num_clks = ARRAY_SIZE(apq8064_icc_bus_clocks);
	ret = devm_clk_bulk_get(dev, fp->num_clks, fp->bus_clks);
	if (ret)
		return ret;

	ret = clk_bulk_prepare_enable(fp->num_clks, fp->bus_clks);
	if (ret)
		return ret;

	fp->rpm = dev_get_drvdata(pdev->dev.parent);
	if (!fp->rpm) {
		dev_err(&pdev->dev, "unable to retrieve handle to RPM\n");
		return -ENODEV;
	}

	fp->rpm_id = desc->rpm_id;

	provider = &fp->provider;
	INIT_LIST_HEAD(&provider->nodes);
	provider->dev = dev;
	provider->set = apq8064_icc_set;
	provider->aggregate = icc_std_aggregate;
	provider->xlate = of_icc_xlate_onecell;
	provider->data = data;
	provider->get_bw = apq8064_get_bw;
	provider->commit = apq8064_icc_commit;

	ret = icc_provider_add(provider);
	if (ret) {
		dev_err(dev, "error adding interconnect provider: %d\n", ret);
		goto err_disable_clks;
	}

	for (i = 0; i < num_nodes; i++) {
		size_t j;

		node = icc_node_create(fnodes[i]->id);
		if (IS_ERR(node)) {
			ret = PTR_ERR(node);
			goto err_del_icc;
		}

		node->name = fnodes[i]->name;
		node->data = fnodes[i];
		icc_node_add(node, provider);

		dev_dbg(dev, "registered node %s\n", node->name);

		/* populate links */
		for (j = 0; j < fnodes[i]->num_links; j++)
			icc_link_create(node, fnodes[i]->links[j]);

		/* Calculate amount of mports */
		for (j = 0; j < fnodes[i]->num_mports; j++)
			fp->num_mports = max(fp->num_mports, fnodes[i]->mports[j]);

		/* Calculate amount of sports */
		for (j = 0; j < fnodes[i]->num_sports; j++)
			fp->num_sports = max(fp->num_sports, fnodes[i]->sports[j]);

		/* Calculate amount of tiered sports */
		for (j = 0; j < fnodes[i]->num_sports; j++)
			fp->num_tieredslaves = max(fp->num_tieredslaves, fnodes[i]->sports[j]);

		data->nodes[i] = node;
	}
	data->num_nodes = num_nodes;

	fp->num_mports++;
	fp->num_sports++;
	fp->num_tieredslaves++;

	fp->bwsum = devm_kzalloc(dev, (sizeof(uint16_t *) * fp->num_sports), GFP_KERNEL);
	if (!fp->bwsum)
		return -ENOMEM;

	fp->arb = devm_kzalloc(dev, (sizeof(uint16_t *) * (fp->num_tieredslaves * fp->num_mports) + 1), GFP_KERNEL);
	if (!fp->arb)
		return -ENOMEM;

	fp->actarb = devm_kzalloc(dev, (sizeof(u64 *) * (fp->num_tieredslaves * fp->num_mports) + 1), GFP_KERNEL);
	if (!fp->actarb)
		return -ENOMEM;

	platform_set_drvdata(pdev, fp);

	return 0;

err_del_icc:
	icc_nodes_remove(provider);
	icc_provider_del(provider);

err_disable_clks:
	clk_bulk_disable_unprepare(fp->num_clks, fp->bus_clks);

	return ret;
}

static int apq8064_icc_remove(struct platform_device *pdev)
{
	struct apq8064_icc_provider *fp = platform_get_drvdata(pdev);

	icc_nodes_remove(&fp->provider);
	clk_bulk_disable_unprepare(fp->num_clks, fp->bus_clks);
	return icc_provider_del(&fp->provider);
}

static const struct of_device_id apq8064_fabric_of_match[] = {
	{ .compatible = "qcom,apq8064-afab", .data = &apq8064_afab},
	{ .compatible = "qcom,apq8064-sfab", .data = &apq8064_sfab},
	{ .compatible = "qcom,apq8064-mfab", .data = &apq8064_mfab},
	{ },
};
MODULE_DEVICE_TABLE(of, apq8064_fabric_of_match);

static struct platform_driver apq8064_fabric_driver = {
	.probe = apq8064_icc_probe,
	.remove = apq8064_icc_remove,
	.driver = {
		.name = "fab-apq8064",
		.of_match_table = apq8064_fabric_of_match,
		.sync_state = icc_sync_state,
	},
};
module_platform_driver(apq8064_fabric_driver);
MODULE_DESCRIPTION("Qualcomm APQ8064 FABRIC driver");
MODULE_AUTHOR("Wouter Franken <wouter.franken@mind.be>");
MODULE_LICENSE("GPL v2");
