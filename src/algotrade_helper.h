#include <functional>
#include <string>

namespace algotrade{

using std::runtime_error;

static constexpr int64_t ErrorCode_Unknown = -1;

class Error: public runtime_error {
    int32_t c;
public:
    explicit Error (std::string const &what): runtime_error(what), c(ErrorCode_Unknown) {}
    explicit Error (char const *what): runtime_error(what), c(ErrorCode_Unknown) {}
    explicit Error (std::string const &what, int32_t code): runtime_error(what), c(code) {}
    explicit Error (char const *what, int32_t code): runtime_error(what), c(code) {}
    virtual int32_t code () const {
        return c;
    }
};

}