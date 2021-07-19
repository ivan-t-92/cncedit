#ifndef S840D_ALARM_H
#define S840D_ALARM_H

#include <stdexcept>

class S840D_Alarm : public std::exception
{
    const int m_code;
public:
    S840D_Alarm(int code) noexcept;
    int getAlarmCode() const noexcept;
    const char* what() const noexcept override;
};

#endif // S840D_ALARM_H
