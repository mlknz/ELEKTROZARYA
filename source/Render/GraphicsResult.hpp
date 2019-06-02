#pragma once

#include <utility>
#include <Vulkan/Vulkan.hpp>

namespace Ride
{
    enum class GraphicsResult
    {
        Ok,
        Error
    };

    template<typename Value> class ResultValue
    {
    public:
        ResultValue(GraphicsResult aResult)
            : result(aResult) {}

        ResultValue(GraphicsResult aResult, Value&& aValue)
                    : result(aResult)
                    , value(std::forward<Value>(aValue)) {}

        ResultValue(GraphicsResult aResult, const Value& aValue)
                    : result(aResult)
                    , value(aValue) {}

        ResultValue(vk::Result result)
                    : ResultValue(ToGraphicsResult(result)) {}

        ResultValue(vk::Result result, Value&& val)
                    : ResultValue(ToGraphicsResult(result), std::forward<Value>(val)) {}

        ResultValue(vk::Result result, const Value& val)
                    : ResultValue(ToGraphicsResult(result), val) {}

        ResultValue(ResultValue&& other)
                    : result(std::move(other.result))
                    , value(std::move(other.value)) {}

        GraphicsResult result;
        Value value;

    private:
        GraphicsResult ToGraphicsResult(vk::Result vkResult)
        {
            switch (vkResult)
            {
            case vk::Result::eSuccess:
                return GraphicsResult::Ok;
            default:
                return GraphicsResult::Error;
            }
        }
    };
}
