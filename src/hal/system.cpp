
#include "EMBMartin/hal/system.h"

extern uint32_t SystemCoreClock;

constexpr inline uint16_t __TIM_Prescaler_plus1 = 1000; // 配合 TIM_GetCounter 可获得小数点后三位精度

EMBMARTIN_SYS_NAMESPACE_BEGIN

InnerTimerManager::InnerTimerManager(
    TIM_TypeDef *__TIMX, double time,
    bool open,
    uint8_t PreemptionPriority, uint8_t SubPriority,
    std::initializer_list<Callback> init) noexcept
    : TIMX{__TIMX}
{
    if (__TIMX != TIM1)
        RCC_APB1PeriphClockCmd(get_TIM_RCC_APB1Periph(__TIMX), ENABLE);
    else
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_InternalClockConfig(__TIMX);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {
        .TIM_Prescaler = __TIM_Prescaler_plus1 - 1,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = uint16_t(time * SystemCoreClock / __TIM_Prescaler_plus1 - 1),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0};
    TIM_TimeBaseInit(__TIMX, &TIM_TimeBaseInitStructure);
    TIM_ClearFlag(__TIMX, TIM_FLAG_Update); // 清除 TIM_TimeBaseInit 中生成的更新中断标志

    // 记录实际装载的周期（取整后的真实值，供 Delayer 等换算节拍）
    __periods[uint8_t(Get_TIM_Index(__TIMX))] =
        double(__TIM_Prescaler_plus1) * (double(TIM_TimeBaseInitStructure.TIM_Period) + 1.0) / double(SystemCoreClock);
    __opened[uint8_t(Get_TIM_Index(__TIMX))] = false; // 时基刚刚重置，此刻一定还没开

    TIM_ITConfig(__TIMX, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = uint8_t((__TIMX == TIM1) ? TIM1_UP_IRQn : (__TIMX == TIM2) ? TIM2_IRQn
                                                                 : (__TIMX == TIM3)   ? TIM3_IRQn
                                                                 : (__TIMX == TIM4)   ? TIM4_IRQn
                                                                                      : (IRQn)0xFF),
        .NVIC_IRQChannelPreemptionPriority = PreemptionPriority,
        .NVIC_IRQChannelSubPriority = SubPriority,
        .NVIC_IRQChannelCmd = ENABLE};
    NVIC_Init(&NVIC_InitStructure);

    // 先登记初始队列，再开启定时器，避免第一个中断打空队列
    for (const auto &__callback : init)
    {
        if (__handle_count == MAX_CALLBACKS)
            break;
        const auto __handle = __add(uint8_t(Get_TIM_Index(__TIMX)), __callback);
        if (__handle.valid())
            __handles[__handle_count++] = __handle;
    }

    if (open)
    {
        TIM_Cmd(__TIMX, ENABLE);
        __opened[uint8_t(Get_TIM_Index(__TIMX))] = true;
    }
}

InnerTimerManager::~InnerTimerManager() noexcept
{
    for (uint8_t __i = 0; __i < __handle_count; ++__i)
        remove(__handles[__i]);
}

InnerTimerManager::Handle InnerTimerManager::__add(uint8_t __idx, const Callback &__callback) noexcept
{
    if (__idx >= __queues.size() || !__callback)
        return {};

    // 关中断，避免与中断服务函数的遍历交错
    IRQGuard __guard;

    for (uint8_t __i = 0; __i < MAX_CALLBACKS; ++__i)
    {
        if (!__queues[__idx][__i].used)
        {
            construct(__queues[__idx][__i], __callback);
            return {__idx, __i};
        }
    }

    return {}; // 队列已满
}

void InnerTimerManager::remove(const Handle &__handle) noexcept
{
    if (!__handle.valid() || __handle.timer >= __queues.size() || __handle.slot >= MAX_CALLBACKS)
        return;

    IRQGuard __guard; // 先关中断再销毁回调对象
    destroy(__queues[__handle.timer][__handle.slot]);
}

double InnerTimerManager::period(TIM_TypeDef *TIMX) noexcept
{
    const uint8_t __idx = uint8_t(Get_TIM_Index(TIMX));
    return __idx < __periods.size() ? __periods[__idx] : 0.0;
}

bool InnerTimerManager::is_open(TIM_TypeDef *TIMX) noexcept
{
    const uint8_t __idx = uint8_t(Get_TIM_Index(TIMX));
    return __idx < __opened.size() && __opened[__idx];
}

size_t InnerTimerManager::count(TIM_TypeDef *TIMX) noexcept
{
    const uint8_t __idx = uint8_t(Get_TIM_Index(TIMX));
    if (__idx >= __queues.size())
        return 0;

    size_t __total = 0;
    for (const auto &__slot : __queues[__idx])
    {
        if (__slot.used)
            ++__total;
    }
    return __total;
}

EMBMARTIN_SYS_NAMESPACE_END

// 定时器更新中断：由本库统一实现，直接调用对应队列
extern "C"
{
    void TIM1_UP_IRQHandler(void)
    {
        if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
        {
            EMBMartin::STM32::sys::InnerTimerManager::invoke(0);
            TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
        }
    }

    void TIM2_IRQHandler(void)
    {
        if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
        {
            EMBMartin::STM32::sys::InnerTimerManager::invoke(1);
            TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        }
    }

    void TIM3_IRQHandler(void)
    {
        if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
        {
            EMBMartin::STM32::sys::InnerTimerManager::invoke(2);
            TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        }
    }

    void TIM4_IRQHandler(void)
    {
        if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
        {
            EMBMartin::STM32::sys::InnerTimerManager::invoke(3);
            TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        }
    }
}
