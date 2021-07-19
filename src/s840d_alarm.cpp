#include "s840d_alarm.h"

S840D_Alarm::S840D_Alarm(int code) noexcept
    : m_code(code)
{}

int S840D_Alarm::getAlarmCode() const noexcept
{
    return m_code;
}

const char* S840D_Alarm::what() const noexcept
{
    return "Alarm";
}
