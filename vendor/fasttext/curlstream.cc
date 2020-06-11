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

CurlStreambuff::CurlStreambuff(CurlStreambuff && src)
{
  std::swap(m_http_handle, src.m_http_handle);
  std::swap(m_multi_handle, src.m_multi_handle);
  std::swap(m_buffer, src.m_buffer);
  std::swap(m_pos, src.m_pos);
}

CurlStreambuff& CurlStreambuff::operator = (CurlStreambuff && rhs)
{
  std::swap(m_http_handle, rhs.m_http_handle);
  std::swap(m_multi_handle, rhs.m_multi_handle);
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_pos, rhs.m_pos);
  return *this;
}

std::streamsize CurlStreambuff::xsgetn(char *s, std::streamsize n)
{
  auto remaining = n;
  while (remaining > 0) {
    if (m_pos >= m_buffer.size()) {
      if (fillbuffer() == 0) {
        break;
      }
    }
    auto to_copy = std::min<size_t>(remaining, m_buffer.size() - m_pos);
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
  self->m_buffer.resize(sz * nmemb);
  if(self->m_buffer.empty()) {
    return 0;
  }
  memcpy(&self->m_buffer[0], data, self->m_buffer.size());
  self->m_pos = 0;
  return sz * nmemb;
}

size_t CurlStreambuff::fillbuffer()
{
  using namespace std::chrono_literals;
  m_buffer.clear();
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
    } else if (!m_buffer.empty()) {
      break;
    }
    curl_multi_perform(m_multi_handle, &still_running_count);
  }
  return m_buffer.size();
}

int main(int argc, char **argv)
{
  CurlStream stream(argc < 2 ? "https://www.example.com" : argv[1]);
  
  char buffer[32 + 1];
  while (!stream.eof()) {
    stream.read(buffer, 32);
    buffer[stream.gcount()] = 0;
    std::cout << buffer;
  }
  std::cout << '\n';
}

