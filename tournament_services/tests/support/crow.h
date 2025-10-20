
namespace crow {
  struct request {};
  struct response {
    int code = 200;
    std::string body;
  };
}
