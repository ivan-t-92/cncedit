#include "variables.h"
#include "util.h"

#include <algorithm>
#include <new>

Variables::DefineResult Variables::define(const std::string& name, ValueType type) noexcept
{
    return define(name, createDefaultValue(type));
}

Variables::DefineResult Variables::define(const std::string& name, Value initValue) noexcept
{
    if (isDefined(name))
    {
        return DefineResult::AlreadyExists;
    }

    try
    {
        values[to_upper_copy(name)] = initValue;
    }
    catch (std::bad_alloc&)
    {
        return DefineResult::OutOfMemory;
    }
    catch (...)
    {
        return DefineResult::UnknownError;
    }

    return DefineResult::Success;
}

Variables::DefineResult Variables::defineArray(const std::string& name,
                                               ValueType type,
                                               const std::vector<int>& arrayDimensions) noexcept
{
    const int maxArraySize {32767};
    if (isDefined(name))
        return DefineResult::AlreadyExists;

    auto it = std::find_if(arrayDimensions.begin(),
                           arrayDimensions.end(),
                           [=](int i) {return i <= 0 || i > maxArraySize;});
    if (it != arrayDimensions.end())
        return DefineResult::InvalidArraySize;

    try
    {
        switch (arrayDimensions.size())
        {
        case 1:
            valueArrays1[to_upper_copy(name)] = std::vector<Value> {
                    static_cast<std::size_t>(arrayDimensions[0]), createDefaultValue(type)};
            break;
        case 2:
            valueArrays2[to_upper_copy(name)] = {
                std::vector<Value> {
                    static_cast<std::size_t>(arrayDimensions[0]*arrayDimensions[1]),
                    createDefaultValue(type)},
                static_cast<std::size_t>(arrayDimensions[0])};
            break;
        case 3:
            valueArrays3[to_upper_copy(name)] = {
                std::vector<Value> {
                    static_cast<std::size_t>(arrayDimensions[0]*arrayDimensions[1]*arrayDimensions[2]),
                    createDefaultValue(type)},
                static_cast<std::size_t>(arrayDimensions[0]),
                static_cast<std::size_t>(arrayDimensions[1])};
            break;
        default:
            return DefineResult::InvalidDimensionCount;
        }
    }
    catch (std::bad_alloc&)
    {
        return DefineResult::OutOfMemory;
    }
    catch (...)
    {
        return DefineResult::UnknownError;
    }
    return DefineResult::Success;
}

bool Variables::isDefined(const std::string& name) const noexcept
{
    return (isVariable(name) ||
            isArray1(name) ||
            isArray2(name) ||
            isArray3(name));
}

int Variables::dimensionCount(const std::string& name) const noexcept
{
    if (isVariable(name))
        return 0;

    if (isArray1(name))
        return 1;

    if (isArray2(name))
        return 2;

    if (isArray3(name))
        return 3;

    return -1;
}

Variables::AccessResult Variables::setValue(const std::string& name, const Value& value) noexcept
{
    auto it = values.find(to_upper_copy(name));
    if (it == values.end())
        return AccessResult::DoNotExists;

    if (getValueType(it->second) != getValueType(value))
        return AccessResult::TypeMismatch;

    it->second = value;
    return AccessResult::Success;
}

//template <typename T>
static bool validIndex(int index, const std::vector<Value>& vector)
{
    return index >= 0 && static_cast<size_t>(index) < vector.size();
}

Variables::AccessResult Variables::setArray1Value(const std::string& name, int index, const Value& value) noexcept
{
    auto it = valueArrays1.find(to_upper_copy(name));
    if (it == valueArrays1.end())
        return AccessResult::DoNotExists;

    if (!validIndex(index, it->second))
        return AccessResult::ArrayIndexOutOfBounds;

    if (getValueType(it->second[index]) != getValueType(value))
        return AccessResult::TypeMismatch;

    it->second[index] = value;
    return AccessResult::Success;
}

Variables::AccessResult Variables::setArray2Value(const std::string& name, int index1, int index2, const Value& value) noexcept
{
    auto it = valueArrays2.find(to_upper_copy(name));
    if (it == valueArrays2.end())
        return AccessResult::DoNotExists;

    if (index1 < 0 || index2 < 0)
        return AccessResult::ArrayIndexOutOfBounds;

    auto& array2 = it->second;
    auto index = array2.index(index1, index2);
    if (!validIndex(index, array2.arr))
        return AccessResult::ArrayIndexOutOfBounds;

    if (getValueType(array2.arr[index]) != getValueType(value))
        return AccessResult::TypeMismatch;

    array2.arr[index] = value;
    return AccessResult::Success;
}

Variables::AccessResult Variables::setArray3Value(const std::string& name, int index1, int index2, int index3, const Value& value) noexcept
{
    auto it = valueArrays3.find(to_upper_copy(name));
    if (it == valueArrays3.end())
        return AccessResult::DoNotExists;

    if (index1 < 0 || index2 < 0 || index3 < 0)
        return AccessResult::ArrayIndexOutOfBounds;

    auto& array3 = it->second;
    auto index = array3.index(index1, index2, index3);
    if (!validIndex(index, array3.arr))
        return AccessResult::ArrayIndexOutOfBounds;

    if (getValueType(array3.arr[index]) != getValueType(value))
        return AccessResult::TypeMismatch;

    array3.arr[index] = value;
    return AccessResult::Success;
}

std::pair<Value, Variables::AccessResult> Variables::getValue(const std::string& name) const noexcept
{
    auto it = values.find(to_upper_copy(name));
    if (it == values.end())
        return {Value{}, AccessResult::DoNotExists};

    return {it->second, AccessResult::Success};
}

std::pair<Value, Variables::AccessResult> Variables::getArray1Value(const std::string& name, int index) const noexcept
{
    if (isArray2(name) ||
        isArray3(name))
        return {Value{}, AccessResult::DimensionMismatch};

    auto it = valueArrays1.find(to_upper_copy(name));
    if (it == valueArrays1.end())
        return {Value{}, AccessResult::DoNotExists};

    if (!validIndex(index, it->second))
        return {Value{}, AccessResult::ArrayIndexOutOfBounds};

    return {it->second[index], AccessResult::Success};
}

std::pair<Value, Variables::AccessResult> Variables::getArray2Value(const std::string& name, int index1, int index2) const noexcept
{
    if (isArray1(name) ||
        isArray3(name))
        return {Value{}, AccessResult::DimensionMismatch};

    auto it = valueArrays2.find(to_upper_copy(name));
    if (it == valueArrays2.end())
        return {Value{}, AccessResult::DoNotExists};

    if (index1 < 0 || index2 < 0)
        return {Value{}, AccessResult::ArrayIndexOutOfBounds};

    auto& array2 = it->second;
    auto index = array2.index(index1, index2);
    if (!validIndex(index, array2.arr))
        return {Value{}, AccessResult::ArrayIndexOutOfBounds};

    return {array2.arr[index], AccessResult::Success};
}

std::pair<Value, Variables::AccessResult> Variables::getArray3Value(const std::string& name, int index1, int index2, int index3) const noexcept
{
    if (isArray1(name) ||
        isArray2(name))
        return {Value{}, AccessResult::DimensionMismatch};

    auto it = valueArrays3.find(to_upper_copy(name));
    if (it == valueArrays3.end())
        return {Value{}, AccessResult::DoNotExists};

    if (index1 < 0 || index2 < 0 || index3 < 0)
        return {Value{}, AccessResult::ArrayIndexOutOfBounds};

    auto& array3 = it->second;
    auto index = array3.index(index1, index2, index3);
    if (!validIndex(index, array3.arr))
        return {Value{}, AccessResult::ArrayIndexOutOfBounds};

    return {array3.arr[index], AccessResult::Success};
}

void Variables::clear() noexcept
{
    values.clear();
    valueArrays1.clear();
    valueArrays2.clear();
    valueArrays3.clear();
}

bool Variables::isVariable(const std::string& name) const
{
    return values.find(to_upper_copy(name)) != values.end();
}

bool Variables::isArray1(const std::string& name) const
{
    return valueArrays1.find(to_upper_copy(name)) != valueArrays1.end();
}

bool Variables::isArray2(const std::string& name) const
{
    return valueArrays2.find(to_upper_copy(name)) != valueArrays2.end();
}

bool Variables::isArray3(const std::string& name) const
{
    return valueArrays3.find(to_upper_copy(name)) != valueArrays3.end();
}
