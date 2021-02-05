
#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <iostream>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using error_code = boost::system::error_code;

namespace {
    std::string
    chunkify(std::string_view s)
    {
        std::ostringstream ss;
        ss << std::hex << s.size();
        auto result = boost::to_upper_copy(ss.str()) + "\r\n";
        result.append(s.begin(), s.end());
        result += "\r\n";
        return result;
    }

    void
    call_app_handler(error_code ec, beast::string_view msg)
    {
        std::cout << "call_app_handler: ec=" << ec.message() << ", msg=" << msg
                  << std::endl;
    }

    template<typename Cont>
    void
    sync_read(auto &stream_, auto &buffer_, Cont &&cont)
    {
        beast::error_code ec{};
        http::response_parser<http::buffer_body> res0;
        http::read_header(stream_, buffer_, res0, ec);
        if (ec) { return call_app_handler(ec, "read: header"); }
        int field_count = 0;
        for (auto const &field : res0.get())
            std::cout << "Field#" << ++field_count << " : " << field.name()
                      << " = " << field.value() << std::endl;

        std::stringstream os;
        while (!res0.is_done())
        {
            char buf[512];
            res0.get().body().data = buf;
            res0.get().body().size = sizeof(buf);
            http::read(stream_, buffer_, res0, ec);
            if (ec == http::error::need_buffer) ec = {};
            if (ec) return call_app_handler(ec, "read: body");
            ;
            os.write(buf, sizeof(buf) - res0.get().body().size);
        }
        // http::response_parser<http::string_body> res{std::move(res0)};
        // http::read(stream_, buffer_, res,ec);
        // if(ec){
        //     return call_app_handler(ec,"read: body");
        // }
        // call_app_handler(ec,res.get().body());
        call_app_handler(ec, os.str());
        cont();
    }
}// namespace
int
main()
{
    net::io_context ioc;
    beast::test::stream s(ioc);

    s.append("HTTP/1.1 500\r\n"
             "Date: Thu, 04 Feb 2021 04:41:45 GMT\r\n"
             "Content-Type: application/json;charset=UTF-8\r\n"
             "Transfer-Encoding: chunked\r\n"
             "Connection: keep-alive\r\n"
             "\r\n");
    s.append(chunkify(
    "{\"timestamp\":\"2021-02-04T04:41:45.301+0000\",\"status\":500,\"error\":"
    "\"Internal Server Error\",\"message\":\"App has reached an unexpected "
    "condition\",\"path\":\"/shortcut/v1/cui\"}"));
    s.append(chunkify(""));

    beast::flat_buffer buf;
    sync_read(s, buf, []() { std::cout << "all done\n"; });
    return 0;
}
