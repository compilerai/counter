#pragma once

#include <memory>
#include <string>
#include <boost/iostreams/filtering_stream.hpp>

namespace bio = boost::iostreams;
using namespace std;

unique_ptr<bio::filtering_ostream> plain_ostream_p(string const& filename);
unique_ptr<bio::filtering_istream> plain_istream_p(string const& filename);

unique_ptr<bio::filtering_ostream> gzip_ostream_p(string const& filename);
unique_ptr<bio::filtering_istream> gzip_istream_p(string const& filename);

unique_ptr<bio::filtering_istream> plain_or_gzip_istream_p(string const& filename);

bool file_exists(string const& filename);
bool file_is_gzip_compressed(string const& filename);
