/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <context.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>

#include "hi3798mv2x.h"
#include "plat_private.h"

#define REG_PERI_CPU_RVBARADDR		0xF8A80034
#define REG_PERI_CPU_AARCH_MODE		0xF8A80030

#define REG_CPU_LP_CPU_SW_BEGIN		10
#define CPU_REG_COREPO_SRST		12
#define CPU_REG_CORE_SRST		8

static void hi3798mv2x_cpu_standby(plat_local_state_t cpu_state)
{
	dsb();
	wfi();
}

static int hi3798mv2x_pwr_domain_on(u_register_t mpidr)
{
	unsigned int cpu = plat_core_pos_by_mpidr(mpidr);
	unsigned int regval, regval_bak;

	/* Select 400MHz before start slave cores */
	regval_bak = mmio_read_32((uintptr_t)(REG_BASE_CRG + REG_CPU_LP));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_LP), 0x206);
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_LP), 0x606);

	/* Clear the slave cpu arm_por_srst_req reset */
	regval = mmio_read_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST));
	regval &= ~(1 << (cpu + CPU_REG_COREPO_SRST));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST), regval);

	/* Clear the slave cpu reset */
	regval = mmio_read_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST));
	regval &= ~(1 << (cpu + CPU_REG_CORE_SRST));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST), regval);

	/* Restore cpu frequency */
	regval = regval_bak & (~(1 << REG_CPU_LP_CPU_SW_BEGIN));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_LP), regval);
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_LP), regval_bak);

	return PSCI_E_SUCCESS;
}

static void hi3798mv2x_pwr_domain_off(const psci_power_state_t *target_state)
{
	assert(0);
}

static void hi3798mv2x_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	assert(0);
}

static void hi3798mv2x_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	assert(target_state->pwr_domain_state[MPIDR_AFFLVL0] ==
					PLAT_MAX_OFF_STATE);

	/* Enable the gic cpu interface */
	hi3798mv2x_gic_pcpu_init();

	/* Program the gic per-cpu distributor or re-distributor interface */
	hi3798mv2x_gic_cpuif_enable();
}

static void hi3798mv2x_pwr_domain_suspend_finish(
		const psci_power_state_t *target_state)
{
	assert(0);
}

static void __dead2 hi3798mv2x_system_off(void)
{
	ERROR("Hi3798mv2x System Off: operation not handled.\n");
	panic();
}

static void __dead2 hi3798mv2x_system_reset(void)
{
	mmio_write_32((uintptr_t)(HISI_WDG0_BASE + 0xc00), 0x1ACCE551);
	mmio_write_32((uintptr_t)(HISI_WDG0_BASE + 0x0),   0x00000100);
	mmio_write_32((uintptr_t)(HISI_WDG0_BASE + 0x8),   0x00000003);

	wfi();
	ERROR("Hi3798mv2x System Reset: operation not handled.\n");
	panic();
}

static int32_t hi3798mv2x_validate_power_state(unsigned int power_state,
					   psci_power_state_t *req_state)
{
	VERBOSE("%s: power_state: 0x%x\n", __func__, power_state);

	int pstate = psci_get_pstate_type(power_state);

	assert(req_state);

	/* Sanity check the requested state */
	if (pstate == PSTATE_TYPE_STANDBY)
		req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_MAX_RET_STATE;
	else
		req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_MAX_OFF_STATE;

	/* We expect the 'state id' to be zero */
	if (psci_get_pstate_id(power_state))
		return PSCI_E_INVALID_PARAMS;

	return PSCI_E_SUCCESS;
}

static int hi3798mv2x_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	if ((entrypoint >= DDR_BASE) && (entrypoint < (DDR_BASE + DDR_SIZE)))
		return PSCI_E_SUCCESS;

	return PSCI_E_INVALID_ADDRESS;
}

static void hi3798mv2x_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;

	for (i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

static const plat_psci_ops_t hi3798mv2x_plat_psci_ops = {
	.cpu_standby			= hi3798mv2x_cpu_standby,
	.pwr_domain_on			= hi3798mv2x_pwr_domain_on,
	.pwr_domain_off			= hi3798mv2x_pwr_domain_off,
	.pwr_domain_suspend		= hi3798mv2x_pwr_domain_suspend,
	.pwr_domain_on_finish		= hi3798mv2x_pwr_domain_on_finish,
	.pwr_domain_suspend_finish	= hi3798mv2x_pwr_domain_suspend_finish,
	.system_off			= hi3798mv2x_system_off,
	.system_reset			= hi3798mv2x_system_reset,
	.validate_power_state		= hi3798mv2x_validate_power_state,
	.validate_ns_entrypoint		= hi3798mv2x_validate_ns_entrypoint,
	.get_sys_suspend_power_state	= hi3798mv2x_get_sys_suspend_power_state,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	*psci_ops = &hi3798mv2x_plat_psci_ops;

	mmio_write_32((uintptr_t)REG_PERI_CPU_AARCH_MODE, 0xF);
	mmio_write_32((uintptr_t)REG_PERI_CPU_RVBARADDR, sec_entrypoint);
	return 0;
}
