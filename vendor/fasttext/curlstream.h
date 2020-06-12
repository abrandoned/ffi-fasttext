#include <iostream>
#include <streambuf>
#include <curl/curl.h>
#include <string.h>

class CurlStreambuff : public std::streambuf {
public:
  explicit CurlStreambuff(const std::string& url);
  ~CurlStreambuff();

  // Copy and move prohibited
  CurlStreambuff(const CurlStreambuff&) = delete;
  CurlStreambuff& operator = (const CurlStreambuff&) = delete;
  CurlStreambuff(CurlStreambuff && src) = delete;
  CurlStreambuff& operator = (CurlStreambuff && rhs) = delete;

protected:
  std::streamsize xsgetn(char *s, std::streamsize n) override;
  int underflow() override;
  int uflow() override;

private:
  static int writer_callback(char *data, size_t sz, size_t nmemb, void* ptr);
  size_t fillbuffer();
  
  CURL *m_http_handle = nullptr;
  CURLM *m_multi_handle = nullptr;
  char m_buffer[CURL_MAX_WRITE_SIZE];
  size_t m_pos = 0, m_sz = 0;
};

class CurlStream : private CurlStreambuff, public std::istream {
public:
  explicit CurlStream(const std::string& url)
    : CurlStreambuff(url), std::istream(this)
  {
  }
};

