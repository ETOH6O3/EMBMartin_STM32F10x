#pragma once

#include <cstdint>
#include "EMBMartin/macro.h"


EMBMARTIN_SYS_NAMESPACE_BEGIN

/**
 * @brief 中断保护类
 * @details 用于在作用域内临时关闭中断，防止中断干扰。
 */
class IRQGuard {
public:
    IRQGuard() noexcept {
        // 保存当前 PRIMASK 状态 (记录中断是否本来就是关闭的)
        saved_primask_ = __get_PRIMASK();
        __disable_irq();  // 关中断
        ++nesting_count_; // 嵌套计数
    }

    ~IRQGuard() {
        if (--nesting_count_ == 0) {
            // 仅当最外层退出时，才恢复之前的中断状态
            __set_PRIMASK(saved_primask_);
        }
    }

    // 禁止拷贝，防止意外复制导致状态错误
    IRQGuard(const IRQGuard&) = delete;
    IRQGuard& operator=(const IRQGuard&) = delete;

private:
    static inline uint32_t nesting_count_ = 0; // 嵌套深度
    uint32_t saved_primask_ = 0;
};
EMBMARTIN_SYS_NAMESPACE_END