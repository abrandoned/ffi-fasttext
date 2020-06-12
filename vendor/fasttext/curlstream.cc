#include <memory>
#include <chrono>
#include <thread>
#include <streambuf>
#include "./curlstream.h"

class curl_global_initializer {
public:
  curl_global_initializer() { curl_global_init(CURL_GLOBAL_DEFAULT); }
  ~curl_global_initializer() { curl_global_cleanup(); }
};

namespace {
  bool curl_initialized()
  {
    static std::unique_ptr<curl_global_initializer> initializer = std::make_unique<curl_global_initializer>();
    return initializer != nullptr;
  }
}

CurlStreambuff::CurlStreambuff(const std::string& url)
{
  if (curl_initialized()) {
    m_http_handle = curl_easy_init();
    curl_easy_setopt(m_http_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_http_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_http_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_http_handle, CURLOPT_WRITEFUNCTION, &CurlStreambuff::writer_callback);
    curl_easy_setopt(m_http_handle, CURLOPT_WRITEDATA, this);

    m_multi_handle = curl_multi_init();
    curl_multi_add_handle(m_multi_handle, m_http_handle);
  }
}

CurlStreambuff::~CurlStreambuff()
{
  if (m_multi_handle && m_http_handle) {
    curl_multi_remove_handle(m_multi_handle, m_http_handle);
  } 
  if (m_http_handle) {
    curl_easy_cleanup(m_http_handle);
  }
  if (m_multi_handle) {
    curl_multi_cleanup(m_multi_handle);
  }
}

std::streamsize CurlStreambuff::xsgetn(char *s, std::streamsize n)
{
  auto remaining = n;
  while (remaining > 0) {
    if (m_pos >= m_sz) {
      if (fillbuffer() == 0) {
        break;
      }
    }
    auto to_copy = std::min<size_t>(remaining, m_sz - m_pos);
    memcpy(s, &m_buffer[m_pos], to_copy);
    s += to_copy;
    m_pos += to_copy;
    remaining -= to_copy;    
  }
  return n - remaining;
}

int CurlStreambuff::underflow()
{
  char c;
  if (xsgetn(&c, 1) < 1) {
    return traits_type::eof();
  }
  return c;
}

int CurlStreambuff::writer_callback(char *data, size_t sz, size_t nmemb, void* ptr)
{
  auto self = static_cast<CurlStreambuff*>(ptr);
  auto bytes = sz * nmemb;
  if(bytes > sizeof(m_buffer) || bytes == 0) {
    return 0;
  }
  memcpy(&self->m_buffer[0], data, bytes);
  self->m_sz = bytes;
  self->m_pos = 0;
  return bytes;
}

size_t CurlStreambuff::fillbuffer()
{
  using namespace std::chrono_literals;
  m_sz = 0;
  int still_running_count = 0, repeats = 0;
  curl_multi_perform(m_multi_handle, &still_running_count);
  while (still_running_count > 0) {
    /* wait for activity, timeout or "nothing" */ 
    int numfds;
    auto mc = curl_multi_wait(m_multi_handle, nullptr, 0, 1000, &numfds);

    if(mc != CURLM_OK) {
      break;
    }

    /* 'numfds' being zero means either a timeout or no file descriptors to
       wait for. Try timeout on first occurrence, then assume no file
       descriptors and no file descriptors to wait for means wait for 100
       milliseconds. */ 
    if(numfds == 0) {
      if (++repeats > 1) {
        std::this_thread::sleep_for(100ms);
      }
    } else if (m_sz > 0) {
      break;
    }
    curl_multi_perform(m_multi_handle, &still_running_count);
  }
  return m_sz;
}

