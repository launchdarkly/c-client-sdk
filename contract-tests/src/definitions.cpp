#include "definitions/definitions.hpp"

namespace ld {

EvaluateFlagParams::EvaluateFlagParams() :
    valueType{ValueType::Unspecified},
    detail{false}
{}

CommandParams::CommandParams() :
    command{Command::Unknown}
{}

} // namespace ld
