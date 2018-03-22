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

  DatabaseMiddleware() : engine_("mysql://crow:crow@127.0.0.1/flask") {
  }

  void before_handle(crow::request &, crow::response &, context &ctxt) {
    ctxt.conn = engine_.connect();
  }

  void after_handle(crow::request &, crow::response &, context &) {
  }

  crow::db::mysql::Engine engine_;
};


using CrowApp = crow::App<DatabaseMiddleware>;


std::unique_ptr<CrowApp> CreateApp() {
  std::unique_ptr<CrowApp> _app = std::make_unique<CrowApp>();
  CrowApp &app = *_app;

  CROW_ROUTE(app, "/") ([]() {
        crow::json::wvalue res;
        res["message"] = "Hello, world";
        return res;
     });

  return _app;
}


int main(int, char**) {
  auto app = CreateApp();
  app->port(18080).concurrency(4).run();
  return 0;
}
