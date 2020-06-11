#include <vector>
#include <iostream>
#include <streambuf>
#include <curl/curl.h>
#include <string.h>

class CurlStreambuff : public std::streambuf {
public:
  explicit CurlStreambuff(const std::string& url);
  ~CurlStreambuff();

  // Copy prohibited
  CurlStreambuff(const CurlStreambuff&) = delete;
  CurlStreambuff& operator = (const CurlStreambuff&) = delete;

  // Move permitted
  CurlStreambuff(CurlStreambuff && src);
  CurlStreambuff& operator = (CurlStreambuff && rhs);

protected:
  std::streamsize xsgetn(char *s, std::streamsize n) override;
  int underflow() override;

private:
  static int writer_callback(char *data, size_t sz, size_t nmemb, void* ptr);
  size_t fillbuffer();
  
  CURL *m_http_handle = nullptr;
  CURLM *m_multi_handle = nullptr;
  std::vector<char> m_buffer;
  size_t m_pos = 0;
};

class CurlStream : private CurlStreambuff, public std::istream {
public:
  explicit CurlStream(const std::string& url)
    : CurlStreambuff(url), std::istream(this)
  {
  }
};

