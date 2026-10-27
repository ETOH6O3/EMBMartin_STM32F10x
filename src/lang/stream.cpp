
#include "EMBMartin/lang/stream.h"


using namespace EMBMartin::lang;
namespace{
    class __NO_USE_OUTSTREAM : public EMBMartin::lang::OutStream<128>
    {
    public:
        using EMBMartin::lang::OutStream<128>::OutStream;
    protected:
        virtual void output_char(char c) noexcept override{};
    };
    __NO_USE_OUTSTREAM __default_no_console{};
}

__attribute__((weak)) EMBMartin::lang::OutStream<128> &console = __default_no_console;