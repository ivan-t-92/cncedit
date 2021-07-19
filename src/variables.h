#ifndef VARIABLES_H
#define VARIABLES_H

#include "value.h"

#include <unordered_map>
#include <vector>
#include <utility>

class Variables
{
public:
    enum class DefineResult
    {
        Success = 0,
        AlreadyExists,
        InvalidArraySize,
        InvalidDimensionCount,
        OutOfMemory,
        UnknownError
    };
    enum class AccessResult
    {
        Success = 0,
        DoNotExists,
        ArrayIndexOutOfBounds,
        InvalidDimensionCount,
        DimensionMismatch,
        TypeMismatch
    };

    DefineResult define(const std::string& name, ValueType type) noexcept;
    DefineResult define(const std::string& name, Value initValue) noexcept;
    DefineResult defineArray(const std::string& name, ValueType type, const std::vector<int>& arrayDimensions) noexcept; // TODO? convert vector to std::array<int, 1...3>
    bool isDefined(const std::string& name) const noexcept;
    int dimensionCount(const std::string& name) const noexcept;
    AccessResult setValue(const std::string& name, const Value& value) noexcept;
    template<typename IntIterator>
    AccessResult setArrayValue(const std::string& name, const Value& value, const IntIterator begin, const IntIterator end) noexcept
    {
        switch (end - begin)
        {
        case 1:
            if (isArray2(name) ||
                isArray3(name))
            {
                return AccessResult::DimensionMismatch;
            }
            return setArray1Value(name, *begin, value);
        case 2:
            if (isArray1(name) ||
                isArray3(name))
            {
                return AccessResult::DimensionMismatch;
            }
            return setArray2Value(name, *begin, *(begin+1), value);
        case 3:
            if (isArray1(name) ||
                isArray2(name))
            {
                return AccessResult::DimensionMismatch;
            }
            return setArray3Value(name, *begin, *(begin+1), *(begin+2), value);
        default:
            return AccessResult::InvalidDimensionCount;
        }
        return AccessResult::Success;
    }
    AccessResult setArray1Value(const std::string& name, int index, const Value& value) noexcept;
    AccessResult setArray2Value(const std::string& name, int index1, int index2, const Value& value) noexcept;
    AccessResult setArray3Value(const std::string& name, int index1, int index2, int index3, const Value& value) noexcept;
    std::pair<Value, AccessResult> getValue(const std::string& name) const noexcept;
    template<typename IntIterator>
    std::pair<Value, AccessResult> getArrayValue(const std::string& name, const IntIterator begin, const IntIterator end) const noexcept
    {
        switch (end - begin)
        {
        case 1:
            return getArray1Value(name, *begin);
        case 2:
            return getArray2Value(name, *begin, *(begin+1));
        case 3:
            return getArray3Value(name, *begin, *(begin+1), *(begin+2));
        default:
            return {Value{}, AccessResult::InvalidDimensionCount};
        }
    }
    std::pair<Value, AccessResult> getArray1Value(const std::string& name, int index) const noexcept;
    std::pair<Value, AccessResult> getArray2Value(const std::string& name, int index1, int index2) const noexcept;
    std::pair<Value, AccessResult> getArray3Value(const std::string& name, int index1, int index2, int index3) const noexcept;
    void clear() noexcept;
private:
    bool isVariable(const std::string& name) const;
    bool isArray1(const std::string& name) const;
    bool isArray2(const std::string& name) const;
    bool isArray3(const std::string& name) const;

    struct Array2
    {
        std::vector<Value> arr;
        std::size_t w;

        constexpr size_t index(int index1, int index2) const
        {
            return index1 * w + index2;
        }
    };

    struct Array3
    {
        std::vector<Value> arr;
        std::size_t w1;
        std::size_t w2;

        constexpr size_t index(int index1, int index2, int index3) const
        {
            return index1 * w1 * w2 + index2 * w2 + index3;
        }
    };

    std::unordered_map<std::string, Value> values;
    std::unordered_map<std::string, std::vector<Value>> valueArrays1;
    std::unordered_map<std::string, Array2> valueArrays2;
    std::unordered_map<std::string, Array3> valueArrays3;
};

#endif // VARIABLES_H
