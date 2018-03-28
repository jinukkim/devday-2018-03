#include <iostream>

#include <crow.h>
#include <crowdb/db.h>
#include <crowdb/backend/mysql_backend.h>


using std::cout;
using std::endl;


using ConnPtr = std::unique_ptr<
    crow::db::mysql::Connection, crow::db::mysql::Engine::ReturnToPool>;


struct DatabaseMiddleware {
  struct context {
    ConnPtr conn;
  };

  DatabaseMiddleware() : engine_() {
  }

  void initialize(const std::string &connection_string) {
    engine_ = std::make_unique<crow::db::mysql::Engine>(connection_string);
  }

  __attribute__((noinline))
  void before_handle(crow::request &, crow::response &, context &ctxt) {
    ctxt.conn = engine_->connect();
  }

  __attribute__((noinline))
  void after_handle(crow::request &, crow::response &, context &) {
  }

  std::unique_ptr<crow::db::mysql::Engine> engine_;
};


struct DnsReverseLookupMiddleware {
  struct context {
  };

  __attribute__((noinline))
  void before_handle(crow::request &req, crow::response &, context &) {
    boost::asio::ip::tcp::resolver resolver(*req.io_service);
    boost::system::error_code ec;
    boost::asio::ip::tcp::resolver::iterator ie, it =
        resolver.resolve(req.remote_addr, ec);
    if (it != ie) {
      req.add_header("REMOTE_ADDR", it->host_name());
    }
  }

  __attribute__((noinline))
  void after_handle(crow::request &, crow::response &, context &) {
  }
};


using CrowApp = crow::App<DatabaseMiddleware, DnsReverseLookupMiddleware>;


std::string HandleIndex(CrowApp &app, const crow::request &req) {
  auto &ctxt = app.get_context<DatabaseMiddleware>(req);
  auto result = ctxt.conn->execute<int, std::string, std::string>(
      "SELECT id, title, body FROM entries ORDER BY id DESC");

  std::vector<crow::json::wvalue> posts;
  for (auto row : result) {
    crow::json::wvalue post;
    post["id"] = crow::db::get<0>(row);
    post["title"] = crow::db::get<1>(row);
    post["text"] = crow::db::get<2>(row);
    posts.emplace_back(std::move(post));
  }

  crow::json::wvalue res;
  res["posts"] = std::move(posts);

  return crow::json::dump(res);
}


std::unique_ptr<CrowApp> CreateApp() {
  std::unique_ptr<CrowApp> _app = std::make_unique<CrowApp>();
  CrowApp &app = *_app;

  CROW_ROUTE(app, "/") ([app=&app](const crow::request& req) {
      return HandleIndex(*app, req);
     });

  return _app;
}


int main(int, char**) {
  auto app = CreateApp();

  app->get_middleware<DatabaseMiddleware>().initialize(
      "mysql://crow:crow@127.0.0.1/flask");

  app->port(18080).concurrency(4).run();
  return 0;
}
