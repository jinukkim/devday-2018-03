#pragma once

#include <string>
#include <cassert>

namespace crow 
{
    namespace db 
    {
        class Connection
        {
            public:
                                
        };

        class Engine
        {
            public:
                Connection connect();
        };

		inline Engine engine(const std::string& url)
        {
            assert(url.substr(0, 8) == "mysql://");
            return Engine();
        }
    }
}
