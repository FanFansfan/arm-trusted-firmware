/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <drivers/arm/gicv2.h>
#include <plat/common/platform.h>

/******************************************************************************
 * On a GICv2 system, the Group 1 secure interrupts are treated as Group 0
 * interrupts.
 *****************************************************************************/
static const interrupt_prop_t hi3798mv2x_interrupt_props[] = {
	HI3798MV2X_G1S_IRQ_PROPS(GICV2_INTR_GROUP0),
	HI3798MV2X_G0_IRQ_PROPS(GICV2_INTR_GROUP0)
};

static unsigned int target_mask_array[PLATFORM_CORE_COUNT];

static const gicv2_driver_data_t hi3798mv2x_gic_data = {
	.gicd_base = HI3798MV2X_GICD_BASE,
	.gicc_base = HI3798MV2X_GICC_BASE,
	.interrupt_props = hi3798mv2x_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(hi3798mv2x_interrupt_props),
	.target_masks = target_mask_array,
	.target_masks_num = ARRAY_SIZE(target_mask_array),
};

/******************************************************************************
 * Helper to initialize the GICv2 only driver.
 *****************************************************************************/
void hi3798mv2x_gic_driver_init(void)
{
	gicv2_driver_init(&hi3798mv2x_gic_data);
}

void hi3798mv2x_gic_init(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
	gicv2_cpuif_enable();
}

/******************************************************************************
 * Helper to enable the GICv2 CPU interface
 *****************************************************************************/
void hi3798mv2x_gic_cpuif_enable(void)
{
	gicv2_cpuif_enable();
}

/******************************************************************************
 * Helper to initialize the per cpu distributor interface in GICv2
 *****************************************************************************/
void hi3798mv2x_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
}
