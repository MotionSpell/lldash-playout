// MPEG-DASH live simulator
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>

static auto const SegmentDuration = 1000LL;
static auto const FragmentDuration = 200LL;
static auto const FragmentsPerSegment = SegmentDuration / FragmentDuration;

using namespace std;

struct HttpRequest
{
  string method; // e.g: PUT, POST, GET
  string url; // e.g: /toto/dash.mp4
};

string readLine()
{
  char line[2048];

  if(!fgets(line, sizeof line, stdin))
    return "";

  string r = line;

  // remove trailing EOL
  while(!r.empty() && (r.back() == '\r' || r.back() == '\n'))
    r.pop_back();

  return r;
}

HttpRequest parseRequest()
{
  HttpRequest r;

  string req = readLine();

  stringstream ss(req);
  ss >> r.method;
  ss >> std::ws;
  ss >> r.url;

  while(1)
  {
    auto line = readLine();

    if(line.empty())
      break;
  }

  return r;
}

static const char mpd[] = R"(<?xml version="1.0" encoding="utf-8"?>
<MPD
  availabilityStartTime="1970-01-01T00:00:00Z"
  maxSegmentDuration="PT2S"
  timeShiftBufferDepth="PT5M"
  type="dynamic">
  <Period id="p0" start="PT0S">
    <AdaptationSet contentType="video" mimeType="video/mp4" segmentAlignment="true" startWithSAP="1">
      <SegmentTemplate
        timescale="1000" duration="1000"
        initialization="init.mp4"
        media="$Number$.m4s"
        startNumber="0" />
      <Representation bandwidth="300000" codecs="cwi1" id="1" />
    </AdaptationSet>
  </Period>
</MPD>
)";

static const uint8_t initChunk[] =
{
  0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6f, 0x6d,
  0x00, 0x00, 0x00, 0x01, 0x69, 0x73, 0x6f, 0x6d, 0x64, 0x61, 0x73, 0x68,

  0x00, 0x00, 0x02, 0x71, 0x6d,
  0x6f, 0x6f, 0x76, 0x00, 0x00, 0x00, 0x6c, 0x6d, 0x76, 0x68, 0x64, 0x00,
  0x00, 0x00, 0x00, 0xd9, 0x23, 0xd4, 0x5e, 0xd9, 0x23, 0xd4, 0x5e,

  // timescale
  0x00, 0x00, 0x03, 0xe8,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x38, 0x6d, 0x76, 0x65, 0x78, 0x00,
  0x00, 0x00, 0x10, 0x6d, 0x65, 0x68, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x74, 0x72, 0x65, 0x78, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x01, 0xc5, 0x74, 0x72, 0x61, 0x6b, 0x00, 0x00, 0x00, 0x5c, 0x74,
  0x6b, 0x68, 0x64, 0x00, 0x00, 0x00, 0x01, 0xd9, 0x23, 0xd4, 0x5e, 0xd9,
  0x23, 0xd4, 0x5e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x61, 0x6d, 0x64, 0x69, 0x61,

  0x00, 0x00, 0x00, 0x20, 'm', 'd', 'h', 'd',
  0x00, 0x00, 0x00, 0x00, 0xd9, 0x23, 0xd4, 0x5e, 0xd9, 0x23, 0xd4, 0x5e,

  // timescale
  0x00, 0x00, 0x03, 0xe8,

  0x00,
  0x00, 0x00, 0x00, 0x55, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x68,
  0x64, 0x6c, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76,
  0x69, 0x64, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x47, 0x50, 0x41, 0x43, 0x20, 0x49, 0x53, 0x4f, 0x20,
  0x56, 0x69, 0x64, 0x65, 0x6f, 0x20, 0x48, 0x61, 0x6e, 0x64, 0x6c, 0x65,
  0x72, 0x00, 0x00, 0x00, 0x01, 0x02, 0x6d, 0x69, 0x6e, 0x66, 0x00, 0x00,
  0x00, 0x14, 0x76, 0x6d, 0x68, 0x64, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x64, 0x69,
  0x6e, 0x66, 0x00, 0x00, 0x00, 0x1c, 0x64, 0x72, 0x65, 0x66, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x75, 0x72,
  0x6c, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xc2, 0x73, 0x74,
  0x62, 0x6c, 0x00, 0x00, 0x00, 0x66, 0x73, 0x74, 0x73, 0x64, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x56, 0x63, 0x77,
  0x69, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x47, 0x50, 0x41, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x48,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xff, 0xff, 0x00, 0x00, 0x00, 0x10,
  0x73, 0x74, 0x74, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x10, 0x73, 0x74, 0x73, 0x73, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x73, 0x74, 0x73, 0x63,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
  0x73, 0x74, 0x73, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x73, 0x74, 0x63, 0x6f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

std::vector<uint8_t> getFragment(int64_t index)
{
  int64_t cts = index * FragmentDuration;

  std::vector<uint8_t> r;

  auto write = [&] (std::vector<uint8_t> const& data)
    {
      for(auto c : data)
        r.push_back(c);
    };

  auto writeInt = [&] (int64_t val, int nbytes)
    {
      for(int i = 0; i < nbytes; ++i)
        r.push_back((val >> ((nbytes - 1 - i) * 8)) & 0xff);
    };

  write({
    // moof
    0x00, 0x00, 0x00, 0x6c, 'm', 'o', 'o', 'f',

    // mfhd
    0x00, 0x00, 0x00, 0x10, 'm', 'f', 'h', 'd',
    0x00, // version
    0x00, 0x00, 0x00, // flags
  });

  writeInt(index, 4); // sequence_number

  write({
    // traf
    0x00, 0x00, 0x00, 0x54, 't', 'r', 'a', 'f',

    // tfhd
    0x00, 0x00, 0x00, 0x1c, 't', 'f', 'h', 'd',
    0x00, // version
    0x02, 0x00, 0x38, // flags: default-base-is-moof
  });

  writeInt(1, 4); // track-id
  writeInt(FragmentDuration, 4); // default-sample-duration

  write({
    0x00, 0x00, 0x00, 0x08, // default-sample-size
    0x02, 0x00, 0x00, 0x00, // default-sample-flags

    // tfdt
    0x00, 0x00, 0x00, 0x14, 't', 'f', 'd', 't',
    0x01, // version
    0x00, 0x00, 0x00, // flags
  });

  writeInt(cts, 8); // baseMediaDecodeTime

  write({
    // trun
    0x00, 0x00, 0x00, 0x1c, 't', 'r', 'u', 'n',
    0x00, // version
    0x00, 0x00, 0x01, // flags
    0x00, 0x00, 0x00, 0x01, // sample count
    0x00, 0x00, 0x00, 0x74, // data-offset
    0x00, 0x00, 0x02, 0x00, // first-sample-flags: sample-size-present

    0x00, 0x00, 0x00, 0x04, // sample[0].size
  });

  write({
    // mdat
    0x00, 0x00, 0x00, 0x10, 'm', 'd', 'a', 't',
  });

  writeInt(0x1122334455667788, 8); // data

  return r;
}

void sendLine(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
  fprintf(stdout, "\r\n");
}

void sendChunk(const void* ptr, size_t len)
{
  sendLine("%X", len);
  if(ptr)
    fwrite(ptr, 1, len, stdout);
  sendLine("");
}

int main()
{
  long long reqNumber = 0;
  auto req = parseRequest();

  if(req.method != "GET")
  {
    fprintf(stderr, "Unhandled method '%s'", req.method.c_str());
    return 1;
  }

  if(req.url == "/latency.mpd")
  {
    sendLine("HTTP/1.1 200 OK");
    sendLine("Transfer-Encoding: chunked");
    sendLine("");
    sendChunk(mpd, (sizeof mpd) - 1);
    sendChunk(nullptr, 0);
  }
  else if(req.url == "/init.mp4")
  {
    sendLine("HTTP/1.1 200 OK");
    sendLine("Transfer-Encoding: chunked");
    sendLine("");
    sendChunk(initChunk, sizeof initChunk);
    sendChunk(nullptr, 0);
  }
  else if(sscanf(req.url.c_str(), "/%lld.m4s", &reqNumber) == 1)
  {
    auto const reqTime = reqNumber * SegmentDuration;
    auto const currTime = (time(nullptr) * 1000LL);
    auto const deltaTime = reqTime - currTime;

    if(deltaTime > 0)
      this_thread::sleep_for(chrono::milliseconds(deltaTime));

    fprintf(stderr, "\n[server] HTTP-GET segment number: %lld\n", reqNumber);

    sendLine("HTTP/1.1 200 OK");
    sendLine("Transfer-Encoding: chunked");
    sendLine("");
    for(int i=0;i < FragmentsPerSegment;++i)
    {
      auto fragment = getFragment(reqTime / FragmentDuration + i);
      sendChunk(fragment.data(), fragment.size());
    }
    sendChunk(nullptr, 0);
    fprintf(stderr, "[server] Sent %d fragments\n", int(FragmentsPerSegment));
  }
  else
  {
    sendLine("HTTP/1.1 404 Not found");
    sendLine("");
  }

  return 0;
}

