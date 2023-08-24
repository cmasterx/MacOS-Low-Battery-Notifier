#pragma once

#include <iostream>

namespace debug
{
    template <typename T>
    void print(T msg)
    {
        std::cout << msg;
    }

    template <typename T, typename ...Args>
    void print(T first, Args ...args)
    {
        std::cout << first << ' ';
        print(args...);
    }

    template <typename ...Args>
    void println(Args ...args)
    {
        print(args...);
        std::cout << std::endl;
    }
}
