#ifndef CEC_TIMER_H
#define CEC_TIMER_H

//Manual Page.129
#define TIMERBASE0                  0x40064000
#define TIMERBASE1                  0x40064800

#define TIMERBASE_EN                0x00
#define TIMERBASE_LOW_EN_SHIFT      0U
#define TIMERBASE_HIGH_EN_SHIFT     1U

#define TIMERBASE_DIV               0x04
//0..15 (uint16 size)

#define TIMERBASE_IE                0x10
#define TIMERBASE_LOW_IE_SHIFT      0U
#define TIMERBASE_HIGH_IE_SHIFT     1U

//Interupt Status, if your want clear statue, set 1 to this field
#define TIMERBASE_IF                0x14
#define TIMERBASE_LOW_IF_SHIFT      0U
#define TIMERBASE_HIGH_IF_SHIFT     1U

#define TIMERBASE_HIGHLOAD          0x20
//0..15  (Limit High Count value )
#define TIMERBASE_HIGHCNT           0x24
//0..15 Current HighCount
#define TIMERBASE_LOWLOAD           0x30
//0..15
#define TIMERBASE_LOWCNT            0x34
//0..15
#define TIMERBASE0_EN_ADDR          (TIMERBASE0 + TIMERBASE_EN)
#define TIMERBASE0_EN               (*(volatile uint32_t *)TIMERBASE0_EN_ADDR)
#define TIMERBASE0_DIV_ADDR         (TIMERBASE0 + TIMERBASE_DIV)
#define TIMERBASE0_DIV              (*(volatile uint32_t *)TIMERBASE0_DIV_ADDR)
#define TIMERBASE0_IE_ADDR          (TIMERBASE0 + TIMERBASE_IE)
#define TIMERBASE0_IE               (*(volatile uint32_t *)TIMERBASE0_IE_ADDR)
#define TIMERBASE0_IF_ADDR          (TIMERBASE0 + TIMERBASE_IF)
#define TIMERBASE0_IF               (*(volatile uint32_t *)TIMERBASE0_IF_ADDR)
#define TIMERBASE0_HIGHLOAD_ADDR    (TIMERBASE0 + TIMERBASE_HIGHLOAD)
#define TIMERBASE0_HIGHLOAD         (*(volatile uint32_t *)TIMERBASE0_HIGHLOAD_ADDR)
#define TIMERBASE0_HIGHCNT_ADDR     (TIMERBASE0 + TIMERBASE_HIGHCNT)
#define TIMERBASE0_HIGHCNT          (*(volatile uint32_t *)TIMERBASE0_HIGHCNT_ADDR)
#define TIMERBASE0_LOWLOAD_ADDR     (TIMERBASE0 + TIMERBASE_LOWLOAD)
#define TIMERBASE0_LOWLOAD          (*(volatile uint32_t *)TIMERBASE0_LOWLOAD_ADDR)
#define TIMERBASE0_LOWCNT_ADDR      (TIMERBASE0 + TIMERBASE_LOWCNT)
#define TIMERBASE0_LOWCNT           (*(volatile uint32_t *)TIMERBASE0_LOWCNT_ADDR)



#define TIMERBASE1_EN_ADDR          (TIMERBASE1 + TIMERBASE_EN)
#define TIMERBASE1_EN               (*(volatile uint32_t *)TIMERBASE1_EN_ADDR)
#define TIMERBASE1_DIV_ADDR         (TIMERBASE1 + TIMERBASE_DIV)
#define TIMERBASE1_DIV              (*(volatile uint32_t *)TIMERBASE1_DIV_ADDR)
#define TIMERBASE1_IE_ADDR          (TIMERBASE1 + TIMERBASE_IE)
#define TIMERBASE1_IE               (*(volatile uint32_t *)TIMERBASE1_IE_ADDR)
#define TIMERBASE1_IF_ADDR          (TIMERBASE1 + TIMERBASE_IF)
#define TIMERBASE1_IF               (*(volatile uint32_t *)TIMERBASE1_IF_ADDR)
#define TIMERBASE1_HIGHLOAD_ADDR    (TIMERBASE1 + TIMERBASE_HIGHLOAD)
#define TIMERBASE1_HIGHLOAD         (*(volatile uint32_t *)TIMERBASE1_HIGHLOAD_ADDR)
#define TIMERBASE1_HIGHCNT_ADDR     (TIMERBASE1 + TIMERBASE_HIGHCNT)
#define TIMERBASE1_HIGHCNT          (*(volatile uint32_t *)TIMERBASE1_HIGHCNT_ADDR)
#define TIMERBASE1_LOWLOAD_ADDR     (TIMERBASE1 + TIMERBASE_LOWLOAD)
#define TIMERBASE1_LOWLOAD          (*(volatile uint32_t *)TIMERBASE1_LOWLOAD_ADDR)
#define TIMERBASE1_LOWCNT_ADDR      (TIMERBASE1 + TIMERBASE_LOWCNT)
#define TIMERBASE1_LOWCNT           (*(volatile uint32_t *)TIMERBASE1_LOWCNT_ADDR)


//MANUAL PAGE.159
#define TIMERPLUS0_BASE         0x40067000
#define TIMERPLUS1_BASE         0x40067800

#define TIMERPLUS_EN            0x00
#define TIMERPLUS_DIV           0x04
#define TIMERPLUS_CTR           0x08
#define TIMERPLUS_IE            0x10
#define TIMERPLUS_IF            0x14
#define TIMERPLUS_HIGH_GOAL     0x20
#define TIMERPLUS_HIGH_CNT      0x24
#define TIMERPLUS_HIGH_CVAL     0x28
#define TIMERPLUS_LOW_GOAL      0x30
#define TIMERPLUS_LOW_CNT       0x34
#define TIMERPLUS_LOW_CVAL      0x38
#define TIMERPLUS_HALL_VAL      0x30





#define TIMERPLUS0_EN_ADDR           (TIMERPLUS0_BASE + TIMERPLUS_EN)
#define TIMERPLUS0_DIV_ADDR          (TIMERPLUS0_BASE + TIMERPLUS_DIV)
#define TIMERPLUS0_CTR_ADDR          (TIMERPLUS0_BASE + TIMERPLUS_CTR)
#define TIMERPLUS0_IE_ADDR           (TIMERPLUS0_BASE + TIMERPLUS_IE)
#define TIMERPLUS0_IF_ADDR           (TIMERPLUS0_BASE + TIMERPLUS_IF)
#define TIMERPLUS0_HIGH_GOAL_ADDR    (TIMERPLUS0_BASE + TIMERPLUS_HIGH_GOAL)
#define TIMERPLUS0_HIGH_CNT_ADDR     (TIMERPLUS0_BASE + TIMERPLUS_HIGH_CNT)
#define TIMERPLUS0_HIGH_CVAL_ADDR    (TIMERPLUS0_BASE + TIMERPLUS_HIGH_CVAL)
#define TIMERPLUS0_LOW_GOAL_ADDR     (TIMERPLUS0_BASE + TIMERPLUS_LOW_GOAL)
#define TIMERPLUS0_LOW_CNT_ADDR      (TIMERPLUS0_BASE + TIMERPLUS_LOW_CNT)
#define TIMERPLUS0_LOW_CVAL_ADDR     (TIMERPLUS0_BASE + TIMERPLUS_LOW_CVAL)
#define TIMERPLUS0_HALL_VAL_ADDR     (TIMERPLUS0_BASE + TIMERPLUS_HALL_VAL)

#define TIMERPLUS0_EN               (*(volatile uint32_t *)TIMERPLUS0_EN_ADDR)
#define TIMERPLUS0_DIV              (*(volatile uint32_t *)TIMERPLUS0_DIV_ADDR)
#define TIMERPLUS0_CTR              (*(volatile uint32_t *)TIMERPLUS0_CTR_ADDR)
#define TIMERPLUS0_IE               (*(volatile uint32_t *)TIMERPLUS0_IE_ADDR)
#define TIMERPLUS0_IF               (*(volatile uint32_t *)TIMERPLUS0_IF_ADDR)
#define TIMERPLUS0_HIGH_GOAL        (*(volatile uint32_t *)TIMERPLUS0_HIGH_GOAL_ADDR)
#define TIMERPLUS0_HIGH_CNT         (*(volatile uint32_t *)TIMERPLUS0_HIGH_CNT_ADDR)
#define TIMERPLUS0_HIGH_CVAL        (*(volatile uint32_t *)TIMERPLUS0_HIGH_CVAL_ADDR)
#define TIMERPLUS0_LOW_GOAL         (*(volatile uint32_t *)TIMERPLUS0_LOW_GOAL_ADDR)
#define TIMERPLUS0_LOW_CNT          (*(volatile uint32_t *)TIMERPLUS0_LOW_CNT_ADDR)
#define TIMERPLUS0_LOW_CVAL         (*(volatile uint32_t *)TIMERPLUS0_LOW_CVAL_ADDR)
#define TIMERPLUS0_HALL_VAL         (*(volatile uint32_t *)TIMERPLUS0_HALL_VAL_ADDR)




#define TIMERPLUS1_EN_ADDR          (TIMERPLUS1_BASE + TIMERPLUS_EN)
#define TIMERPLUS1_DIV_ADDR          (TIMERPLUS1_BASE + TIMERPLUS_DIV)
#define TIMERPLUS1_CTR_ADDR          (TIMERPLUS1_BASE + TIMERPLUS_CTR)
#define TIMERPLUS1_IE_ADDR           (TIMERPLUS1_BASE + TIMERPLUS_IE)
#define TIMERPLUS1_IF_ADDR           (TIMERPLUS1_BASE + TIMERPLUS_IF)
#define TIMERPLUS1_HIGH_GOAL_ADDR    (TIMERPLUS1_BASE + TIMERPLUS_HIGH_GOAL)
#define TIMERPLUS1_HIGH_CNT_ADDR     (TIMERPLUS1_BASE + TIMERPLUS_HIGH_CNT)
#define TIMERPLUS1_HIGH_CVAL_ADDR    (TIMERPLUS1_BASE + TIMERPLUS_HIGH_CVAL)
#define TIMERPLUS1_LOW_GOAL_ADDR     (TIMERPLUS1_BASE + TIMERPLUS_LOW_GOAL)
#define TIMERPLUS1_LOW_CNT_ADDR      (TIMERPLUS1_BASE + TIMERPLUS_LOW_CNT)
#define TIMERPLUS1_LOW_CVAL_ADDR     (TIMERPLUS1_BASE + TIMERPLUS_LOW_CVAL)
#define TIMERPLUS1_HALL_VAL_ADDR     (TIMERPLUS1_BASE + TIMERPLUS_HALL_VAL)

#define TIMERPLUS1_EN               (*(volatile uint32_t *)TIMERPLUS1_EN_ADDR)
#define TIMERPLUS1_DIV              (*(volatile uint32_t *)TIMERPLUS1_DIV_ADDR)
#define TIMERPLUS1_CTR              (*(volatile uint32_t *)TIMERPLUS1_CTR_ADDR)
#define TIMERPLUS1_IE               (*(volatile uint32_t *)TIMERPLUS1_IE_ADDR)
#define TIMERPLUS1_IF               (*(volatile uint32_t *)TIMERPLUS1_IF_ADDR)
#define TIMERPLUS1_HIGH_GOAL        (*(volatile uint32_t *)TIMERPLUS1_HIGH_GOAL_ADDR)
#define TIMERPLUS1_HIGH_CNT         (*(volatile uint32_t *)TIMERPLUS1_HIGH_CNT_ADDR)
#define TIMERPLUS1_HIGH_CVAL        (*(volatile uint32_t *)TIMERPLUS1_HIGH_CVAL_ADDR)
#define TIMERPLUS1_LOW_GOAL         (*(volatile uint32_t *)TIMERPLUS1_LOW_GOAL_ADDR)
#define TIMERPLUS1_LOW_CNT          (*(volatile uint32_t *)TIMERPLUS1_LOW_CNT_ADDR)
#define TIMERPLUS1_LOW_CVAL         (*(volatile uint32_t *)TIMERPLUS1_LOW_CVAL_ADDR)
#define TIMERPLUS1_HALL_VAL         (*(volatile uint32_t *)TIMERPLUS1_HALL_VAL_ADDR)

#endif