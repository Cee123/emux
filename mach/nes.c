#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <controller.h>
#include <cpu.h>
#include <log.h>
#include <machine.h>
#include <memory.h>
#include <resource.h>
#include <util.h>

/* Clock frequencies */
#define MASTER_CLOCK_RATE	21477272.0f
#define CPU_CLOCK_RATE		(MASTER_CLOCK_RATE / 12)
#define PPU_CLOCK_RATE		(MASTER_CLOCK_RATE / 4)
#define APU_CLOCK_RATE		(MASTER_CLOCK_RATE / 12)
#define APU_SEQ_CLOCK_RATE	(MASTER_CLOCK_RATE / 89490)

/* Interrupt definitions */
#define NMI_N			0
#define IRQ_N			1

/* Bus definitions */
#define CPU_BUS_ID		0
#define PPU_BUS_ID		1

/* Memory sizes */
#define WRAM_SIZE		KB(2)
#define VRAM_SIZE		KB(4)

/* CPU memory map */
#define WRAM_START		0x0000
#define WRAM_END		0x07FF
#define WRAM_MIRROR_START	0x0800
#define WRAM_MIRROR_END		0x1FFF
#define PPU_START		0x2000
#define PPU_END			0x2007
#define PPU_MIRROR_START	0x2008
#define PPU_MIRROR_END		0x3FFF
#define APU_START		0x4000
#define APU_END			0x4013
#define SPRITE_DMA_START	0x4014
#define SPRITE_DMA_END		0x4014
#define APU_CTRL_STAT		0x4015
#define CTRL_START		0x4016
#define CTRL_END		0x4017
#define APU_FRAME_COUNTER	0x4017
#define EXPANSION_START		0x4018
#define EXPANSION_END		0x5FFF
#define SRAM_START		0x6000
#define SRAM_END		0x7FFF
#define PRG_ROM_START		0x8000
#define PRG_ROM_END		0xFFFF

/* PPU memory map */
#define CHR_START		0x0000
#define CHR_END			0x1FFF
#define VRAM_START		0x2000
#define VRAM_END		0x2FFF
#define VRAM_MIRROR_START	0x3000
#define VRAM_MIRROR_END		0x3EFF
#define PALETTE_START		0x3F00
#define PALETTE_END		0x3F1F
#define PALETTE_MIRROR_START	0x3F20
#define PALETTE_MIRROR_END	0x3FFF

struct nes_data {
	uint8_t wram[WRAM_SIZE];
	uint8_t vram[VRAM_SIZE];
	struct region wram_region;
};

static bool nes_init();
static void nes_deinit();

/* WRAM area */
static struct resource wram_mirror =
	MEM("mem_mirror", CPU_BUS_ID, WRAM_MIRROR_START, WRAM_MIRROR_END);

static struct resource wram_area =
	MEMX("mem", CPU_BUS_ID, WRAM_START, WRAM_END, &wram_mirror, 1);

/* RP2A03 CPU */
static struct resource rp2a03_resources[] = {
	IRQ("nmi", NMI_N),
	IRQ("irq", IRQ_N),
	CLK("clk", CPU_CLOCK_RATE)
};

static struct cpu_instance rp2a03_instance = {
	.cpu_name = "rp2a03",
	.bus_id = CPU_BUS_ID,
	.resources = rp2a03_resources,
	.num_resources = ARRAY_SIZE(rp2a03_resources)
};

/* APU controller */
static struct resource apu_resources[] = {
	MEM("main", CPU_BUS_ID, APU_START, APU_END),
	MEM("ctrl_stat", CPU_BUS_ID, APU_CTRL_STAT, APU_CTRL_STAT),
	MEM("seq", CPU_BUS_ID, APU_FRAME_COUNTER, APU_FRAME_COUNTER),
	CLK("clk", APU_CLOCK_RATE),
	CLK("seq_clk", APU_SEQ_CLOCK_RATE),
	IRQ("irq", IRQ_N)
};

static struct controller_instance apu_instance = {
	.controller_name = "apu",
	.bus_id = CPU_BUS_ID,
	.resources = apu_resources,
	.num_resources = ARRAY_SIZE(apu_resources)
};

/* Sprite DMA controller */
static struct resource sprite_dma_resource =
	MEM("mem", CPU_BUS_ID, SPRITE_DMA_START, SPRITE_DMA_END);

static struct controller_instance sprite_dma_instance = {
	.controller_name = "nes_sprite",
	.bus_id = CPU_BUS_ID,
	.resources = &sprite_dma_resource,
	.num_resources = 1
};

/* NES standard controller */
static struct resource nes_controller_resource =
	MEM("mem", CPU_BUS_ID, CTRL_START, CTRL_END);

static struct controller_instance nes_controller_instance = {
	.controller_name = "nes_controller",
	.resources = &nes_controller_resource,
	.num_resources = 1
};

/* NES mapper controller */
static struct resource vram_mirror =
	MEM("vram", PPU_BUS_ID, VRAM_MIRROR_START, VRAM_MIRROR_END);

static struct resource nes_mapper_resources[] = {
	MEM("expansion", CPU_BUS_ID, EXPANSION_START, EXPANSION_END),
	MEM("sram", CPU_BUS_ID, SRAM_START, SRAM_END),
	MEM("prg_rom", CPU_BUS_ID, PRG_ROM_START, PRG_ROM_END),
	MEM("chr", PPU_BUS_ID, CHR_START, CHR_END),
	MEMX("vram", PPU_BUS_ID, VRAM_START, VRAM_END, &vram_mirror, 1),
	IRQ("irq", IRQ_N)
};

static struct controller_instance nes_mapper_instance = {
	.controller_name = "nes_mapper",
	.resources = nes_mapper_resources,
	.num_resources = ARRAY_SIZE(nes_mapper_resources)
};

/* PPU controller */
static struct resource ppu_mirror =
	MEM("mem_mirror", CPU_BUS_ID, PPU_MIRROR_START, PPU_MIRROR_END);

static struct resource palette_mirror =
	MEM("pal_mirror", PPU_BUS_ID, PALETTE_MIRROR_START, PALETTE_MIRROR_END);

static struct resource ppu_resources[] = {
	MEMX("mem", CPU_BUS_ID, PPU_START, PPU_END, &ppu_mirror, 1),
	MEMX("pal", PPU_BUS_ID, PALETTE_START, PALETTE_END, &palette_mirror, 1),
	IRQ("irq", NMI_N),
	CLK("clk", PPU_CLOCK_RATE)
};

static struct controller_instance ppu_instance = {
	.controller_name = "ppu",
	.bus_id = PPU_BUS_ID,
	.resources = ppu_resources,
	.num_resources = ARRAY_SIZE(ppu_resources)
};

bool nes_init(struct machine *machine)
{
	struct nes_data *nes_data;

	/* Create machine data structure */
	nes_data = calloc(1, sizeof(struct nes_data));

	/* Add WRAM region */
	nes_data->wram_region.area = &wram_area;
	nes_data->wram_region.mops = &ram_mops;
	nes_data->wram_region.data = nes_data->wram;
	memory_region_add(&nes_data->wram_region);

	g_ram_data = nes_data->wram;
	g_ram_size = WRAM_SIZE;

	/* NES cart controls VRAM address lines so let the mapper handle it */
	nes_mapper_instance.mach_data = nes_data->vram;

	/* Add controllers and CPU */
	if (!controller_add(&apu_instance) ||
		!controller_add(&sprite_dma_instance) ||
		!controller_add(&nes_mapper_instance) ||
		!controller_add(&ppu_instance) ||
		!controller_add(&nes_controller_instance) ||
		!cpu_add(&rp2a03_instance)) {
		free(nes_data);
		return false;
	}

	/* Save machine data structure */
	machine->priv_data = nes_data;

	return true;
}

void nes_deinit(struct machine *machine)
{
	free(machine->priv_data);
}

MACHINE_START(nes, "Nintendo Entertainment System")
	.init = nes_init,
	.deinit = nes_deinit
MACHINE_END

