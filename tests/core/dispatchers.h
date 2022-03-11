// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace GameKit
{
    namespace Tests
    {
        /**
         * Templated struct that dispatches a call to a Functor object.
         * Used to wrap member functions as function pointers so that they can be used as callbacks for GameKit low level APIs.
         * Example: if a low level GameKit Api has the form of
         *      void GameKitLowLevelSomeFunction(GAMEKIT_HANDLE handle, DISPATCH_RECEIVER_HANDLE receiver, CallbackFunc callback)
         * and CallbackFunc is
         *      typedef void(*CallbackFunc)(DISPATCH_RECEIVER_HANDLE receiver, Arg1 arg, Arg2 arg)
         *
         * Then declare a member function in a class of the form
         *      void Class::MemberFunction(Arg1 arg, Arg2 arg)
         *
         * Define a FunctorDispatcher typedef that wraps the member function
         *      typedef FunctorDispatcher<void(Class::*)(Arg1, Arg2), &Class::MemberFunction> MemberFunctionDispatcher;
         *
         * Then the MemberFunctionDispatcher can be used when calling the low level API:
         *      GameKitLowLevelSomeFunction(handle, this, &MemberFunctionDispatcher::Dispatch);
         */
        template <typename Functor, Functor> struct FunctorDispatcher;
        template <typename Functor, typename RetType, typename ... Args, RetType(Functor::* CbFunc)(Args...)>
        struct FunctorDispatcher<RetType(Functor::*)(Args...), CbFunc>
        {
            static RetType Dispatch(void* obj, Args... args)
            {
                Functor* instance = static_cast<Functor*>(obj);
                return (instance->*CbFunc)(std::forward<Args>(args)...);
            }

            static RetType Dispatch(void* obj, Args&&... args)
            {
                Functor* instance = static_cast<Functor*>(obj);
                return (instance->*CbFunc)(std::forward<Args>(args)...);
            }
        };

        /**
         * Templated struct that dispatches a call to a Lambda function.
         * Used to wrap Lambdas as function pointers so that they can be used as callbacks for GameKit low level APIs.
         * Example: if a low level GameKit Api has the form of
         *      void GameKitLowLevelSomeFunction(GAMEKIT_HANDLE handle, DISPATCH_RECEIVER_HANDLE receiver, CallbackFunc callback)
         * and CallbackFunc is
         *      typedef void(*CallbackFunc)(DISPATCH_RECEIVER_HANDLE receiver, Arg1 arg, Arg2 arg)
         *
         * Then declare a member lambda of the form
         *      auto lambdaFunction = [](Arg1 arg1, Arg2 arg2) -> void { ... };
         *
         * Define a LambdaDispatcher typedef that wraps the lambda function
         *      typedef LambdaDispatcher<decltype(lambdaFunction), void, Arg1, Arg2> LambdaFunctionDispatcher;
         *
         * Then the LambdaFunctionDispatcher can be used when calling the low level API:
         *      GameKitLowLevelSomeFunction(handle, (void*)lambdaFunction, &LambdaFunctionDispatcher::Dispatch);
         */
        template <typename Lambda, typename RetType, typename ... Args>
        struct LambdaDispatcher
        {
            static RetType Dispatch(void* func, Args... args)
            {
                return (*static_cast<Lambda*>(func)) (std::forward<Args>(args)...);
            }

            static RetType Dispatch(void* func, Args&&... args)
            {
                return (*static_cast<Lambda*>(func)) (std::forward<Args>(args)...);
            }
        };
    }
}