
// short file Matt made to switch between z80's easily
// comment this out to use the old z80 core
// UPDATE : I don't think the old z80 core works anymore, it hasn't been maintained and some new functions (get elapsed cycles) aren't available
#define USE_M80	1

#include "../daphne.h"
#ifdef USE_M80
#include "../cpu/m80.h"
#define Z80_GET_AF	m80_get_reg(M80_AF)
#define Z80_GET_HL	m80_get_reg(M80_HL)
#define Z80_GET_DE	m80_get_reg(M80_DE)
#define Z80_GET_BC	m80_get_reg(M80_BC)
#define Z80_GET_PC	m80_get_pc()
#define Z80_GET_SP	m80_get_sp()
#define Z80_GET_IX	m80_get_reg(M80_IX)
#define Z80_GET_IY	m80_get_reg(M80_IY)
#define Z80_GET_IFF1	0
#define Z80_SET_PC(val)	m80_set_pc(val)
#define Z80_SET_SP(val)	m80_set_sp(val)
#define Z80_SET_AF(val) m80_set_reg(M80_AF, val)
#define Z80_EXECUTE(val)	m80_execute(val)
#define Z80_ASSERT_IRQ	m80_set_irq_line(ASSERT_LINE)
#define Z80_CLEAR_IRQ	m80_set_irq_line(CLEAR_LINE)
#define Z80_ASSERT_NMI	m80_set_nmi_line(CLEAR_LINE); m80_set_nmi_line(ASSERT_LINE);
#define Z80_GET_ELAPSED_CYCLES	m80_get_cycles_executed()
#define Z80_EXIT

#endif
#ifdef USE_Z80
#include "../cpu/z80.h"
#define Z80_GET_AF	z80_get_reg(Z80_AF)
#define Z80_GET_HL	z80_get_reg(Z80_HL)
#define Z80_GET_DE	z80_get_reg(Z80_DE)
#define Z80_GET_BC	z80_get_reg(Z80_BC)
#define Z80_GET_PC	z80_get_pc()
#define Z80_GET_SP	z80_get_sp()
#define Z80_GET_IX	z80_get_reg(Z80_IX)
#define Z80_GET_IY	z80_get_reg(Z80_IY)
#define Z80_GET_IFF1	z80_get_reg(Z80_IFF1)
#define Z80_SET_PC(val)	z80_set_pc(val)
#define Z80_SET_SP(val)	z80_set_sp(val)
#define Z80_SET_AF(val) z80_set_reg(Z80_AF, val)
#define Z80_EXECUTE(val)	z80_execute(val)
#define Z80_ASSERT_IRQ	z80_set_irq_line(0, HOLD_LINE)
#define Z80_CLEAR_IRQ	z80_set_irq_line(0, CLEAR_LINE)
#define Z80_ASSERT_NMI	z80_set_nmi_line(ASSERT_LINE); z80_execute(1); z80_set_nmi_line(CLEAR_LINE)
#define Z80_GET_ELAPSED_CYCLES	printf("ERROR : Unsupported in Z80 core, you'll need to add this because DAPHNE now relies on it!\n");
#define Z80_EXIT		z80_exit();

#endif

