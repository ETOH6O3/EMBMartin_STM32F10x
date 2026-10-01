
# include "stream.h"


using namespace EMBMartin;
namespace{
    class __NO_USE_OUTSTREAM : public OutStream<128>
    {
    public:
        using OutStream<128>::OutStream;
    protected:
        virtual void output_char(char c) noexcept override{};
    };
    __NO_USE_OUTSTREAM __default_no_console{};
}

__attribute__((weak)) OutStream<128> &console = __default_no_console;