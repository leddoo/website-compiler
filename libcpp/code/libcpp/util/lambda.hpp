#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/allocator.hpp>


namespace libcpp {

    template <typename T>
    struct Lambda;

    template <typename Return, typename... Args>
    struct Lambda<Return(Args...)> {
        Return operator()(Args... args) const {
            return lambda->call(args...);
        }

        struct Callable_Base {
            virtual ~Callable_Base() = default;
            virtual Return call(Args...) = 0;
        };

        template <typename T>
        struct Callable : public Callable_Base {
            T callable;

            Callable(const T &t) : callable(t) {}
            ~Callable() override = default;
            Return call(Args... args) override {
                return callable(args...);
            }
        };


        template <typename T>
        static Lambda<Return(Args...)> create(const T &t, Allocator &allocator) {
            auto result = Lambda<Return(Args...)> {};
            result.lambda = allocate_uninitialized<Callable<T>>(allocator);
            new(result.lambda) Callable<T> { t };
            return result;
        }

        Lambda() = default;

        Callable_Base *lambda;
    };

}

